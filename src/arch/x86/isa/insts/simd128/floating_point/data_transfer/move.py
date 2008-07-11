# Copyright (c) 2007 The Hewlett-Packard Development Company
# All rights reserved.
#
# Redistribution and use of this software in source and binary forms,
# with or without modification, are permitted provided that the
# following conditions are met:
#
# The software must be used only for Non-Commercial Use which means any
# use which is NOT directed to receiving any direct monetary
# compensation for, or commercial advantage from such use.  Illustrative
# examples of non-commercial use are academic research, personal study,
# teaching, education and corporate research & development.
# Illustrative examples of commercial use are distributing products for
# commercial advantage and providing services using the software for
# commercial advantage.
#
# If you wish to use this software or functionality therein that may be
# covered by patents for commercial use, please contact:
#     Director of Intellectual Property Licensing
#     Office of Strategy and Technology
#     Hewlett-Packard Company
#     1501 Page Mill Road
#     Palo Alto, California  94304
#
# Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.  Redistributions
# in binary form must reproduce the above copyright notice, this list of
# conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.  Neither the name of
# the COPYRIGHT HOLDER(s), HEWLETT-PACKARD COMPANY, nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.  No right of
# sublicense is granted herewith.  Derivatives of the software and
# output created using the software may be prepared, but only for
# Non-Commercial Uses.  Derivatives of the software may be shared with
# others provided: (i) the others agree to abide by the list of
# conditions herein which includes the Non-Commercial Use restrictions;
# and (ii) such Derivatives of the software include the above copyright
# notice to acknowledge the contribution from this software where
# applicable, this list of conditions and the disclaimer below.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Gabe Black

microcode = '''
def macroop MOVAPS_R_M {
    # Check low address.
    ldfp xmmh, seg, sib, "DISPLACEMENT + 8", dataSize=8
    ldfp xmml, seg, sib, disp, dataSize=8
};

def macroop MOVAPS_R_P {
    rdip t7
    # Check low address.
    ldfp xmmh, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    ldfp xmml, seg, riprel, disp, dataSize=8
};

def macroop MOVAPS_M_R {
    # Check low address.
    stfp xmmh, seg, sib, "DISPLACEMENT + 8", dataSize=8
    stfp xmml, seg, sib, disp, dataSize=8
};

def macroop MOVAPS_P_R {
    rdip t7
    # Check low address.
    stfp xmmh, seg, riprel, "DISPLACEMENT + 8", dataSize=8
    stfp xmml, seg, riprel, disp, dataSize=8
};

def macroop MOVAPS_R_R {
    # Check low address.
    movfp xmml, xmmlm, dataSize=8
    movfp xmmh, xmmhm, dataSize=8
};

# MOVAPD
# MOVUPS
# MOVUPD
# MOVHPS
# MOVHPD
# MOVLPS

def macroop MOVLPD_R_M {
    ldfp xmml, seg, sib, disp, dataSize=8
};

def macroop MOVLPD_R_P {
    rdip t7
    ldfp xmml, seg, riprel, disp, dataSize=8
};

def macroop MOVLPD_M_R {
    stfp xmml, seg, sib, disp, dataSize=8
};

def macroop MOVLPD_P_R {
    rdip t7
    stfp xmml, seg, riprel, disp, dataSize=8
};

def macroop MOVLPD_R_R {
    movfp xmml, xmmlm, dataSize=8
};

# MOVHLPS
# MOVLHPS
# MOVSS

def macroop MOVSD_R_M {
    # Zero xmmh
    ldfp xmml, seg, sib, disp, dataSize=8
};

def macroop MOVSD_R_P {
    rdip t7
    # Zero xmmh
    ldfp xmml, seg, riprel, disp, dataSize=8
};

def macroop MOVSD_M_R {
    stfp xmml, seg, sib, disp, dataSize=8
};

def macroop MOVSD_P_R {
    rdip t7
    stfp xmml, seg, riprel, disp, dataSize=8
};

def macroop MOVSD_R_R {
    movfp xmml, xmmlm, dataSize=8
};
'''