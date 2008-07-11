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
 * Authors: Nathan Binkert
 *          Steve Reinhardt
 */

%module core

%{
#include "python/swig/pyobject.hh"

#include "sim/core.hh"
#include "sim/host.hh"
#include "sim/startup.hh"

extern const char *compileDate;
std::vector<std::string> compileFlags();
extern const char *hgRev;
extern const char *hgDate;
%}

%include "stdint.i"
%include "std_string.i"
%include "std_vector.i"
%include "sim/host.hh"

void setOutputDir(const std::string &dir);
void setOutputFile(const std::string &file);
void SimStartup();
void doExitCleanup();

%immutable compileDate;
char *compileDate;

namespace std { %template(StringVector) vector<string>; }
std::vector<std::string> compileFlags();

char *hgRev;
char *hgDate;

void setClockFrequency(Tick ticksPerSecond);

%immutable curTick;
Tick curTick;

void serializeAll(const std::string &cpt_dir);
void unserializeAll(const std::string &cpt_dir);

void initAll();
void regAllStats();

%wrapper %{
// fix up module name to reflect the fact that it's inside the m5 package
#undef SWIG_name
#define SWIG_name "m5.internal._core"
%}