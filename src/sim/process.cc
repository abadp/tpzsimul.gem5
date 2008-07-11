/*
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
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
 *          Ali Saidi
 */

#include <unistd.h>
#include <fcntl.h>
#include <string>

#include "arch/remote_gdb.hh"
#include "base/intmath.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "base/statistics.hh"
#include "config/full_system.hh"
#include "cpu/thread_context.hh"
#include "mem/page_table.hh"
#include "mem/physical.hh"
#include "mem/translating_port.hh"
#include "params/Process.hh"
#include "params/LiveProcess.hh"
#include "sim/process.hh"
#include "sim/process_impl.hh"
#include "sim/stats.hh"
#include "sim/syscall_emul.hh"
#include "sim/system.hh"

#include "arch/isa_specific.hh"
#if THE_ISA == ALPHA_ISA
#include "arch/alpha/linux/process.hh"
#include "arch/alpha/tru64/process.hh"
#elif THE_ISA == SPARC_ISA
#include "arch/sparc/linux/process.hh"
#include "arch/sparc/solaris/process.hh"
#elif THE_ISA == MIPS_ISA
#include "arch/mips/linux/process.hh"
#elif THE_ISA == ARM_ISA
#include "arch/arm/linux/process.hh"
#elif THE_ISA == X86_ISA
#include "arch/x86/linux/process.hh"
#else
#error "THE_ISA not set"
#endif


using namespace std;
using namespace TheISA;

//
// The purpose of this code is to fake the loader & syscall mechanism
// when there's no OS: thus there's no resone to use it in FULL_SYSTEM
// mode when we do have an OS
//
#if FULL_SYSTEM
#error "process.cc not compatible with FULL_SYSTEM"
#endif

// current number of allocated processes
int num_processes = 0;

Process::Process(ProcessParams * params)
    : SimObject(params), system(params->system), checkpointRestored(false),
    max_stack_size(params->max_stack_size)
{
    string in = params->input;
    string out = params->output;

    // initialize file descriptors to default: same as simulator
    int stdin_fd, stdout_fd, stderr_fd;

    if (in == "stdin" || in == "cin")
        stdin_fd = STDIN_FILENO;
    else if (in == "None")
        stdin_fd = -1;
    else
        stdin_fd = Process::openInputFile(in);

    if (out == "stdout" || out == "cout")
        stdout_fd = STDOUT_FILENO;
    else if (out == "stderr" || out == "cerr")
        stdout_fd = STDERR_FILENO;
    else if (out == "None")
        stdout_fd = -1;
    else
        stdout_fd = Process::openOutputFile(out);

    stderr_fd = (stdout_fd != STDOUT_FILENO) ? stdout_fd : STDERR_FILENO;

    M5_pid = system->allocatePID();
    // initialize first 3 fds (stdin, stdout, stderr)
    Process::FdMap *fdo = &fd_map[STDIN_FILENO];
    fdo->fd = stdin_fd;
    fdo->filename = in;
    fdo->flags = O_RDONLY;
    fdo->mode = -1;
    fdo->fileOffset = 0;

    fdo =  &fd_map[STDOUT_FILENO];
    fdo->fd = stdout_fd;
    fdo->filename = out;
    fdo->flags =  O_WRONLY | O_CREAT | O_TRUNC;
    fdo->mode = 0774;
    fdo->fileOffset = 0;

    fdo = &fd_map[STDERR_FILENO];
    fdo->fd = stderr_fd;
    fdo->filename = "STDERR";
    fdo->flags = O_WRONLY;
    fdo->mode = -1;
    fdo->fileOffset = 0;


    // mark remaining fds as free
    for (int i = 3; i <= MAX_FD; ++i) {
        Process::FdMap *fdo = &fd_map[i];
        fdo->fd = -1;
    }

    mmap_start = mmap_end = 0;
    nxm_start = nxm_end = 0;
    pTable = new PageTable(this);
    // other parameters will be initialized when the program is loaded
}


void
Process::regStats()
{
    using namespace Stats;

    num_syscalls
        .name(name() + ".PROG:num_syscalls")
        .desc("Number of system calls")
        ;
}

//
// static helper functions
//
int
Process::openInputFile(const string &filename)
{
    int fd = open(filename.c_str(), O_RDONLY);

    if (fd == -1) {
        perror(NULL);
        cerr << "unable to open \"" << filename << "\" for reading\n";
        fatal("can't open input file");
    }

    return fd;
}


