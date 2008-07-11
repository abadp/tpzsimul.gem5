# Copyright (c) 2004-2006 The Regents of The University of Michigan
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

import imp
import py_compile
import sys
import zipfile

from os.path import basename, exists, isdir, join

class DictImporter(object):
    '''This importer takes a dictionary of arbitrary module names that
    map to arbitrary filenames.'''
    def __init__(self, modules, build_env):
        self.modules = modules
        self.installed = set()
        self.build_env = build_env

    def __del__(self):
        self.unload()

    def unload(self):
        import sys
        for module in self.installed:
            del sys.modules[module]
        self.installed = set()

    def find_module(self, fullname, path):
        if fullname == '__scons':
            return self

        if fullname == 'm5.objects':
            return self

        if fullname.startswith('m5.internal'):
            return None

        if fullname in self.modules and exists(self.modules[fullname]):
            return self

        return None

    def load_module(self, fullname):
        mod = imp.new_module(fullname)
        sys.modules[fullname] = mod
        self.installed.add(fullname)

        mod.__loader__ = self
        if fullname == 'm5.objects':
            mod.__path__ = fullname.split('.')
            return mod

        if fullname == '__scons':
            mod.__dict__['m5_build_env'] = self.build_env
            return mod

        srcfile = self.modules[fullname]
        if basename(srcfile) == '__init__.py':
            mod.__path__ = fullname.split('.')
        mod.__file__ = srcfile

        exec file(srcfile, 'r') in mod.__dict__

        return mod

class ordered_dict(dict):
    def keys(self):
        keys = super(ordered_dict, self).keys()
        keys.sort()
        return keys

    def values(self):
        return [ self[key] for key in self.keys() ]

    def items(self):
        return [ (key,self[key]) for key in self.keys() ]

    def iterkeys(self):
        for key in self.keys():
            yield key

    def itervalues(self):
        for value in self.values():
            yield value

    def iteritems(self):
        for key,value in self.items():
            yield key, value

