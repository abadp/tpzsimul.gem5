# Copyright (c) 2005 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
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
# Authors: Nathan Binkert

import code
import datetime
import os
import socket
import sys

from util import attrdict
import config
import defines
from options import OptionParser
import traceflags

__all__ = [ 'options', 'arguments', 'main' ]

def print_list(items, indent=4):
    line = ' ' * indent
    for i,item in enumerate(items):
        if len(line) + len(item) > 76:
            print line
            line = ' ' * indent

        if i < len(items) - 1:
            line += '%s, ' % item
        else:
            line += item
            print line

usage="%prog [m5 options] script.py [script options]"
version="%prog 2.0"
brief_copyright='''
Copyright (c) 2001-2008
The Regents of The University of Michigan
All Rights Reserved
'''

options = OptionParser(usage=usage, version=version,
                       description=brief_copyright)
add_option = options.add_option
set_group = options.set_group
usage = options.usage

# Help options
add_option('-A', "--authors", action="store_true", default=False,
    help="Show author information")
add_option('-B', "--build-info", action="store_true", default=False,
    help="Show build information")
add_option('-C', "--copyright", action="store_true", default=False,
    help="Show full copyright information")
add_option('-R', "--readme", action="store_true", default=False,
    help="Show the readme")
add_option('-N', "--release-notes", action="store_true", default=False,
    help="Show the release notes")

# Options for configuring the base simulator
add_option('-d', "--outdir", metavar="DIR", default=".",
    help="Set the output directory to DIR [Default: %default]")
add_option('-i', "--interactive", action="store_true", default=False,
    help="Invoke the interactive interpreter after running the script")
add_option("--pdb", action="store_true", default=False,
    help="Invoke the python debugger before running the script")
add_option('-p', "--path", metavar="PATH[:PATH]", action='append', split=':',
    help="Prepend PATH to the system path when invoking the script")
add_option('-q', "--quiet", action="count", default=0,
    help="Reduce verbosity")
add_option('-v', "--verbose", action="count", default=0,
    help="Increase verbosity")

# Statistics options
set_group("Statistics Options")
add_option("--stats-file", metavar="FILE", default="m5stats.txt",
    help="Sets the output file for statistics [Default: %default]")

# Debugging options
set_group("Debugging Options")
add_option("--debug-break", metavar="TIME[,TIME]", action='append', split=',',
    help="Cycle to create a breakpoint")

# Tracing options
set_group("Trace Options")
add_option("--trace-help", action='store_true',
    help="Print help on trace flags")
add_option("--trace-flags", metavar="FLAG[,FLAG]", action='append', split=',',
    help="Sets the flags for tracing (-FLAG disables a flag)")
add_option("--trace-start", metavar="TIME", type='int',
    help="Start tracing at TIME (must be in ticks)")
add_option("--trace-file", metavar="FILE", default="cout",
    help="Sets the output file for tracing [Default: %default]")
add_option("--trace-ignore", metavar="EXPR", action='append', split=':',
    help="Ignore EXPR sim objects")

# Help options
set_group("Help Options")
add_option("--list-sim-objects", action='store_true', default=False,
    help="List all built-in SimObjects, their parameters and default values")

