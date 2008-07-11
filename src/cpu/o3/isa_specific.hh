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
 * Authors: Korey Sewell
 */

#include "cpu/base.hh"

#if THE_ISA == ALPHA_ISA
    #include "cpu/o3/alpha/cpu.hh"
    #include "cpu/o3/alpha/impl.hh"
    #include "cpu/o3/alpha/params.hh"
    #include "cpu/o3/alpha/dyn_inst.hh"
#elif THE_ISA == MIPS_ISA
    #include "cpu/o3/mips/cpu.hh"
    #include "cpu/o3/mips/impl.hh"
    #include "cpu/o3/mips/params.hh"
    #include "cpu/o3/mips/dyn_inst.hh"
#elif THE_ISA == SPARC_ISA
    #include "cpu/o3/sparc/cpu.hh"
    #include "cpu/o3/sparc/impl.hh"
    #include "cpu/o3/sparc/params.hh"
    #include "cpu/o3/sparc/dyn_inst.hh"
#else
    #error "ISA-specific header files O3CPU not defined ISA"
#endif