class Generate(object):
    def __init__(self, py_sources, sim_objects, build_env):
        self.py_sources = py_sources
        self.py_modules = {}
        for source in py_sources:
            self.py_modules[source.modpath]  = source.srcpath

        importer = DictImporter(self.py_modules, build_env)

        # install the python importer so we can grab stuff from the source
        # tree itself.
        sys.meta_path[0:0] = [ importer ]

        import m5
        self.m5 = m5

        # import all sim objects so we can populate the all_objects list
        # make sure that we're working with a list, then let's sort it
        sim_objects = list(sim_objects)
        sim_objects.sort()
        for simobj in sim_objects:
            exec('from m5.objects import %s' % simobj)

        # we need to unload all of the currently imported modules so that they
        # will be re-imported the next time the sconscript is run
        importer.unload()
        sys.meta_path.remove(importer)

        self.sim_objects = m5.SimObject.allClasses
        self.enums = m5.params.allEnums

        self.params = {}
        for name,obj in self.sim_objects.iteritems():
            for param in obj._params.local.values():
                if not hasattr(param, 'swig_decl'):
                    continue
                pname = param.ptype_str
                if pname not in self.params:
                    self.params[pname] = param

    def createSimObjectParam(self, target, source, env):
        assert len(target) == 1 and len(source) == 1

        hh_file = file(target[0].abspath, 'w')
        name = str(source[0].get_contents())
        obj = self.sim_objects[name]

        print >>hh_file, obj.cxx_decl()

    # Generate Python file containing a dict specifying the current
    # build_env flags.
    def makeDefinesPyFile(self, target, source, env):
        f = file(str(target[0]), 'w')
        print >>f, "m5_build_env = ", source[0]
        f.close()

    # Generate python file containing info about the M5 source code
    def makeInfoPyFile(self, target, source, env):
        f = file(str(target[0]), 'w')
        for src in source:
            data = ''.join(file(src.srcnode().abspath, 'r').xreadlines())
            print >>f, "%s = %s" % (src, repr(data))
        f.close()

    # Generate the __init__.py file for m5.objects
    def makeObjectsInitFile(self, target, source, env):
        f = file(str(target[0]), 'w')
        print >>f, 'from params import *'
        print >>f, 'from m5.SimObject import *'
        for module in source:
            print >>f, 'from %s import *' % module.get_contents()
        f.close()

    def createSwigParam(self, target, source, env):
        assert len(target) == 1 and len(source) == 1

        i_file = file(target[0].abspath, 'w')
        name = str(source[0].get_contents())
        param = self.params[name]

        for line in param.swig_decl():
            print >>i_file, line

    def createEnumStrings(self, target, source, env):
        assert len(target) == 1 and len(source) == 1

        cc_file = file(target[0].abspath, 'w')
        name = str(source[0].get_contents())
        obj = self.enums[name]

        print >>cc_file, obj.cxx_def()
        cc_file.close()

    def createEnumParam(self, target, source, env):
        assert len(target) == 1 and len(source) == 1

        hh_file = file(target[0].abspath, 'w')
        name = str(source[0].get_contents())
        obj = self.enums[name]

        print >>hh_file, obj.cxx_decl()

    def buildParams(self, target, source, env):
        names = [ s.get_contents() for s in source ]
        objs = [ self.sim_objects[name] for name in names ]
        out = file(target[0].abspath, 'w')

        ordered_objs = []
        obj_seen = set()
        def order_obj(obj):
            name = str(obj)
            if name in obj_seen:
                return

            obj_seen.add(name)
            if str(obj) != 'SimObject':
                order_obj(obj.__bases__[0])

            ordered_objs.append(obj)

        for obj in objs:
            order_obj(obj)

        enums = set()
        predecls = []
        pd_seen = set()

        def add_pds(*pds):
            for pd in pds:
                if pd not in pd_seen:
                    predecls.append(pd)
                    pd_seen.add(pd)

        for obj in ordered_objs:
            params = obj._params.local.values()
            for param in params:
                ptype = param.ptype
                if issubclass(ptype, self.m5.params.Enum):
                    if ptype not in enums:
                        enums.add(ptype)
                pds = param.swig_predecls()
                if isinstance(pds, (list, tuple)):
                    add_pds(*pds)
                else:
                    add_pds(pds)

        print >>out, '%module params'

        print >>out, '%{'
        for obj in ordered_objs:
            print >>out, '#include "params/%s.hh"' % obj
        print >>out, '%}'

        for pd in predecls:
            print >>out, pd

        enums = list(enums)
        enums.sort()
        for enum in enums:
            print >>out, '%%include "enums/%s.hh"' % enum.__name__
        print >>out

        for obj in ordered_objs:
            if obj.swig_objdecls:
                for decl in obj.swig_objdecls:
                    print >>out, decl
                continue

            code = ''
            base = obj.get_base()

            code += '// stop swig from creating/wrapping default ctor/dtor\n'
            code += '%%nodefault %s;\n' % obj.cxx_class
            code += 'class %s ' % obj.cxx_class
            if base:
                code += ': public %s' % base
            code += ' {};\n'

            klass = obj.cxx_class;
            if hasattr(obj, 'cxx_namespace'):
                new_code = 'namespace %s {\n' % obj.cxx_namespace
                new_code += code
                new_code += '}\n'
                code = new_code
                klass = '%s::%s' % (obj.cxx_namespace, klass)

            print >>out, code

        print >>out, '%%include "src/sim/sim_object_params.hh"' % obj
        for obj in ordered_objs:
            print >>out, '%%include "params/%s.hh"' % obj

    def makeSwigInit(self, target, source, env):
        f = file(str(target[0]), 'w')
        print >>f, 'extern "C" {'
        for module in source:
            print >>f, '    void init_%s();' % module.get_contents()
        print >>f, '}'
        print >>f, 'void init_swig() {'
        for module in source:
            print >>f, '    init_%s();' % module.get_contents()
        print >>f, '}'
        f.close()

    def compilePyFile(self, target, source, env):
        '''Action function to compile a .py into a .pyc'''
        py_compile.compile(str(source[0]), str(target[0]))

    def buildPyZip(self, target, source, env):
        '''Action function to build the zip archive.  Uses the
        PyZipFile module included in the standard Python library.'''

        py_compiled = {}
        for s in self.py_sources:
            compname = str(s.compiled)
            assert compname not in py_compiled
            py_compiled[compname] = s

        zf = zipfile.ZipFile(str(target[0]), 'w')
        for s in source:
            zipname = str(s)
            arcname = py_compiled[zipname].arcname
            zf.write(zipname, arcname)
        zf.close()

    def traceFlagsPy(self, target, source, env):
        assert(len(target) == 1)

        f = file(str(target[0]), 'w')

        allFlags = []
        for s in source:
            val = eval(s.get_contents())
            allFlags.append(val)

        print >>f, 'baseFlags = ['
        for flag, compound, desc in allFlags:
            if not compound:
                print >>f, "    '%s'," % flag
        print >>f, "    ]"
        print >>f

        print >>f, 'compoundFlags = ['
        print >>f, "    'All',"
        for flag, compound, desc in allFlags:
            if compound:
                print >>f, "    '%s'," % flag
        print >>f, "    ]"
        print >>f

        print >>f, "allFlags = frozenset(baseFlags + compoundFlags)"
        print >>f

        print >>f, 'compoundFlagMap = {'
        all = tuple([flag for flag,compound,desc in allFlags if not compound])
        print >>f, "    'All' : %s," % (all, )
        for flag, compound, desc in allFlags:
            if compound:
                print >>f, "    '%s' : %s," % (flag, compound)
        print >>f, "    }"
        print >>f

        print >>f, 'flagDescriptions = {'
        print >>f, "    'All' : 'All flags',"
        for flag, compound, desc in allFlags:
            print >>f, "    '%s' : '%s'," % (flag, desc)
        print >>f, "    }"

        f.close()

    def traceFlagsCC(self, target, source, env):
        assert(len(target) == 1)

        f = file(str(target[0]), 'w')

        allFlags = []
        for s in source:
            val = eval(s.get_contents())
            allFlags.append(val)

        # file header
        print >>f, '''
/*
 * DO NOT EDIT THIS FILE! Automatically generated
 */

#include "base/traceflags.hh"

using namespace Trace;

const char *Trace::flagStrings[] =
{'''

        # The string array is used by SimpleEnumParam to map the strings
        # provided by the user to enum values.
        for flag, compound, desc in allFlags:
            if not compound:
                print >>f, '    "%s",' % flag

        print >>f, '    "All",'
        for flag, compound, desc in allFlags:
            if compound:
                print >>f, '    "%s",' % flag

        print >>f, '};'
        print >>f
        print >>f, 'const int Trace::numFlagStrings = %d;' % (len(allFlags) + 1)
        print >>f

        #
        # Now define the individual compound flag arrays.  There is an array
        # for each compound flag listing the component base flags.
        #
        all = tuple([flag for flag,compound,desc in allFlags if not compound])
        print >>f, 'static const Flags AllMap[] = {'
        for flag, compound, desc in allFlags:
            if not compound:
                print >>f, "    %s," % flag
        print >>f, '};'
        print >>f

        for flag, compound, desc in allFlags:
            if not compound:
                continue
            print >>f, 'static const Flags %sMap[] = {' % flag
            for flag in compound:
                print >>f, "    %s," % flag
            print >>f, "    (Flags)-1"
            print >>f, '};'
            print >>f

        #
        # Finally the compoundFlags[] array maps the compound flags
        # to their individual arrays/
        #
        print >>f, 'const Flags *Trace::compoundFlags[] ='
        print >>f, '{'
        print >>f, '    AllMap,'
        for flag, compound, desc in allFlags:
            if compound:
                print >>f, '    %sMap,' % flag
        # file trailer
        print >>f, '};'

        f.close()

    def traceFlagsHH(self, target, source, env):
        assert(len(target) == 1)

        f = file(str(target[0]), 'w')

        allFlags = []
        for s in source:
            val = eval(s.get_contents())
            allFlags.append(val)

        # file header boilerplate
        print >>f, '''
/*
 * DO NOT EDIT THIS FILE!
 *
 * Automatically generated from traceflags.py
 */

#ifndef __BASE_TRACE_FLAGS_HH__
#define __BASE_TRACE_FLAGS_HH__

namespace Trace {

enum Flags {'''

        # Generate the enum.  Base flags come first, then compound flags.
        idx = 0
        for flag, compound, desc in allFlags:
            if not compound:
                print >>f, '    %s = %d,' % (flag, idx)
                idx += 1

        numBaseFlags = idx
        print >>f, '    NumFlags = %d,' % idx

        # put a comment in here to separate base from compound flags
        print >>f, '''
// The remaining enum values are *not* valid indices for Trace::flags.
// They are "compound" flags, which correspond to sets of base
// flags, and are used by changeFlag.'''

        print >>f, '    All = %d,' % idx
        idx += 1
        for flag, compound, desc in allFlags:
            if compound:
                print >>f, '    %s = %d,' % (flag, idx)
                idx += 1

        numCompoundFlags = idx - numBaseFlags
        print >>f, '    NumCompoundFlags = %d' % numCompoundFlags

        # trailer boilerplate
        print >>f, '''\
}; // enum Flags

// Array of strings for SimpleEnumParam
extern const char *flagStrings[];
extern const int numFlagStrings;

// Array of arraay pointers: for each compound flag, gives the list of
// base flags to set.  Inidividual flag arrays are terminated by -1.
extern const Flags *compoundFlags[];

/* namespace Trace */ }

#endif // __BASE_TRACE_FLAGS_HH__
'''

        f.close()

    def programInfo(self, target, source, env):
        def gen_file(target, rev, node, date):
            pi_stats = file(target, 'w')
            print >>pi_stats, 'const char *hgRev = "%s:%s";' %  (rev, node)
            print >>pi_stats, 'const char *hgDate = "%s";' % date
            pi_stats.close()

        target = str(target[0])
        scons_dir = str(source[0].get_contents())
        try:
            import mercurial.demandimport, mercurial.hg, mercurial.ui
            import mercurial.util, mercurial.node
            if not exists(scons_dir) or not isdir(scons_dir) or \
                   not exists(join(scons_dir, ".hg")):
                raise ValueError
            repo = mercurial.hg.repository(mercurial.ui.ui(), scons_dir)
            rev = mercurial.node.nullrev + repo.changelog.count()
            changenode = repo.changelog.node(rev)
            changes = repo.changelog.read(changenode)
            date = mercurial.util.datestr(changes[2])

            gen_file(target, rev, mercurial.node.hex(changenode), date)

            mercurial.demandimport.disable()
        except ImportError:
            gen_file(target, "Unknown", "Unknown", "Unknown")

        except:
            print "in except"
            gen_file(target, "Unknown", "Unknown", "Unknown")
            mercurial.demandimport.disable()