int
Process::openOutputFile(const string &filename)
{
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0774);

    if (fd == -1) {
        perror(NULL);
        cerr << "unable to open \"" << filename << "\" for writing\n";
        fatal("can't open output file");
    }

    return fd;
}


int
Process::registerThreadContext(ThreadContext *tc)
{
    // add to list
    int myIndex = threadContexts.size();
    threadContexts.push_back(tc);

    RemoteGDB *rgdb = new RemoteGDB(system, tc);
    GDBListener *gdbl = new GDBListener(rgdb, 7000 + myIndex);
    gdbl->listen();
    //gdbl->accept();

    remoteGDB.push_back(rgdb);

    // return CPU number to caller
    return myIndex;
}

void
Process::startup()
{
    if (threadContexts.empty())
        fatal("Process %s is not associated with any CPUs!\n", name());

    // first thread context for this process... initialize & enable
    ThreadContext *tc = threadContexts[0];

    // mark this context as active so it will start ticking.
    tc->activate(0);

    Port *mem_port;
    mem_port = system->physmem->getPort("functional");
    initVirtMem = new TranslatingPort("process init port", this,
            TranslatingPort::Always);
    mem_port->setPeer(initVirtMem);
    initVirtMem->setPeer(mem_port);
}

void
Process::replaceThreadContext(ThreadContext *tc, int tcIndex)
{
    if (tcIndex >= threadContexts.size()) {
        panic("replaceThreadContext: bad tcIndex, %d >= %d\n",
              tcIndex, threadContexts.size());
    }

    threadContexts[tcIndex] = tc;
}

// map simulator fd sim_fd to target fd tgt_fd
void
Process::dup_fd(int sim_fd, int tgt_fd)
{
    if (tgt_fd < 0 || tgt_fd > MAX_FD)
        panic("Process::dup_fd tried to dup past MAX_FD (%d)", tgt_fd);

    Process::FdMap *fdo = &fd_map[tgt_fd];
    fdo->fd = sim_fd;
}


// generate new target fd for sim_fd
int
Process::alloc_fd(int sim_fd, string filename, int flags, int mode, bool pipe)
{
    // in case open() returns an error, don't allocate a new fd
    if (sim_fd == -1)
        return -1;

    // find first free target fd
    for (int free_fd = 0; free_fd <= MAX_FD; ++free_fd) {
        Process::FdMap *fdo = &fd_map[free_fd];
        if (fdo->fd == -1) {
            fdo->fd = sim_fd;
            fdo->filename = filename;
            fdo->mode = mode;
            fdo->fileOffset = 0;
            fdo->flags = flags;
            fdo->isPipe = pipe;
            fdo->readPipeSource = 0;
            return free_fd;
        }
    }

    panic("Process::alloc_fd: out of file descriptors!");
}


// free target fd (e.g., after close)
void
Process::free_fd(int tgt_fd)
{
    Process::FdMap *fdo = &fd_map[tgt_fd];
    if (fdo->fd == -1)
        warn("Process::free_fd: request to free unused fd %d", tgt_fd);

    fdo->fd = -1;
    fdo->filename = "NULL";
    fdo->mode = 0;
    fdo->fileOffset = 0;
    fdo->flags = 0;
    fdo->isPipe = false;
    fdo->readPipeSource = 0;
}


// look up simulator fd for given target fd
int
Process::sim_fd(int tgt_fd)
{
    if (tgt_fd > MAX_FD)
        return -1;

    return fd_map[tgt_fd].fd;
}

Process::FdMap *
Process::sim_fd_obj(int tgt_fd)
{
    if (tgt_fd > MAX_FD)
        panic("sim_fd_obj called in fd out of range.");

    return &fd_map[tgt_fd];
}
bool
Process::checkAndAllocNextPage(Addr vaddr)
{
    // if this is an initial write we might not have
    if (vaddr >= stack_min && vaddr < stack_base) {
        pTable->allocate(roundDown(vaddr, VMPageSize), VMPageSize);
        return true;
    }

    // We've accessed the next page of the stack, so extend the stack
    // to cover it.
    if (vaddr < stack_min && vaddr >= stack_base - max_stack_size) {
        while (vaddr < stack_min) {
            stack_min -= TheISA::PageBytes;
            if(stack_base - stack_min > max_stack_size)
                fatal("Maximum stack size exceeded\n");
            if(stack_base - stack_min > 8*1024*1024)
                fatal("Over max stack size for one thread\n");
            pTable->allocate(stack_min, TheISA::PageBytes);
            warn("Increasing stack size by one page.");
        };
        return true;
    }
    return false;
}

 // find all offsets for currently open files and save them
