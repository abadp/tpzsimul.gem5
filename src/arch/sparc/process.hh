/*
 * Copyright (c) 2003-2004 The Regents of The University of Michigan
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
 * Authors: Gabe Black
 *          Ali Saidi
 */

#ifndef __SPARC_PROCESS_HH__
#define __SPARC_PROCESS_HH__

#include <string>
#include <vector>
#include "sim/byteswap.hh"
#include "sim/process.hh"

class ObjectFile;
class System;

class SparcLiveProcess : public LiveProcess
{
  protected:

    const Addr StackBias;

    //The locations of the fill and spill handlers
    Addr fillStart, spillStart;

    SparcLiveProcess(LiveProcessParams * params,
            ObjectFile *objFile, Addr _StackBias);

    void startup();

    template<class IntType>
    void argsInit(int pageSize);

  public:

    //Handles traps which request services from the operating system
    virtual void handleTrap(int trapNum, ThreadContext *tc);

    Addr readFillStart()
    { return fillStart; }

    Addr readSpillStart()
    { return spillStart; }

    virtual void flushWindows(ThreadContext *tc) = 0;
};

template<class IntType>
struct M5_auxv_t
{
    IntType a_type;
    union {
        IntType a_val;
        IntType a_ptr;
        IntType a_fcn;
    };

    M5_auxv_t()
    {}

    M5_auxv_t(IntType type, IntType val)
    {
        a_type = SparcISA::htog(type);
        a_val = SparcISA::htog(val);
    }
};

class Sparc32LiveProcess : public SparcLiveProcess
{
  protected:

    Sparc32LiveProcess(LiveProcessParams * params, ObjectFile *objFile) :
            SparcLiveProcess(params, objFile, 0)
    {
        // Set up stack. On SPARC Linux, stack goes from the top of memory
        // downward, less the hole for the kernel address space.
        stack_base = (Addr)0xf0000000ULL;

        // Set up region for mmaps.
        mmap_start = mmap_end = 0x70000000;
    }

    void startup();

  public:

    void argsInit(int intSize, int pageSize);

    void flushWindows(ThreadContext *tc);
};

class Sparc64LiveProcess : public SparcLiveProcess
{
  protected:

    Sparc64LiveProcess(LiveProcessParams * params, ObjectFile *objFile) :
            SparcLiveProcess(params, objFile, 2047)
    {
        // Set up stack. On SPARC Linux, stack goes from the top of memory
        // downward, less the hole for the kernel address space.
        stack_base = (Addr)0x80000000000ULL;

        // Set up region for mmaps.  Tru64 seems to start just above 0 and
        // grow up from there.
        mmap_start = mmap_end = 0xfffff80000000000ULL;
    }

    void startup();

  public:

    void argsInit(int intSize, int pageSize);

    void flushWindows(ThreadContext *tc);
};

#endif // __SPARC_PROCESS_HH__