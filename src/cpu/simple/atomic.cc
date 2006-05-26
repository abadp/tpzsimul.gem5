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

#include "arch/utility.hh"
#include "cpu/exetrace.hh"
#include "cpu/simple/atomic.hh"
#include "mem/packet_impl.hh"
#include "sim/builder.hh"

using namespace std;
using namespace TheISA;

AtomicSimpleCPU::TickEvent::TickEvent(AtomicSimpleCPU *c)
    : Event(&mainEventQueue, CPU_Tick_Pri), cpu(c)
{
}


void
AtomicSimpleCPU::TickEvent::process()
{
    cpu->tick();
}

const char *
AtomicSimpleCPU::TickEvent::description()
{
    return "AtomicSimpleCPU tick event";
}


void
AtomicSimpleCPU::init()
{
    //Create Memory Ports (conect them up)
    Port *mem_dport = mem->getPort("");
    dcachePort.setPeer(mem_dport);
    mem_dport->setPeer(&dcachePort);

    Port *mem_iport = mem->getPort("");
    icachePort.setPeer(mem_iport);
    mem_iport->setPeer(&icachePort);

    BaseCPU::init();
#if FULL_SYSTEM
    for (int i = 0; i < execContexts.size(); ++i) {
        ExecContext *xc = execContexts[i];

        // initialize CPU, including PC
        TheISA::initCPU(xc, xc->readCpuId());
    }
#endif
}

bool
AtomicSimpleCPU::CpuPort::recvTiming(Packet *pkt)
{
    panic("AtomicSimpleCPU doesn't expect recvAtomic callback!");
    return true;
}

Tick
AtomicSimpleCPU::CpuPort::recvAtomic(Packet *pkt)
{
    panic("AtomicSimpleCPU doesn't expect recvAtomic callback!");
    return curTick;
}

void
AtomicSimpleCPU::CpuPort::recvFunctional(Packet *pkt)
{
    panic("AtomicSimpleCPU doesn't expect recvFunctional callback!");
}

void
AtomicSimpleCPU::CpuPort::recvStatusChange(Status status)
{
    if (status == RangeChange)
        return;

    panic("AtomicSimpleCPU doesn't expect recvStatusChange callback!");
}

Packet *
AtomicSimpleCPU::CpuPort::recvRetry()
{
    panic("AtomicSimpleCPU doesn't expect recvRetry callback!");
    return NULL;
}


AtomicSimpleCPU::AtomicSimpleCPU(Params *p)
    : BaseSimpleCPU(p), tickEvent(this),
      width(p->width), simulate_stalls(p->simulate_stalls),
      icachePort(name() + "-iport", this), dcachePort(name() + "-iport", this)
{
    _status = Idle;

    ifetch_req = new Request(true);
    ifetch_req->setAsid(0);
    // @todo fix me and get the real cpu iD!!!
    ifetch_req->setCpuNum(0);
    ifetch_req->setSize(sizeof(MachInst));
    ifetch_pkt = new Packet(ifetch_req, Packet::ReadReq, Packet::Broadcast);
    ifetch_pkt->dataStatic(&inst);

    data_read_req = new Request(true);
    // @todo fix me and get the real cpu iD!!!
    data_read_req->setCpuNum(0);
    data_read_req->setAsid(0);
    data_read_pkt = new Packet(data_read_req, Packet::ReadReq,
                               Packet::Broadcast);
    data_read_pkt->dataStatic(&dataReg);

    data_write_req = new Request(true);
    // @todo fix me and get the real cpu iD!!!
    data_write_req->setCpuNum(0);
    data_write_req->setAsid(0);
    data_write_pkt = new Packet(data_write_req, Packet::WriteReq,
                                Packet::Broadcast);
}


AtomicSimpleCPU::~AtomicSimpleCPU()
{
}

void
AtomicSimpleCPU::serialize(ostream &os)
{
    BaseSimpleCPU::serialize(os);
    SERIALIZE_ENUM(_status);
    nameOut(os, csprintf("%s.tickEvent", name()));
    tickEvent.serialize(os);
}

void
AtomicSimpleCPU::unserialize(Checkpoint *cp, const string &section)
{
    BaseSimpleCPU::unserialize(cp, section);
    UNSERIALIZE_ENUM(_status);
    tickEvent.unserialize(cp, csprintf("%s.tickEvent", section));
}

void
AtomicSimpleCPU::switchOut(Sampler *s)
{
    sampler = s;
    if (status() == Running) {
        _status = SwitchedOut;

        tickEvent.squash();
    }
    sampler->signalSwitched();
}


void
AtomicSimpleCPU::takeOverFrom(BaseCPU *oldCPU)
{
    BaseCPU::takeOverFrom(oldCPU);

    assert(!tickEvent.scheduled());

    // if any of this CPU's ExecContexts are active, mark the CPU as
    // running and schedule its tick event.
    for (int i = 0; i < execContexts.size(); ++i) {
        ExecContext *xc = execContexts[i];
        if (xc->status() == ExecContext::Active && _status != Running) {
            _status = Running;
            tickEvent.schedule(curTick);
            break;
        }
    }
}