void
Process::fix_file_offsets() {
    Process::FdMap *fdo_stdin = &fd_map[STDIN_FILENO];
    Process::FdMap *fdo_stdout = &fd_map[STDOUT_FILENO];
    Process::FdMap *fdo_stderr = &fd_map[STDERR_FILENO];
    string in = fdo_stdin->filename;
    string out = fdo_stdout->filename;

    // initialize file descriptors to default: same as simulator
    int stdin_fd, stdout_fd, stderr_fd;

    if (in == "stdin" || in == "cin")
        stdin_fd = STDIN_FILENO;
    else if (in == "None")
        stdin_fd = -1;
    else{
        //OPEN standard in and seek to the right location
        stdin_fd = Process::openInputFile(in);
        if (lseek(stdin_fd, fdo_stdin->fileOffset, SEEK_SET) < 0)
            panic("Unable to seek to correct location in file: %s", in);
    }

    if (out == "stdout" || out == "cout")
        stdout_fd = STDOUT_FILENO;
    else if (out == "stderr" || out == "cerr")
        stdout_fd = STDERR_FILENO;
    else if (out == "None")
        stdout_fd = -1;
    else{
        stdout_fd = Process::openOutputFile(out);
        if (lseek(stdin_fd, fdo_stdout->fileOffset, SEEK_SET) < 0)
            panic("Unable to seek to correct in file: %s", out);
    }

    stderr_fd = (stdout_fd != STDOUT_FILENO) ? stdout_fd : STDERR_FILENO;

    fdo_stdin->fd = stdin_fd;
    fdo_stdout->fd = stdout_fd;
    fdo_stderr->fd = stderr_fd;


    for (int free_fd = 3; free_fd <= MAX_FD; ++free_fd) {
        Process::FdMap *fdo = &fd_map[free_fd];
        if (fdo->fd != -1) {
            if (fdo->isPipe){
                if (fdo->filename == "PIPE-WRITE")
                    continue;
                else {
                    assert (fdo->filename == "PIPE-READ");
                    //create a new pipe
                    int fds[2];
                    int pipe_retval = pipe(fds);

                    if (pipe_retval < 0) {
                        // error
                        panic("Unable to create new pipe.");
                    }
                    fdo->fd = fds[0]; //set read pipe
                    Process::FdMap *fdo_write = &fd_map[fdo->readPipeSource];
                    if (fdo_write->filename != "PIPE-WRITE")
                        panic ("Couldn't find write end of the pipe");

                    fdo_write->fd = fds[1];//set write pipe
               }
            } else {
                //Open file
                int fd = open(fdo->filename.c_str(), fdo->flags, fdo->mode);

                if (fd == -1)
                    panic("Unable to open file: %s", fdo->filename);
                fdo->fd = fd;

                //Seek to correct location before checkpoint
                if (lseek(fd,fdo->fileOffset, SEEK_SET) < 0)
                    panic("Unable to seek to correct location in file: %s", fdo->filename);
            }
        }
    }
}
void
Process::find_file_offsets(){
    for (int free_fd = 0; free_fd <= MAX_FD; ++free_fd) {
        Process::FdMap *fdo = &fd_map[free_fd];
        if (fdo->fd != -1) {
            fdo->fileOffset = lseek(fdo->fd, 0, SEEK_CUR);
        }  else {
                fdo->filename = "NULL";
                fdo->fileOffset = 0;
        }
    }
}

void
Process::setReadPipeSource(int read_pipe_fd, int source_fd){
    Process::FdMap *fdo = &fd_map[read_pipe_fd];
    fdo->readPipeSource = source_fd;
}

void
Process::FdMap::serialize(std::ostream &os)
{
    SERIALIZE_SCALAR(fd);
    SERIALIZE_SCALAR(isPipe);
    SERIALIZE_SCALAR(filename);
    SERIALIZE_SCALAR(flags);
    SERIALIZE_SCALAR(readPipeSource);
    SERIALIZE_SCALAR(fileOffset);
}

void
Process::FdMap::unserialize(Checkpoint *cp, const std::string &section)
{
    UNSERIALIZE_SCALAR(fd);
    UNSERIALIZE_SCALAR(isPipe);
    UNSERIALIZE_SCALAR(filename);
    UNSERIALIZE_SCALAR(flags);
    UNSERIALIZE_SCALAR(readPipeSource);
    UNSERIALIZE_SCALAR(fileOffset);
}

