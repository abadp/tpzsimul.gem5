/*
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <string>
#include <sstream>

#include "base/cprintf.hh"
#include "base/loader/symtab.hh"
#include "base/misc.hh"
#include "base/output.hh"
#include "cpu/base.hh"
#include "cpu/exec_context.hh"
#include "cpu/sampler/sampler.hh"
#include "sim/param.hh"
#include "sim/sim_events.hh"

#include "base/trace.hh"

using namespace std;

vector<BaseCPU *> BaseCPU::cpuList;

// This variable reflects the max number of threads in any CPU.  Be
// careful to only use it once all the CPUs that you care about have
// been initialized
int maxThreadsPerCPU = 1;

#if FULL_SYSTEM
BaseCPU::BaseCPU(Params *p)
    : SimObject(p->name), clock(p->clock), checkInterrupts(true),
      params(p), number_of_threads(p->numberOfThreads), system(p->system)
#else
BaseCPU::BaseCPU(Params *p)
    : SimObject(p->name), clock(p->clock), params(p),
      number_of_threads(p->numberOfThreads)
#endif
{
    DPRINTF(FullCPU, "BaseCPU: Creating object, mem address %#x.\n", this);

    // add self to global list of CPUs
    cpuList.push_back(this);

    DPRINTF(FullCPU, "BaseCPU: CPU added to cpuList, mem address %#x.\n",
            this);

    if (number_of_threads > maxThreadsPerCPU)
        maxThreadsPerCPU = number_of_threads;

    // allocate per-thread instruction-based event queues
    comInstEventQueue = new EventQueue *[number_of_threads];
    for (int i = 0; i < number_of_threads; ++i)
        comInstEventQueue[i] = new EventQueue("instruction-based event queue");

    //
    // set up instruction-count-based termination events, if any
    //
    if (p->max_insts_any_thread != 0)
        for (int i = 0; i < number_of_threads; ++i)
            new SimExitEvent(comInstEventQueue[i], p->max_insts_any_thread,
                "a thread reached the max instruction count");

    if (p->max_insts_all_threads != 0) {
        // allocate & initialize shared downcounter: each event will
        // decrement this when triggered; simulation will terminate
        // when counter reaches 0
        int *counter = new int;
        *counter = number_of_threads;
        for (int i = 0; i < number_of_threads; ++i)
            new CountedExitEvent(comInstEventQueue[i],
                "all threads reached the max instruction count",
                p->max_insts_all_threads, *counter);
    }

    // allocate per-thread load-based event queues
    comLoadEventQueue = new EventQueue *[number_of_threads];
    for (int i = 0; i < number_of_threads; ++i)
        comLoadEventQueue[i] = new EventQueue("load-based event queue");

    //
    // set up instruction-count-based termination events, if any
    //
    if (p->max_loads_any_thread != 0)
        for (int i = 0; i < number_of_threads; ++i)
            new SimExitEvent(comLoadEventQueue[i], p->max_loads_any_thread,
                "a thread reached the max load count");

    if (p->max_loads_all_threads != 0) {
        // allocate & initialize shared downcounter: each event will
        // decrement this when triggered; simulation will terminate
        // when counter reaches 0
        int *counter = new int;
        *counter = number_of_threads;
        for (int i = 0; i < number_of_threads; ++i)
            new CountedExitEvent(comLoadEventQueue[i],
                "all threads reached the max load count",
                p->max_loads_all_threads, *counter);
    }

#if FULL_SYSTEM
    memset(interrupts, 0, sizeof(interrupts));
    intstatus = 0;
#endif

    functionTracingEnabled = false;
    if (p->functionTrace) {
        functionTraceStream = simout.find(csprintf("ftrace.%s", name()));
        currentFunctionStart = currentFunctionEnd = 0;
        functionEntryTick = p->functionTraceStart;

        if (p->functionTraceStart == 0) {
            functionTracingEnabled = true;
        } else {
            Event *e =
                new EventWrapper<BaseCPU, &BaseCPU::enableFunctionTrace>(this,
                                                                         true);
            e->schedule(p->functionTraceStart);
        }
    }
}


void
BaseCPU::enableFunctionTrace()
{
    functionTracingEnabled = true;
}

BaseCPU::~BaseCPU()
{
}

void
BaseCPU::init()
{
    if (!params->deferRegistration)
        registerExecContexts();
}

void
BaseCPU::regStats()
{
    using namespace Stats;

    numCycles
        .name(name() + ".numCycles")
        .desc("number of cpu cycles simulated")
        ;

    int size = execContexts.size();
    if (size > 1) {
        for (int i = 0; i < size; ++i) {
            stringstream namestr;
            ccprintf(namestr, "%s.ctx%d", name(), i);
            execContexts[i]->regStats(namestr.str());
        }
    } else if (size == 1)
        execContexts[0]->regStats(name());
}


void
BaseCPU::registerExecContexts()
{
    for (int i = 0; i < execContexts.size(); ++i) {
        ExecContext *xc = execContexts[i];
#if FULL_SYSTEM
        int id = params->cpu_id;
        if (id != -1)
            id += i;

        xc->cpu_id = system->registerExecContext(xc, id);
#else
        xc->cpu_id = xc->process->registerExecContext(xc);
#endif
    }
}


void
BaseCPU::switchOut(Sampler *sampler)
{
    panic("This CPU doesn't support sampling!");
}

void
BaseCPU::takeOverFrom(BaseCPU *oldCPU)
{
    assert(execContexts.size() == oldCPU->execContexts.size());

    for (int i = 0; i < execContexts.size(); ++i) {
        ExecContext *newXC = execContexts[i];
        ExecContext *oldXC = oldCPU->execContexts[i];

        newXC->takeOverFrom(oldXC);
        assert(newXC->cpu_id == oldXC->cpu_id);
#if FULL_SYSTEM
        system->replaceExecContext(newXC, newXC->cpu_id);
#else
        assert(newXC->process == oldXC->process);
        newXC->process->replaceExecContext(newXC, newXC->cpu_id);
#endif
    }

#if FULL_SYSTEM
    for (int i = 0; i < NumInterruptLevels; ++i)
        interrupts[i] = oldCPU->interrupts[i];
    intstatus = oldCPU->intstatus;
#endif
}


#if FULL_SYSTEM
void
BaseCPU::post_interrupt(int int_num, int index)
{
    DPRINTF(Interrupt, "Interrupt %d:%d posted\n", int_num, index);

    if (int_num < 0 || int_num >= NumInterruptLevels)
        panic("int_num out of bounds\n");

    if (index < 0 || index >= sizeof(uint64_t) * 8)
        panic("int_num out of bounds\n");

    checkInterrupts = true;
    interrupts[int_num] |= 1 << index;
    intstatus |= (ULL(1) << int_num);
}

void
BaseCPU::clear_interrupt(int int_num, int index)
{
    DPRINTF(Interrupt, "Interrupt %d:%d cleared\n", int_num, index);

    if (int_num < 0 || int_num >= NumInterruptLevels)
        panic("int_num out of bounds\n");

    if (index < 0 || index >= sizeof(uint64_t) * 8)
        panic("int_num out of bounds\n");

    interrupts[int_num] &= ~(1 << index);
    if (interrupts[int_num] == 0)
        intstatus &= ~(ULL(1) << int_num);
}

void
BaseCPU::clear_interrupts()
{
    DPRINTF(Interrupt, "Interrupts all cleared\n");

    memset(interrupts, 0, sizeof(interrupts));
    intstatus = 0;
}


void
BaseCPU::serialize(std::ostream &os)
{
    SERIALIZE_ARRAY(interrupts, NumInterruptLevels);
    SERIALIZE_SCALAR(intstatus);
}

void
BaseCPU::unserialize(Checkpoint *cp, const std::string &section)
{
    UNSERIALIZE_ARRAY(interrupts, NumInterruptLevels);
    UNSERIALIZE_SCALAR(intstatus);
}

#endif // FULL_SYSTEM

void
BaseCPU::traceFunctionsInternal(Addr pc)
{
    if (!debugSymbolTable)
        return;

    // if pc enters different function, print new function symbol and
    // update saved range.  Otherwise do nothing.
    if (pc < currentFunctionStart || pc >= currentFunctionEnd) {
        string sym_str;
        bool found = debugSymbolTable->findNearestSymbol(pc, sym_str,
                                                         currentFunctionStart,
                                                         currentFunctionEnd);

        if (!found) {
            // no symbol found: use addr as label
            sym_str = csprintf("0x%x", pc);
            currentFunctionStart = pc;
            currentFunctionEnd = pc + 1;
        }

        ccprintf(*functionTraceStream, " (%d)\n%d: %s",
                 curTick - functionEntryTick, curTick, sym_str);
        functionEntryTick = curTick;
    }
}


DEFINE_SIM_OBJECT_CLASS_NAME("BaseCPU", BaseCPU)