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

#include <c_asm.h>

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m5op.h"

char *progname;

void
usage()
{
    char *name = basename(progname);
    printf("usage: %s ivlb <interval>\n"
           "       %s ivle <interval>\n"
           "       %s initparam\n"
           "       %s sw99param\n"
           "       %s resetstats\n"
           "       %s exit\n", name, name, name, name, name, name);
    exit(1);
}

int
main(int argc, char *argv[])
{
    int start;
    int interval;
    unsigned long long param;

    progname = argv[0];
    if (argc < 2)
        usage();

    if (strncmp(argv[1], "ivlb", 5) == 0) {
        if (argc != 3) usage();
        ivlb((unsigned long)atoi(argv[2]));
    } else if (strncmp(argv[1], "ivle", 5) == 0) {
        if (argc != 3) usage();
        ivle((unsigned long)atoi(argv[2]));
    } else if (strncmp(argv[1], "exit", 5) == 0) {
        if (argc != 2) usage();
        m5exit();
    } else if (strncmp(argv[1], "initparam", 10) == 0) {
        if (argc != 2) usage();
        printf("%d", initparam());
    } else if (strncmp(argv[1], "sw99param", 10) == 0) {
        if (argc != 2) usage();

        param = initparam();
        // run-time, rampup-time, rampdown-time, warmup-time, connections
        printf("%d %d %d %d %d", (param >> 48) & 0xfff,
               (param >> 36) & 0xfff, (param >> 24) & 0xfff,
               (param >> 12) & 0xfff, (param >> 0) & 0xfff);
    } else if (strncmp(argv[1], "resetstats", 11) == 0) {
        if (argc != 2) usage();
        resetstats();
    }

    return 0;
}
