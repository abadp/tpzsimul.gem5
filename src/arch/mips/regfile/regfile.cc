/*
 * Copyright (c) 2003-2005 The Regents of The University of Michigan
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
 *          Korey Sewell
 */

#include "arch/mips/regfile/regfile.hh"
#include "sim/serialize.hh"

using namespace std;

namespace MipsISA
{

void
RegFile::clear()
{
    intRegFile.clear();
    floatRegFile.clear();
    miscRegFile.clear();
}

void
RegFile::reset(std::string core_name, unsigned num_threads, unsigned num_vpes, BaseCPU *_cpu)
{
    bzero(&intRegFile, sizeof(intRegFile));
    bzero(&floatRegFile, sizeof(floatRegFile));
    miscRegFile.reset(core_name, num_threads, num_vpes, _cpu);
}

IntReg
RegFile::readIntReg(int intReg)
{
    return intRegFile.readReg(intReg);
}

Fault
RegFile::setIntReg(int intReg, const IntReg &val)
{
    return intRegFile.setReg(intReg, val);
}

MiscReg
RegFile::readMiscRegNoEffect(int miscReg, unsigned tid)
{
    return miscRegFile.readRegNoEffect(miscReg, tid);
}

MiscReg
RegFile::readMiscReg(int miscReg, ThreadContext *tc,
                    unsigned tid)
{
    return miscRegFile.readReg(miscReg, tc, tid);
}

void
RegFile::setMiscRegNoEffect(int miscReg, const MiscReg &val, unsigned tid)
{
    miscRegFile.setRegNoEffect(miscReg, val, tid);
}

void
RegFile::setMiscReg(int miscReg, const MiscReg &val,
                ThreadContext * tc, unsigned tid)
{
    miscRegFile.setReg(miscReg, val, tc, tid);
}

FloatRegVal
RegFile::readFloatReg(int floatReg)
{
    return floatRegFile.readReg(floatReg,SingleWidth);
}

FloatRegVal
RegFile::readFloatReg(int floatReg, int width)
{
    return floatRegFile.readReg(floatReg,width);
}

FloatRegBits
RegFile::readFloatRegBits(int floatReg)
{
    return floatRegFile.readRegBits(floatReg,SingleWidth);
}

FloatRegBits
RegFile::readFloatRegBits(int floatReg, int width)
{
    return floatRegFile.readRegBits(floatReg,width);
}

Fault
RegFile::setFloatReg(int floatReg, const FloatRegVal &val)
{
    return floatRegFile.setReg(floatReg, val, SingleWidth);
}

Fault
RegFile::setFloatReg(int floatReg, const FloatRegVal &val, int width)
{
    return floatRegFile.setReg(floatReg, val, width);
}

Fault
RegFile::setFloatRegBits(int floatReg, const FloatRegBits &val)
{
    return floatRegFile.setRegBits(floatReg, val, SingleWidth);
}

Fault
RegFile::setFloatRegBits(int floatReg, const FloatRegBits &val, int width)
{
    return floatRegFile.setRegBits(floatReg, val, width);
}

void
RegFile::setShadowSet(int css){
    intRegFile.setShadowSet(css);
}

int
RegFile::instAsid()
{
    return miscRegFile.getInstAsid();
}

int
RegFile::dataAsid()
{
    return miscRegFile.getDataAsid();
}

Addr
RegFile::readPC()
{
    return pc;
}

void
RegFile::setPC(Addr val)
{
    pc = val;
}

Addr
RegFile::readNextPC()
{
    return npc;
}

void
RegFile::setNextPC(Addr val)
{
    npc = val;
}

Addr
RegFile::readNextNPC()
{
    return nnpc;
}

void
RegFile::setNextNPC(Addr val)
{
    nnpc = val;
}

void
RegFile::serialize(std::ostream &os)
{
    intRegFile.serialize(os);
    //SERIALIZE_ARRAY(floatRegFile, NumFloatRegs);
    //SERIALZE_ARRAY(miscRegFile);
    //SERIALIZE_SCALAR(miscRegs.fpcr);
    //SERIALIZE_SCALAR(miscRegs.lock_flag);
    //SERIALIZE_SCALAR(miscRegs.lock_addr);
    SERIALIZE_SCALAR(pc);
    SERIALIZE_SCALAR(npc);
    SERIALIZE_SCALAR(nnpc);
}

void
RegFile::unserialize(Checkpoint *cp, const std::string &section)
{
    intRegFile.unserialize(cp, section);
    //UNSERIALIZE_ARRAY(floatRegFile);
    //UNSERIALZE_ARRAY(miscRegFile);
    //UNSERIALIZE_SCALAR(miscRegs.fpcr);
    //UNSERIALIZE_SCALAR(miscRegs.lock_flag);
    //UNSERIALIZE_SCALAR(miscRegs.lock_addr);
    UNSERIALIZE_SCALAR(pc);
    UNSERIALIZE_SCALAR(npc);
    UNSERIALIZE_SCALAR(nnpc);

}

} // namespace MipsISA