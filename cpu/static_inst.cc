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
 */

#include <iostream>
#include "cpu/static_inst.hh"
#include "sim/universe.hh"

template <class ISA>
StaticInstPtr<ISA> StaticInst<ISA>::nullStaticInstPtr;

template <class ISA>
typename StaticInst<ISA>::DecodeCache StaticInst<ISA>::decodeCache;

// Define the decode cache hash map.
template StaticInst<AlphaISA>::DecodeCache
StaticInst<AlphaISA>::decodeCache;

template <class ISA>
void
StaticInst<ISA>::dumpDecodeCacheStats()
{
    using namespace std;

    cerr << "Decode hash table stats @ " << curTick << ":" << endl;
    cerr << "\tnum entries = " << decodeCache.size() << endl;
    cerr << "\tnum buckets = " << decodeCache.bucket_count() << endl;
    vector<int> hist(100, 0);
    int max_hist = 0;
    for (int i = 0; i < decodeCache.bucket_count(); ++i) {
        int count = decodeCache.elems_in_bucket(i);
        if (count > max_hist)
            max_hist = count;
        hist[count]++;
    }
    for (int i = 0; i <= max_hist; ++i) {
        cerr << "\tbuckets of size " << i << " = " << hist[i] << endl;
    }
}


template StaticInstPtr<AlphaISA>
StaticInst<AlphaISA>::nullStaticInstPtr;

template <class ISA>
bool
StaticInst<ISA>::hasBranchTarget(Addr pc, ExecContext *xc, Addr &tgt)
{
    if (isDirectCtrl()) {
        tgt = branchTarget(pc);
        return true;
    }

    if (isIndirectCtrl()) {
        tgt = branchTarget(xc);
        return true;
    }

    return false;
}


// force instantiation of template function(s) above
template StaticInst<AlphaISA>;