void
Process::serialize(std::ostream &os)
{
    SERIALIZE_SCALAR(initialContextLoaded);
    SERIALIZE_SCALAR(brk_point);
    SERIALIZE_SCALAR(stack_base);
    SERIALIZE_SCALAR(stack_size);
    SERIALIZE_SCALAR(stack_min);
    SERIALIZE_SCALAR(next_thread_stack_base);
    SERIALIZE_SCALAR(mmap_start);
    SERIALIZE_SCALAR(mmap_end);
    SERIALIZE_SCALAR(nxm_start);
    SERIALIZE_SCALAR(nxm_end);
    find_file_offsets();
    pTable->serialize(os);
    for (int x = 0; x <= MAX_FD; x++) {
        nameOut(os, csprintf("%s.FdMap%d", name(), x));
        fd_map[x].serialize(os);
    }

}

void
Process::unserialize(Checkpoint *cp, const std::string &section)
{
    UNSERIALIZE_SCALAR(initialContextLoaded);
    UNSERIALIZE_SCALAR(brk_point);
    UNSERIALIZE_SCALAR(stack_base);
    UNSERIALIZE_SCALAR(stack_size);
    UNSERIALIZE_SCALAR(stack_min);
    UNSERIALIZE_SCALAR(next_thread_stack_base);
    UNSERIALIZE_SCALAR(mmap_start);
    UNSERIALIZE_SCALAR(mmap_end);
    UNSERIALIZE_SCALAR(nxm_start);
    UNSERIALIZE_SCALAR(nxm_end);
    pTable->unserialize(cp, section);
    for (int x = 0; x <= MAX_FD; x++) {
        fd_map[x].unserialize(cp, csprintf("%s.FdMap%d", section, x));
     }
    fix_file_offsets();

    checkpointRestored = true;

}


////////////////////////////////////////////////////////////////////////
//
// LiveProcess member definitions
//
////////////////////////////////////////////////////////////////////////


LiveProcess::LiveProcess(LiveProcessParams * params, ObjectFile *_objFile)
    : Process(params), objFile(_objFile),
      argv(params->cmd), envp(params->env), cwd(params->cwd)
{
    __uid = params->uid;
    __euid = params->euid;
    __gid = params->gid;
    __egid = params->egid;
    __pid = params->pid;
    __ppid = params->ppid;

    prog_fname = params->cmd[0];

    // load up symbols, if any... these may be used for debugging or
    // profiling.
    if (!debugSymbolTable) {
        debugSymbolTable = new SymbolTable();
        if (!objFile->loadGlobalSymbols(debugSymbolTable) ||
            !objFile->loadLocalSymbols(debugSymbolTable)) {
            // didn't load any symbols
            delete debugSymbolTable;
            debugSymbolTable = NULL;
        }
    }
}

void
LiveProcess::argsInit(int intSize, int pageSize)
{
    Process::startup();

    // load object file into target memory
    objFile->loadSections(initVirtMem);

    // Calculate how much space we need for arg & env arrays.
    int argv_array_size = intSize * (argv.size() + 1);
    int envp_array_size = intSize * (envp.size() + 1);
    int arg_data_size = 0;
    for (int i = 0; i < argv.size(); ++i) {
        arg_data_size += argv[i].size() + 1;
    }
    int env_data_size = 0;
    for (int i = 0; i < envp.size(); ++i) {
        env_data_size += envp[i].size() + 1;
    }

    int space_needed =
        argv_array_size + envp_array_size + arg_data_size + env_data_size;
    if (space_needed < 32*1024)
        space_needed = 32*1024;

    // set bottom of stack
    stack_min = stack_base - space_needed;
    // align it
    stack_min = roundDown(stack_min, pageSize);
    stack_size = stack_base - stack_min;
    // map memory
    pTable->allocate(stack_min, roundUp(stack_size, pageSize));

    // map out initial stack contents
    Addr argv_array_base = stack_min + intSize; // room for argc
    Addr envp_array_base = argv_array_base + argv_array_size;
    Addr arg_data_base = envp_array_base + envp_array_size;
    Addr env_data_base = arg_data_base + arg_data_size;

    // write contents to stack
    uint64_t argc = argv.size();
    if (intSize == 8)
        argc = htog((uint64_t)argc);
    else if (intSize == 4)
        argc = htog((uint32_t)argc);
    else
        panic("Unknown int size");

    initVirtMem->writeBlob(stack_min, (uint8_t*)&argc, intSize);

    copyStringArray(argv, argv_array_base, arg_data_base, initVirtMem);
    copyStringArray(envp, envp_array_base, env_data_base, initVirtMem);

    assert(NumArgumentRegs >= 2);
    threadContexts[0]->setIntReg(ArgumentReg[0], argc);
    threadContexts[0]->setIntReg(ArgumentReg[1], argv_array_base);
    threadContexts[0]->setIntReg(StackPointerReg, stack_min);

    Addr prog_entry = objFile->entryPoint();
    threadContexts[0]->setPC(prog_entry);
    threadContexts[0]->setNextPC(prog_entry + sizeof(MachInst));

#if THE_ISA != ALPHA_ISA //e.g. MIPS or Sparc
    threadContexts[0]->setNextNPC(prog_entry + (2 * sizeof(MachInst)));
#endif

    num_processes++;
}

