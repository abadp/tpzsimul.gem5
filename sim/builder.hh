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

#ifndef __BUILDER_HH__
#define __BUILDER_HH__

#include <map>
#include <list>
#include <vector>
#include <iostream>

#include "sim/param.hh"

class SimObject;

//
// A SimObjectBuilder serves as an evaluation context for a set of
// parameters that describe a specific instance of a SimObject.  This
// evaluation context corresponds to a section in the .ini file (as
// with the base ParamContext) plus an optional node in the
// configuration hierarchy (the configNode member) for resolving
// SimObject references.  SimObjectBuilder is an abstract superclass;
// derived classes specialize the class for particular subclasses of
// SimObject (e.g., BaseCache).
//
// For typical usage, see the definition of
// SimObjectClass::createObject().
//
class SimObjectBuilder : public ParamContext
{
  private:
    // name of the instance we are creating
    std::string instanceName;

    // The corresponding node in the configuration hierarchy.
    // (optional: may be null if the created object is not in the
    // hierarchy)
    ConfigNode *configNode;

    // The external SimObject class name (for error messages)
    std::string simObjClassName;

  public:
    SimObjectBuilder(const std::string &_configClass,
                     const std::string &_instanceName,
                     ConfigNode *_configNode,
                     const std::string &_simObjClassName)
        : ParamContext(_configClass, true),
          instanceName(_instanceName),
          configNode(_configNode),
          simObjClassName(_simObjClassName)
    {
    }

    virtual ~SimObjectBuilder() {}

    // call parse() on all params in this context to convert string
    // representations to parameter values
    virtual void parseParams(IniFile &iniFile);

    // parameter error prolog (override of ParamContext)
    virtual void printErrorProlog(std::ostream &);

    // generate the name for this SimObject instance (derived from the
    // configuration hierarchy node label and position)
    virtual const std::string &getInstanceName() { return instanceName; }

    // return the configuration hierarchy node for this context.
    virtual ConfigNode *getConfigNode() { return configNode; }

    // Create the actual SimObject corresponding to the parameter
    // values in this context.  This function is overridden in derived
    // classes to call a specific constructor for a particular
    // subclass of SimObject.
    virtual SimObject *create() = 0;
};


//
// Handy macros for initializing parameter members of classes derived
// from SimObjectBuilder.  Assumes that the name of the parameter
// member object is the same as the textual parameter name seen by the
// user.  (Note that '#p' is expanded by the preprocessor to '"p"'.)
//
#define INIT_PARAM(p, desc)		p(this, #p, desc)
#define INIT_PARAM_DFLT(p, desc, dflt)	p(this, #p, desc, dflt)

//
// Initialize an enumeration variable... assumes that 'map' is the
// name of an array of mappings (char * for SimpleEnumParam, or
// EnumParamMap for MappedEnumParam).
//
#define INIT_ENUM_PARAM(p, desc, map)	\
        p(this, #p, desc, map, sizeof(map)/sizeof(map[0]))
#define INIT_ENUM_PARAM_DFLT(p, desc, map, dflt)	\
        p(this, #p, desc, map, sizeof(map)/sizeof(map[0]), dflt)

//
// An instance of SimObjectClass corresponds to a class derived from
// SimObject.  The SimObjectClass instance serves to bind the string
// name (found in the config file) to a function that creates an
// instance of the appropriate derived class.
//
// This would be much cleaner in Smalltalk or Objective-C, where types
// are first-class objects themselves.
//
class SimObjectClass
{
  public:
    // Type CreateFunc is a pointer to a function that creates a new
    // simulation object builder based on a .ini-file parameter
    // section (specified by the first string argument), a unique name
    // for the object (specified by the second string argument), and
    // an optional config hierarchy node (specified by the third
    // argument).  A pointer to the new SimObjectBuilder is returned.
    typedef SimObjectBuilder *(*CreateFunc)(const std::string &configClassName,
                                            const std::string &objName,
                                            ConfigNode *configNode,
                                            const std::string &simObjClassName);

    static std::map<std::string,CreateFunc> *classMap;

    // Constructor.  For example:
    //
    // SimObjectClass baseCacheClass("BaseCache", newBaseCacheBuilder);
    //
    SimObjectClass(const std::string &className, CreateFunc createFunc);

    // create SimObject given name of class and pointer to
    // configuration hierarchy node
    static SimObject *createObject(IniFile &configDB,
                                   const std::string &configClassName,
                                   const std::string &objName,
                                   ConfigNode *configNode);

    // print descriptions of all parameters registered with all
    // SimObject classes
    static void describeAllClasses(std::ostream &os);
};

//
// Macros to encapsulate the magic of declaring & defining
// SimObjectBuilder and SimObjectClass objects
//

#define BEGIN_DECLARE_SIM_OBJECT_PARAMS(OBJ_CLASS)		\
class OBJ_CLASS##Builder : public SimObjectBuilder		\
{								\
  public:

#define END_DECLARE_SIM_OBJECT_PARAMS(OBJ_CLASS)		\
                                                                \
    OBJ_CLASS##Builder(const std::string &configClass,		\
                       const std::string &instanceName,		\
                       ConfigNode *configNode,			\
                       const std::string &simObjClassName);	\
    virtual ~OBJ_CLASS##Builder() {}				\
                                                                \
    OBJ_CLASS *create();					\
};

#define BEGIN_INIT_SIM_OBJECT_PARAMS(OBJ_CLASS)				   \
OBJ_CLASS##Builder::OBJ_CLASS##Builder(const std::string &configClass,	   \
                                       const std::string &instanceName,	   \
                                       ConfigNode *configNode,		   \
                                       const std::string &simObjClassName) \
    : SimObjectBuilder(configClass, instanceName,			   \
                       configNode, simObjClassName),


#define END_INIT_SIM_OBJECT_PARAMS(OBJ_CLASS)			\
{								\
}

#define CREATE_SIM_OBJECT(OBJ_CLASS)				\
OBJ_CLASS *OBJ_CLASS##Builder::create()

#define REGISTER_SIM_OBJECT(CLASS_NAME, OBJ_CLASS)		\
SimObjectBuilder *						\
new##OBJ_CLASS##Builder(const std::string &configClass,		\
                        const std::string &instanceName,	\
                        ConfigNode *configNode,			\
                        const std::string &simObjClassName)	\
{								\
    return new OBJ_CLASS##Builder(configClass, instanceName,	\
                                  configNode, simObjClassName);	\
}								\
                                                                \
SimObjectClass the##OBJ_CLASS##Class(CLASS_NAME,		\
                                     new##OBJ_CLASS##Builder);	\
                                                                \
/* see param.hh */						\
DEFINE_SIM_OBJECT_CLASS_NAME(CLASS_NAME, OBJ_CLASS)


#endif // __BUILDER_HH__
