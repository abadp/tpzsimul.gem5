/*
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
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
 *          Erik Hallnor
 *          Steve Reinhardt
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <fstream>
#include <list>
#include <string>
#include <vector>

#include "base/inifile.hh"
#include "base/misc.hh"
#include "base/output.hh"
#include "base/str.hh"
#include "base/trace.hh"
#include "sim/eventq.hh"
#include "sim/serialize.hh"
#include "sim/sim_events.hh"
#include "sim/sim_exit.hh"
#include "sim/sim_object.hh"

// For stat reset hack
#include "sim/stat_control.hh"

using namespace std;

extern SimObject *resolveSimObject(const string &);

//
// The base implementations use to_number for parsing and '<<' for
// displaying, suitable for integer types.
//
template <class T>
bool
parseParam(const string &s, T &value)
{
    return to_number(s, value);
}

template <class T>
void
showParam(ostream &os, const T &value)
{
    os << value;
}

//
// Template specializations:
// - char (8-bit integer)
// - floating-point types
// - bool
// - string
//

// Treat 8-bit ints (chars) as ints on output, not as chars
template <>
void
showParam(ostream &os, const char &value)
{
    os << (int)value;
}


template <>
void
showParam(ostream &os, const unsigned char &value)
{
    os << (unsigned int)value;
}


// Use sscanf() for FP types as to_number() only handles integers
template <>
bool
parseParam(const string &s, float &value)
{
    return (sscanf(s.c_str(), "%f", &value) == 1);
}

template <>
bool
parseParam(const string &s, double &value)
{
    return (sscanf(s.c_str(), "%lf", &value) == 1);
}

template <>
bool
parseParam(const string &s, bool &value)
{
    const string &ls = to_lower(s);

    if (ls == "true") {
        value = true;
        return true;
    }

    if (ls == "false") {
        value = false;
        return true;
    }

    return false;
}

// Display bools as strings
template <>
void
showParam(ostream &os, const bool &value)
{
    os << (value ? "true" : "false");
}


// String requires no processing to speak of
template <>
bool
parseParam(const string &s, string &value)
{
    value = s;
    return true;
}

int Serializable::ckptMaxCount = 0;
int Serializable::ckptCount = 0;
int Serializable::ckptPrevCount = -1;

void
Serializable::nameOut(ostream &os)
{
    os << "\n[" << name() << "]\n";
}

void
Serializable::nameOut(ostream &os, const string &_name)
{
    os << "\n[" << _name << "]\n";
}

template <class T>
void
paramOut(ostream &os, const std::string &name, const T &param)
{
    os << name << "=";
    showParam(os, param);
    os << "\n";
}

template <class T>
void
arrayParamOut(ostream &os, const std::string &name,
              const std::vector<T> &param)
{
    int size = param.size();
    os << name << "=";
    if (size > 0)
        showParam(os, param[0]);
    for (int i = 1; i < size; ++i) {
        os << " ";
        showParam(os, param[i]);
    }
    os << "\n";
}


template <class T>
void
paramIn(Checkpoint *cp, const std::string &section,
        const std::string &name, T &param)
{
    std::string str;
    if (!cp->find(section, name, str) || !parseParam(str, param)) {
        fatal("Can't unserialize '%s:%s'\n", section, name);
    }
}


template <class T>
void
arrayParamOut(ostream &os, const std::string &name,
              const T *param, int size)
{
    os << name << "=";
    if (size > 0)
        showParam(os, param[0]);
    for (int i = 1; i < size; ++i) {
        os << " ";
        showParam(os, param[i]);
    }
    os << "\n";
}


template <class T>
void
arrayParamIn(Checkpoint *cp, const std::string &section,
             const std::string &name, T *param, int size)
{
    std::string str;
    if (!cp->find(section, name, str)) {
        fatal("Can't unserialize '%s:%s'\n", section, name);
    }

    // code below stolen from VectorParam<T>::parse().
    // it would be nice to unify these somehow...

    vector<string> tokens;

    tokenize(tokens, str, ' ');

    // Need this if we were doing a vector
    // value.resize(tokens.size());

    if (tokens.size() != size) {
        fatal("Array size mismatch on %s:%s'\n", section, name);
    }

    for (int i = 0; i < tokens.size(); i++) {
        // need to parse into local variable to handle vector<bool>,
        // for which operator[] returns a special reference class
        // that's not the same as 'bool&', (since it's a packed
        // vector)
        T scalar_value;
        if (!parseParam(tokens[i], scalar_value)) {
            string err("could not parse \"");

            err += str;
            err += "\"";

            fatal(err);
        }

        // assign parsed value to vector
        param[i] = scalar_value;
    }
}

template <class T>
void
arrayParamIn(Checkpoint *cp, const std::string &section,
             const std::string &name, std::vector<T> &param)
{
    std::string str;
    if (!cp->find(section, name, str)) {
        fatal("Can't unserialize '%s:%s'\n", section, name);
    }

    // code below stolen from VectorParam<T>::parse().
    // it would be nice to unify these somehow...

    vector<string> tokens;

    tokenize(tokens, str, ' ');

    // Need this if we were doing a vector
    // value.resize(tokens.size());

    param.resize(tokens.size());

    for (int i = 0; i < tokens.size(); i++) {
        // need to parse into local variable to handle vector<bool>,
        // for which operator[] returns a special reference class
        // that's not the same as 'bool&', (since it's a packed
        // vector)
        T scalar_value;
        if (!parseParam(tokens[i], scalar_value)) {
            string err("could not parse \"");

            err += str;
            err += "\"";

            fatal(err);
        }

        // assign parsed value to vector
        param[i] = scalar_value;
    }
}



void
objParamIn(Checkpoint *cp, const std::string &section,
           const std::string &name, SimObject * &param)
{
    if (!cp->findObj(section, name, param)) {
        fatal("Can't unserialize '%s:%s'\n", section, name);
    }
}


#define INSTANTIATE_PARAM_TEMPLATES(type)				\
template void								\
paramOut(ostream &os, const std::string &name, type const &param);	\
template void								\
paramIn(Checkpoint *cp, const std::string &section,			\
        const std::string &name, type & param);				\
template void								\
arrayParamOut(ostream &os, const std::string &name,			\
              type const *param, int size);				\
template void								\
arrayParamIn(Checkpoint *cp, const std::string &section,		\
             const std::string &name, type *param, int size);           \
template void								\
arrayParamOut(ostream &os, const std::string &name,			\
              const std::vector<type> &param);				\
template void								\
arrayParamIn(Checkpoint *cp, const std::string &section,		\
             const std::string &name, std::vector<type> &param);

INSTANTIATE_PARAM_TEMPLATES(signed char)
INSTANTIATE_PARAM_TEMPLATES(unsigned char)
INSTANTIATE_PARAM_TEMPLATES(signed short)
INSTANTIATE_PARAM_TEMPLATES(unsigned short)
INSTANTIATE_PARAM_TEMPLATES(signed int)
INSTANTIATE_PARAM_TEMPLATES(unsigned int)
INSTANTIATE_PARAM_TEMPLATES(signed long)
INSTANTIATE_PARAM_TEMPLATES(unsigned long)
INSTANTIATE_PARAM_TEMPLATES(signed long long)
INSTANTIATE_PARAM_TEMPLATES(unsigned long long)
INSTANTIATE_PARAM_TEMPLATES(bool)
INSTANTIATE_PARAM_TEMPLATES(string)


/////////////////////////////

/// Container for serializing global variables (not associated with
/// any serialized object).
class Globals : public Serializable
{
  public:
    const string name() const;
    void serialize(ostream &os);
    void unserialize(Checkpoint *cp);
};

/// The one and only instance of the Globals class.
Globals globals;

const string
Globals::name() const
{
    return "Globals";
}

void
Globals::serialize(ostream &os)
{
    nameOut(os);
    SERIALIZE_SCALAR(curTick);

    nameOut(os, "MainEventQueue");
    mainEventQueue.serialize(os);
}

void
Globals::unserialize(Checkpoint *cp)
{
    const string &section = name();
    UNSERIALIZE_SCALAR(curTick);

    mainEventQueue.unserialize(cp, "MainEventQueue");
}

void
Serializable::serializeAll(const std::string &cpt_dir)
{
    setCheckpointDir(cpt_dir);
    string dir = Checkpoint::dir();
    if (mkdir(dir.c_str(), 0775) == -1 && errno != EEXIST)
            fatal("couldn't mkdir %s\n", dir);

    string cpt_file = dir + Checkpoint::baseFilename;
    ofstream outstream(cpt_file.c_str());
    time_t t = time(NULL);
    outstream << "// checkpoint generated: " << ctime(&t);

    globals.serialize(outstream);
    SimObject::serializeAll(outstream);
}

void
Serializable::unserializeAll(const std::string &cpt_dir)
{
    setCheckpointDir(cpt_dir);
    string dir = Checkpoint::dir();
    string cpt_file = dir + Checkpoint::baseFilename;
    string section = "";

    DPRINTFR(Config, "Loading checkpoint dir '%s'\n",
             dir);
    Checkpoint *cp = new Checkpoint(dir, section);
    unserializeGlobals(cp);
    SimObject::unserializeAll(cp);
}

void
Serializable::unserializeGlobals(Checkpoint *cp)
{
    globals.unserialize(cp);
}

const char *Checkpoint::baseFilename = "m5.cpt";

static string checkpointDirBase;

void
setCheckpointDir(const std::string &name)
{
    checkpointDirBase = name;
    if (checkpointDirBase[checkpointDirBase.size() - 1] != '/')
        checkpointDirBase += "/";
}

string
Checkpoint::dir()
{
    // use csprintf to insert curTick into directory name if it
    // appears to have a format placeholder in it.
    return (checkpointDirBase.find("%") != string::npos) ?
        csprintf(checkpointDirBase, curTick) : checkpointDirBase;
}

void
debug_serialize(const std::string &cpt_dir)
{
    Serializable::serializeAll(cpt_dir);
}


////////////////////////////////////////////////////////////////////////
//
// SerializableClass member definitions
//
////////////////////////////////////////////////////////////////////////

// Map of class names to SerializableBuilder creation functions.
// Need to make this a pointer so we can force initialization on the
// first reference; otherwise, some SerializableClass constructors
// may be invoked before the classMap constructor.
map<string,SerializableClass::CreateFunc> *SerializableClass::classMap = 0;

// SerializableClass constructor: add mapping to classMap
SerializableClass::SerializableClass(const string &className,
                                       CreateFunc createFunc)
{
    if (classMap == NULL)
        classMap = new map<string,SerializableClass::CreateFunc>();

    if ((*classMap)[className])
    {
        cerr << "Error: simulation object class " << className << " redefined"
             << endl;
        fatal("");
    }

    // add className --> createFunc to class map
    (*classMap)[className] = createFunc;
}


//
//
Serializable *
SerializableClass::createObject(Checkpoint *cp,
                                 const std::string &section)
{
    string className;

    if (!cp->find(section, "type", className)) {
        fatal("Serializable::create: no 'type' entry in section '%s'.\n",
              section);
    }

    CreateFunc createFunc = (*classMap)[className];

    if (createFunc == NULL) {
        fatal("Serializable::create: no create function for class '%s'.\n",
              className);
    }

    Serializable *object = createFunc(cp, section);

    assert(object != NULL);

    return object;
}


Serializable *
Serializable::create(Checkpoint *cp, const std::string &section)
{
    Serializable *object = SerializableClass::createObject(cp, section);
    object->unserialize(cp, section);
    return object;
}


Checkpoint::Checkpoint(const std::string &cpt_dir, const std::string &path)
    : db(new IniFile), basePath(path), cptDir(cpt_dir)
{
    string filename = cpt_dir + "/" + Checkpoint::baseFilename;
    if (!db->load(filename)) {
        fatal("Can't load checkpoint file '%s'\n", filename);
    }
}


bool
Checkpoint::find(const std::string &section, const std::string &entry,
                 std::string &value)
{
    return db->find(section, entry, value);
}


bool
Checkpoint::findObj(const std::string &section, const std::string &entry,
                    SimObject *&value)
{
    string path;

    if (!db->find(section, entry, path))
        return false;

    value = resolveSimObject(path);
    return true;
}


bool
Checkpoint::sectionExists(const std::string &section)
{
    return db->sectionExists(section);
}