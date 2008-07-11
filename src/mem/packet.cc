/*
 * Copyright (c) 2006 The Regents of The University of Michigan
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
 * Authors: Ali Saidi
 *          Steve Reinhardt
 */

/**
 * @file
 * Definition of the Packet Class, a packet is a transaction occuring
 * between a single level of the memory heirarchy (ie L1->L2).
 */

#include <iostream>
#include <cstring>
#include "base/cprintf.hh"
#include "base/misc.hh"
#include "base/trace.hh"
#include "mem/packet.hh"

// The one downside to bitsets is that static initializers can get ugly.
#define SET1(a1)                     (1 << (a1))
#define SET2(a1, a2)                 (SET1(a1) | SET1(a2))
#define SET3(a1, a2, a3)             (SET2(a1, a2) | SET1(a3))
#define SET4(a1, a2, a3, a4)         (SET3(a1, a2, a3) | SET1(a4))
#define SET5(a1, a2, a3, a4, a5)     (SET4(a1, a2, a3, a4) | SET1(a5))
#define SET6(a1, a2, a3, a4, a5, a6) (SET5(a1, a2, a3, a4, a5) | SET1(a6))

const MemCmd::CommandInfo
MemCmd::commandInfo[] =
{
    /* InvalidCmd */
    { 0, InvalidCmd, "InvalidCmd" },
    /* ReadReq */
    { SET3(IsRead, IsRequest, NeedsResponse), ReadResp, "ReadReq" },
    /* ReadResp */
    { SET3(IsRead, IsResponse, HasData), InvalidCmd, "ReadResp" },
    /* ReadRespWithInvalidate */
    { SET4(IsRead, IsResponse, HasData, IsInvalidate),
            InvalidCmd, "ReadRespWithInvalidate" },
    /* WriteReq */
    { SET5(IsWrite, NeedsExclusive, IsRequest, NeedsResponse, HasData),
            WriteResp, "WriteReq" },
    /* WriteResp */
    { SET3(IsWrite, NeedsExclusive, IsResponse), InvalidCmd, "WriteResp" },
    /* Writeback */
    { SET4(IsWrite, NeedsExclusive, IsRequest, HasData),
            InvalidCmd, "Writeback" },
    /* SoftPFReq */
    { SET4(IsRead, IsRequest, IsSWPrefetch, NeedsResponse),
            SoftPFResp, "SoftPFReq" },
    /* HardPFReq */
    { SET4(IsRead, IsRequest, IsHWPrefetch, NeedsResponse),
            HardPFResp, "HardPFReq" },
    /* SoftPFResp */
    { SET4(IsRead, IsResponse, IsSWPrefetch, HasData),
            InvalidCmd, "SoftPFResp" },
    /* HardPFResp */
    { SET4(IsRead, IsResponse, IsHWPrefetch, HasData),
            InvalidCmd, "HardPFResp" },
    /* WriteInvalidateReq */
    { SET6(IsWrite, NeedsExclusive, IsInvalidate,
           IsRequest, HasData, NeedsResponse),
            WriteInvalidateResp, "WriteInvalidateReq" },
    /* WriteInvalidateResp */
    { SET3(IsWrite, NeedsExclusive, IsResponse),
            InvalidCmd, "WriteInvalidateResp" },
    /* UpgradeReq */
    { SET4(IsInvalidate, NeedsExclusive, IsRequest, NeedsResponse),
            UpgradeResp, "UpgradeReq" },
    /* UpgradeResp */
    { SET2(NeedsExclusive, IsResponse),
            InvalidCmd, "UpgradeResp" },
    /* ReadExReq */
    { SET5(IsRead, NeedsExclusive, IsInvalidate, IsRequest, NeedsResponse),
            ReadExResp, "ReadExReq" },
    /* ReadExResp */
    { SET4(IsRead, NeedsExclusive, IsResponse, HasData),
            InvalidCmd, "ReadExResp" },
    /* LoadLockedReq */
    { SET4(IsRead, IsLocked, IsRequest, NeedsResponse),
            LoadLockedResp, "LoadLockedReq" },
    /* LoadLockedResp */
    { SET4(IsRead, IsLocked, IsResponse, HasData),
            InvalidCmd, "LoadLockedResp" },
    /* StoreCondReq */
    { SET6(IsWrite, NeedsExclusive, IsLocked,
           IsRequest, NeedsResponse, HasData),
            StoreCondResp, "StoreCondReq" },
    /* StoreCondResp */
    { SET4(IsWrite, NeedsExclusive, IsLocked, IsResponse),
            InvalidCmd, "StoreCondResp" },
    /* SwapReq -- for Swap ldstub type operations */
    { SET6(IsRead, IsWrite, NeedsExclusive, IsRequest, HasData, NeedsResponse),
        SwapResp, "SwapReq" },
    /* SwapResp -- for Swap ldstub type operations */
    { SET5(IsRead, IsWrite, NeedsExclusive, IsResponse, HasData),
            InvalidCmd, "SwapResp" },
    /* NetworkNackError  -- nacked at network layer (not by protocol) */
    { SET2(IsResponse, IsError), InvalidCmd, "NetworkNackError" },
    /* InvalidDestError  -- packet dest field invalid */
    { SET2(IsResponse, IsError), InvalidCmd, "InvalidDestError" },
    /* BadAddressError   -- memory address invalid */
    { SET2(IsResponse, IsError), InvalidCmd, "BadAddressError" },
    /* PrintReq */
    { SET2(IsRequest, IsPrint), InvalidCmd, "PrintReq" }
};


