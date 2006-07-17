/*
 * Copyright (c) 2003 The Regents of The University of Michigan
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
 *
 * Authors: Steve Reinhardt
 *          Ron Dreslinski
 */

/**
 * @file
 * Definitions of page table.
 */
#include <string>
#include <map>
#include <fstream>

#include "arch/faults.hh"
#include "base/bitfield.hh"
#include "base/intmath.hh"
#include "base/trace.hh"
#include "mem/page_table.hh"
#include "sim/builder.hh"
#include "sim/sim_object.hh"
#include "sim/system.hh"

using namespace std;
using namespace TheISA;

PageTable::PageTable(System *_system, Addr _pageSize)
    : pageSize(_pageSize), offsetMask(mask(floorLog2(_pageSize))),
      system(_system)
{
    assert(isPowerOf2(pageSize));
    pTableCache[0].vaddr = 0;
    pTableCache[1].vaddr = 0;
    pTableCache[2].vaddr = 0;
}

PageTable::~PageTable()
{
}

Fault
PageTable::page_check(Addr addr, int size) const
{
    if (size < sizeof(uint64_t)) {
        if (!isPowerOf2(size)) {
            panic("Invalid request size!\n");
            return genMachineCheckFault();
        }

        if ((size - 1) & addr)
            return genAlignmentFault();
    }
    else {
        if ((addr & (VMPageSize - 1)) + size > VMPageSize) {
            panic("Invalid request size!\n");
            return genMachineCheckFault();
        }

        if ((sizeof(uint64_t) - 1) & addr)
            return genAlignmentFault();
    }

    return NoFault;
}




void
PageTable::allocate(Addr vaddr, int size)
{
    // starting address must be page aligned
    assert(pageOffset(vaddr) == 0);

    for (; size > 0; size -= pageSize, vaddr += pageSize) {
        m5::hash_map<Addr,Addr>::iterator iter = pTable.find(vaddr);

        if (iter != pTable.end()) {
            // already mapped
            fatal("PageTable::allocate: address 0x%x already mapped", vaddr);
        }

        pTable[vaddr] = system->new_page();
        pTableCache[2].paddr = pTableCache[1].paddr;
        pTableCache[2].vaddr = pTableCache[1].vaddr;
        pTableCache[1].paddr = pTableCache[0].paddr;
        pTableCache[1].vaddr = pTableCache[0].vaddr;
        pTableCache[0].paddr = pTable[vaddr];
        pTableCache[0].vaddr = vaddr;
    }
}



bool
PageTable::translate(Addr vaddr, Addr &paddr)
{
    Addr page_addr = pageAlign(vaddr);
    paddr = 0;

    if (pTableCache[0].vaddr == vaddr) {
        paddr = pTableCache[0].paddr;
        return true;
    }
    if (pTableCache[1].vaddr == vaddr) {
        paddr = pTableCache[1].paddr;
        return true;
    }
    if (pTableCache[2].vaddr == vaddr) {
        paddr = pTableCache[2].paddr;
        return true;
    }

    m5::hash_map<Addr,Addr>::iterator iter = pTable.find(page_addr);

    if (iter == pTable.end()) {
        return false;
    }

    paddr = iter->second + pageOffset(vaddr);
    return true;
}


Fault
PageTable::translate(RequestPtr &req)
{
    Addr paddr;
    assert(pageAlign(req->getVaddr() + req->getSize() - 1)
           == pageAlign(req->getVaddr()));
    if (!translate(req->getVaddr(), paddr)) {
        return genPageTableFault(req->getVaddr());
    }
    req->setPaddr(paddr);
    return page_check(req->getPaddr(), req->getSize());
}