void
AtomicSimpleCPU::activateContext(int thread_num, int delay)
{
    assert(thread_num == 0);
    assert(cpuXC);

    assert(_status == Idle);
    assert(!tickEvent.scheduled());

    notIdleFraction++;
    tickEvent.schedule(curTick + cycles(delay));
    _status = Running;
}


void
AtomicSimpleCPU::suspendContext(int thread_num)
{
    assert(thread_num == 0);
    assert(cpuXC);

    assert(_status == Running);

    // tick event may not be scheduled if this gets called from inside
    // an instruction's execution, e.g. "quiesce"
    if (tickEvent.scheduled())
        tickEvent.deschedule();

    notIdleFraction--;
    _status = Idle;
}


template <class T>
Fault
AtomicSimpleCPU::read(Addr addr, T &data, unsigned flags)
{
    data_read_req->setVaddr(addr);
    data_read_req->setSize(sizeof(T));
    data_read_req->setFlags(flags);
    data_read_req->setTime(curTick);

    if (traceData) {
        traceData->setAddr(addr);
    }

    // translate to physical address
    Fault fault = cpuXC->translateDataReadReq(data_read_req);

    // Now do the access.
    if (fault == NoFault) {
        data_read_pkt->reset();
        data_read_pkt->reinitFromRequest();

        dcache_complete = dcachePort.sendAtomic(data_read_pkt);
        dcache_access = true;

        assert(data_read_pkt->result == Packet::Success);
        data = data_read_pkt->get<T>();

    }

    // This will need a new way to tell if it has a dcache attached.
    if (data_read_req->getFlags() & UNCACHEABLE)
        recordEvent("Uncached Read");

    return fault;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

template
Fault
AtomicSimpleCPU::read(Addr addr, uint64_t &data, unsigned flags);

template
Fault
AtomicSimpleCPU::read(Addr addr, uint32_t &data, unsigned flags);

template
Fault
AtomicSimpleCPU::read(Addr addr, uint16_t &data, unsigned flags);

template
Fault
AtomicSimpleCPU::read(Addr addr, uint8_t &data, unsigned flags);

#endif //DOXYGEN_SHOULD_SKIP_THIS

template<>
Fault
AtomicSimpleCPU::read(Addr addr, double &data, unsigned flags)
{
    return read(addr, *(uint64_t*)&data, flags);
}

template<>
Fault
AtomicSimpleCPU::read(Addr addr, float &data, unsigned flags)
{
    return read(addr, *(uint32_t*)&data, flags);
}


template<>
Fault
AtomicSimpleCPU::read(Addr addr, int32_t &data, unsigned flags)
{
    return read(addr, (uint32_t&)data, flags);
}


template <class T>
Fault
AtomicSimpleCPU::write(T data, Addr addr, unsigned flags, uint64_t *res)
{
    data_write_req->setVaddr(addr);
    data_write_req->setTime(curTick);
    data_write_req->setSize(sizeof(T));
    data_write_req->setFlags(flags);

    if (traceData) {
        traceData->setAddr(addr);
    }

    // translate to physical address
    Fault fault = cpuXC->translateDataWriteReq(data_write_req);

    // Now do the access.
    if (fault == NoFault) {
        data_write_pkt->reset();
        data = htog(data);
        data_write_pkt->dataStatic(&data);
        data_write_pkt->reinitFromRequest();

        dcache_complete = dcachePort.sendAtomic(data_write_pkt);
        dcache_access = true;

        assert(data_write_pkt->result == Packet::Success);

        if (res && data_write_req->getFlags() & LOCKED) {
            *res = data_write_req->getScResult();
        }
    }

    // This will need a new way to tell if it's hooked up to a cache or not.
    if (data_write_req->getFlags() & UNCACHEABLE)
        recordEvent("Uncached Write");

    // If the write needs to have a fault on the access, consider calling
    // changeStatus() and changing it to "bad addr write" or something.
    return fault;
}


#ifndef DOXYGEN_SHOULD_SKIP_THIS
template
Fault
AtomicSimpleCPU::write(uint64_t data, Addr addr,
                       unsigned flags, uint64_t *res);

template
Fault
AtomicSimpleCPU::write(uint32_t data, Addr addr,
                       unsigned flags, uint64_t *res);

template
Fault
AtomicSimpleCPU::write(uint16_t data, Addr addr,
                       unsigned flags, uint64_t *res);

template
Fault
AtomicSimpleCPU::write(uint8_t data, Addr addr,
                       unsigned flags, uint64_t *res);

#endif //DOXYGEN_SHOULD_SKIP_THIS

template<>
Fault
AtomicSimpleCPU::write(double data, Addr addr, unsigned flags, uint64_t *res)
{
    return write(*(uint64_t*)&data, addr, flags, res);
}

template<>
Fault
AtomicSimpleCPU::write(float data, Addr addr, unsigned flags, uint64_t *res)
{
    return write(*(uint32_t*)&data, addr, flags, res);
}


template<>
Fault
AtomicSimpleCPU::write(int32_t data, Addr addr, unsigned flags, uint64_t *res)
{
    return write((uint32_t)data, addr, flags, res);
}


void
AtomicSimpleCPU::tick()
{
    Tick latency = cycles(1); // instruction takes one cycle by default

    for (int i = 0; i < width; ++i) {
        numCycles++;

        checkForInterrupts();

        ifetch_req->resetMin();
        ifetch_pkt->reset();
        Fault fault = setupFetchPacket(ifetch_pkt);

        if (fault == NoFault) {
            Tick icache_complete = icachePort.sendAtomic(ifetch_pkt);
            // ifetch_req is initialized to read the instruction directly
            // into the CPU object's inst field.

            dcache_access = false; // assume no dcache access
            preExecute();
            fault = curStaticInst->execute(this, traceData);
            postExecute();

            if (simulate_stalls) {
                // This calculation assumes that the icache and dcache
                // access latencies are always a multiple of the CPU's
                // cycle time.  If not, the next tick event may get
                // scheduled at a non-integer multiple of the CPU
                // cycle time.
                Tick icache_stall = icache_complete - curTick - cycles(1);
                Tick dcache_stall =
                    dcache_access ? dcache_complete - curTick - cycles(1) : 0;
                latency += icache_stall + dcache_stall;
            }

        }

        advancePC(fault);
    }

    if (_status != Idle)
        tickEvent.schedule(curTick + latency);
}


////////////////////////////////////////////////////////////////////////
//
//  AtomicSimpleCPU Simulation Object
//
BEGIN_DECLARE_SIM_OBJECT_PARAMS(AtomicSimpleCPU)

    Param<Counter> max_insts_any_thread;
    Param<Counter> max_insts_all_threads;
    Param<Counter> max_loads_any_thread;
    Param<Counter> max_loads_all_threads;
    SimObjectParam<MemObject *> mem;

#if FULL_SYSTEM
    SimObjectParam<AlphaITB *> itb;
    SimObjectParam<AlphaDTB *> dtb;
    SimObjectParam<System *> system;
    Param<int> cpu_id;
    Param<Tick> profile;
#else
    SimObjectParam<Process *> workload;
#endif // FULL_SYSTEM

    Param<int> clock;

    Param<bool> defer_registration;
    Param<int> width;
    Param<bool> function_trace;
    Param<Tick> function_trace_start;
    Param<bool> simulate_stalls;

END_DECLARE_SIM_OBJECT_PARAMS(AtomicSimpleCPU)

BEGIN_INIT_SIM_OBJECT_PARAMS(AtomicSimpleCPU)

    INIT_PARAM(max_insts_any_thread,
               "terminate when any thread reaches this inst count"),
    INIT_PARAM(max_insts_all_threads,
               "terminate when all threads have reached this inst count"),
    INIT_PARAM(max_loads_any_thread,
               "terminate when any thread reaches this load count"),
    INIT_PARAM(max_loads_all_threads,
               "terminate when all threads have reached this load count"),
    INIT_PARAM(mem, "memory"),

#if FULL_SYSTEM
    INIT_PARAM(itb, "Instruction TLB"),
    INIT_PARAM(dtb, "Data TLB"),
    INIT_PARAM(system, "system object"),
    INIT_PARAM(cpu_id, "processor ID"),
    INIT_PARAM(profile, ""),
#else
    INIT_PARAM(workload, "processes to run"),
#endif // FULL_SYSTEM

    INIT_PARAM(clock, "clock speed"),
    INIT_PARAM(defer_registration, "defer system registration (for sampling)"),
    INIT_PARAM(width, "cpu width"),
    INIT_PARAM(function_trace, "Enable function trace"),
    INIT_PARAM(function_trace_start, "Cycle to start function trace"),
    INIT_PARAM(simulate_stalls, "Simulate cache stall cycles")

END_INIT_SIM_OBJECT_PARAMS(AtomicSimpleCPU)


CREATE_SIM_OBJECT(AtomicSimpleCPU)
{
    AtomicSimpleCPU::Params *params = new AtomicSimpleCPU::Params();
    params->name = getInstanceName();
    params->numberOfThreads = 1;
    params->max_insts_any_thread = max_insts_any_thread;
    params->max_insts_all_threads = max_insts_all_threads;
    params->max_loads_any_thread = max_loads_any_thread;
    params->max_loads_all_threads = max_loads_all_threads;
    params->deferRegistration = defer_registration;
    params->clock = clock;
    params->functionTrace = function_trace;
    params->functionTraceStart = function_trace_start;
    params->width = width;
    params->simulate_stalls = simulate_stalls;
    params->mem = mem;

#if FULL_SYSTEM
    params->itb = itb;
    params->dtb = dtb;
    params->system = system;
    params->cpu_id = cpu_id;
    params->profile = profile;
#else
    params->process = workload;
#endif

    AtomicSimpleCPU *cpu = new AtomicSimpleCPU(params);
    return cpu;
}

REGISTER_SIM_OBJECT("AtomicSimpleCPU", AtomicSimpleCPU)