/** delete the data pointed to in the data pointer. Ok to call to matter how
 * data was allocted. */
void
Packet::deleteData()
{
    assert(staticData || dynamicData);
    if (staticData)
        return;

    if (arrayData)
        delete [] data;
    else
        delete data;
}

/** If there isn't data in the packet, allocate some. */
void
Packet::allocate()
{
    if (data)
        return;
    assert(!staticData);
    dynamicData = true;
    arrayData = true;
    data = new uint8_t[getSize()];
}


bool
Packet::checkFunctional(Printable *obj, Addr addr, int size, uint8_t *data)
{
    Addr func_start = getAddr();
    Addr func_end   = getAddr() + getSize() - 1;
    Addr val_start  = addr;
    Addr val_end    = val_start + size - 1;

    if (func_start > val_end || val_start > func_end) {
        // no intersection
        return false;
    }

    // check print first since it doesn't require data
    if (isPrint()) {
        dynamic_cast<PrintReqState*>(senderState)->printObj(obj);
        return false;
    }

    // if there's no data, there's no need to look further
    if (!data) {
        return false;
    }

    // offset of functional request into supplied value (could be
    // negative if partial overlap)
    int offset = func_start - val_start;

    if (isRead()) {
        if (func_start >= val_start && func_end <= val_end) {
            allocate();
            std::memcpy(getPtr<uint8_t>(), data + offset, getSize());
            makeResponse();
            return true;
        } else {
            // In this case the timing packet only partially satisfies
            // the request, so we would need more information to make
            // this work.  Like bytes valid in the packet or
            // something, so the request could continue and get this
            // bit of possibly newer data along with the older data
            // not written to yet.
            panic("Memory value only partially satisfies the functional "
                  "request. Now what?");
        }
    } else if (isWrite()) {
        if (offset >= 0) {
            std::memcpy(data + offset, getPtr<uint8_t>(),
                        (std::min(func_end, val_end) - func_start) + 1);
        } else { // val_start > func_start
            std::memcpy(data, getPtr<uint8_t>() - offset,
                        (std::min(func_end, val_end) - val_start) + 1);
        }
    } else {
        panic("Don't know how to handle command %s\n", cmdString());
    }

    // keep going with request by default
    return false;
}


void
Packet::print(std::ostream &o, const int verbosity,
              const std::string &prefix) const
{
    ccprintf(o, "%s[%x:%x] %s\n", prefix,
             getAddr(), getAddr() + getSize() - 1, cmdString());
}


Packet::PrintReqState::PrintReqState(std::ostream &_os, int _verbosity)
    : curPrefixPtr(new std::string("")), os(_os), verbosity(_verbosity)
{
    labelStack.push_back(LabelStackEntry("", curPrefixPtr));
}


Packet::PrintReqState::~PrintReqState()
{
    labelStack.pop_back();
    assert(labelStack.empty());
    delete curPrefixPtr;
}


Packet::PrintReqState::
LabelStackEntry::LabelStackEntry(const std::string &_label,
                                 std::string *_prefix)
    : label(_label), prefix(_prefix), labelPrinted(false)
{
}


void
Packet::PrintReqState::pushLabel(const std::string &lbl,
                                 const std::string &prefix)
{
    labelStack.push_back(LabelStackEntry(lbl, curPrefixPtr));
    curPrefixPtr = new std::string(*curPrefixPtr);
    *curPrefixPtr += prefix;
}

void
Packet::PrintReqState::popLabel()
{
    delete curPrefixPtr;
    curPrefixPtr = labelStack.back().prefix;
    labelStack.pop_back();
    assert(!labelStack.empty());
}

void
Packet::PrintReqState::printLabels()
{
    if (!labelStack.back().labelPrinted) {
        LabelStack::iterator i = labelStack.begin();
        LabelStack::iterator end = labelStack.end();
        while (i != end) {
            if (!i->labelPrinted) {
                ccprintf(os, "%s%s\n", *(i->prefix), i->label);
                i->labelPrinted = true;
            }
            i++;
        }
    }
}


void
Packet::PrintReqState::printObj(Printable *obj)
{
    printLabels();
    obj->print(os, verbosity, curPrefix());
}