def main():
    import defines
    import event
    import info
    import internal

    # load the options.py config file to allow people to set their own
    # default options
    options_file = config.get('options.py')
    if options_file:
        scope = { 'options' : options }
        execfile(options_file, scope)

    arguments = options.parse_args()

    done = False

    if options.build_info:
        done = True
        print 'Build information:'
        print
        print 'compiled %s' % internal.core.cvar.compileDate;
        print 'started %s' % datetime.datetime.now().ctime()
        print 'executing on %s' % socket.gethostname()
        print 'build options:'
        keys = defines.m5_build_env.keys()
        keys.sort()
        for key in keys:
            val = defines.m5_build_env[key]
            print '    %s = %s' % (key, val)
        print

    if options.copyright:
        done = True
        print info.LICENSE
        print

    if options.authors:
        done = True
        print 'Author information:'
        print
        print info.AUTHORS
        print

    if options.readme:
        done = True
        print 'Readme:'
        print
        print info.README
        print

    if options.release_notes:
        done = True
        print 'Release Notes:'
        print
        print info.RELEASE_NOTES
        print

    if options.trace_help:
        done = True
        print "Base Flags:"
        print_list(traceflags.baseFlags, indent=4)
        print
        print "Compound Flags:"
        for flag in traceflags.compoundFlags:
            if flag == 'All':
                continue
            print "    %s:" % flag
            print_list(traceflags.compoundFlagMap[flag], indent=8)
            print

    if options.list_sim_objects:
        import SimObject
        done = True
        print "SimObjects:"
        objects = SimObject.allClasses.keys()
        objects.sort()
        for name in objects:
            obj = SimObject.allClasses[name]
            print "    %s" % obj
            params = obj._params.keys()
            params.sort()
            for pname in params:
                param = obj._params[pname]
                default = getattr(param, 'default', '')
                print "        %s" % pname
                if default:
                    print "            default: %s" % default
                print "            desc: %s" % param.desc
                print
            print

    if done:
        sys.exit(0)

    # setting verbose and quiet at the same time doesn't make sense
    if options.verbose > 0 and options.quiet > 0:
        options.usage(2)

    verbose = options.verbose - options.quiet
    if options.verbose >= 0:
        print "M5 Simulator System"
        print brief_copyright
        print
        print "M5 compiled %s" % internal.core.cvar.compileDate;
        print "M5 started %s" % datetime.datetime.now().ctime()
        print "M5 executing on %s" % socket.gethostname()

        print "M5 revision %s" % internal.core.cvar.hgRev
        print "M5 commit date %s" % internal.core.cvar.hgDate

        print "command line:",
        for argv in sys.argv:
            print argv,
        print

    # check to make sure we can find the listed script
    if not arguments or not os.path.isfile(arguments[0]):
        if arguments and not os.path.isfile(arguments[0]):
            print "Script %s not found" % arguments[0]

        options.usage(2)

    # tell C++ about output directory
    internal.core.setOutputDir(options.outdir)

    # update the system path with elements from the -p option
    sys.path[0:0] = options.path

    import objects

    # set stats options
    internal.stats.initText(options.stats_file)

    # set debugging options
    for when in options.debug_break:
        internal.debug.schedBreakCycle(int(when))

    on_flags = []
    off_flags = []
    for flag in options.trace_flags:
        off = False
        if flag.startswith('-'):
            flag = flag[1:]
            off = True
        if flag not in traceflags.allFlags:
            print >>sys.stderr, "invalid trace flag '%s'" % flag
            sys.exit(1)

        if off:
            off_flags.append(flag)
        else:
            on_flags.append(flag)

    for flag in on_flags:
        internal.trace.set(flag)

    for flag in off_flags:
        internal.trace.clear(flag)

    if options.trace_start:
        def enable_trace():
            internal.trace.cvar.enabled = True
        event.create(enable_trace, int(options.trace_start))
    else:
        internal.trace.cvar.enabled = True

    internal.trace.output(options.trace_file)

    for ignore in options.trace_ignore:
        internal.trace.ignore(ignore)

    sys.argv = arguments
    sys.path = [ os.path.dirname(sys.argv[0]) ] + sys.path

    scope = { '__file__' : sys.argv[0],
              '__name__' : '__m5_main__' }

    # we want readline if we're doing anything interactive
    if options.interactive or options.pdb:
        exec "import readline" in scope

    # if pdb was requested, execfile the thing under pdb, otherwise,
    # just do the execfile normally
    if options.pdb:
        from pdb import Pdb
        debugger = Pdb()
        debugger.run('execfile("%s")' % sys.argv[0], scope)
    else:
        execfile(sys.argv[0], scope)

    # once the script is done
    if options.interactive:
        interact = code.InteractiveConsole(scope)
        interact.interact("M5 Interactive Console")

if __name__ == '__main__':
    from pprint import pprint

    parse_args()

    print 'opts:'
    pprint(options, indent=4)
    print

    print 'args:'
    pprint(arguments, indent=4)