void
LiveProcess::syscall(int64_t callnum, ThreadContext *tc)
{
    num_syscalls++;

    SyscallDesc *desc = getDesc(callnum);
    if (desc == NULL)
        fatal("Syscall %d out of range", callnum);

    desc->doSyscall(callnum, this, tc);
}

LiveProcess *
LiveProcess::create(LiveProcessParams * params)
{
    LiveProcess *process = NULL;

    string executable =
        params->executable == "" ? params->cmd[0] : params->executable;
    ObjectFile *objFile = createObjectFile(executable);
    if (objFile == NULL) {
        fatal("Can't load object file %s", executable);
    }

    if (objFile->isDynamic())
       fatal("Object file is a dynamic executable however only static "
             "executables are supported!\n       Please recompile your "
             "executable as a static binary and try again.\n");

#if THE_ISA == ALPHA_ISA
    if (objFile->hasTLS())
        fatal("Object file has a TLS section and single threaded TLS is not\n"
              "       currently supported for Alpha! Please recompile your "
              "executable with \n       a non-TLS toolchain.\n");

    if (objFile->getArch() != ObjectFile::Alpha)
        fatal("Object file architecture does not match compiled ISA (Alpha).");
    switch (objFile->getOpSys()) {
      case ObjectFile::Tru64:
        process = new AlphaTru64Process(params, objFile);
        break;

      case ObjectFile::Linux:
        process = new AlphaLinuxProcess(params, objFile);
        break;

      default:
        fatal("Unknown/unsupported operating system.");
    }
#elif THE_ISA == SPARC_ISA
    if (objFile->getArch() != ObjectFile::SPARC64 && objFile->getArch() != ObjectFile::SPARC32)
        fatal("Object file architecture does not match compiled ISA (SPARC).");
    switch (objFile->getOpSys()) {
      case ObjectFile::Linux:
        if (objFile->getArch() == ObjectFile::SPARC64) {
            process = new Sparc64LinuxProcess(params, objFile);
        } else {
            process = new Sparc32LinuxProcess(params, objFile);
        }
        break;


      case ObjectFile::Solaris:
        process = new SparcSolarisProcess(params, objFile);
        break;
      default:
        fatal("Unknown/unsupported operating system.");
    }
#elif THE_ISA == X86_ISA
    if (objFile->getArch() != ObjectFile::X86)
        fatal("Object file architecture does not match compiled ISA (x86).");
    switch (objFile->getOpSys()) {
      case ObjectFile::Linux:
        process = new X86LinuxProcess(params, objFile);
        break;
      default:
        fatal("Unknown/unsupported operating system.");
    }
#elif THE_ISA == MIPS_ISA
    if (objFile->getArch() != ObjectFile::Mips)
        fatal("Object file architecture does not match compiled ISA (MIPS).");
    switch (objFile->getOpSys()) {
      case ObjectFile::Linux:
        process = new MipsLinuxProcess(params, objFile);
        break;

      default:
        fatal("Unknown/unsupported operating system.");
    }
#elif THE_ISA == ARM_ISA
    if (objFile->getArch() != ObjectFile::Arm)
        fatal("Object file architecture does not match compiled ISA (ARM).");
    switch (objFile->getOpSys()) {
      case ObjectFile::Linux:
        process = new ArmLinuxProcess(params, objFile);
        break;

      default:
        fatal("Unknown/unsupported operating system.");
    }
#else
#error "THE_ISA not set"
#endif


    if (process == NULL)
        fatal("Unknown error creating process object.");
    return process;
}

LiveProcess *
LiveProcessParams::create()
{
    return LiveProcess::create(this);
}