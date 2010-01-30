
/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * Copyright (c) 2009 Advanced Micro Devices, Inc.
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

#ifndef RUBY_TESTER_H
#define RUBY_TESTER_H

#include "mem/ruby/common/Global.hh"
#include "mem/mem_object.hh"
#include "cpu/rubytest/CheckTable.hh"
#include "mem/ruby/system/RubyPort.hh"
#include "mem/ruby/common/SubBlock.hh"
#include "mem/ruby/common/DataBlock.hh"
#include "mem/packet.hh"
#include "params/RubyTester.hh"

class RubyTester : public MemObject 
{

 public:

  class CpuPort : public SimpleTimingPort
  {
    RubyTester *tester;

   public:
    
    CpuPort(const std::string &_name, 
            RubyTester *_tester,
            int _idx)
      : SimpleTimingPort(_name, _tester), tester(_tester), idx(_idx)
    {}

    int idx;
    
   protected:
    
    virtual bool recvTiming(PacketPtr pkt);

    virtual Tick recvAtomic(PacketPtr pkt);
    
  };

  struct SenderState : public Packet::SenderState
  {
    SubBlock* subBlock;
    Packet::SenderState *saved;
    
    SenderState(Address addr,
                int size,
                Packet::SenderState *sender_state = NULL)
      : saved(sender_state)
    {subBlock = new SubBlock(addr, size);}

    ~SenderState() {delete subBlock;}
  };

  typedef RubyTesterParams Params;
  // Constructors
  RubyTester(const Params *p);

  // Destructor
  ~RubyTester();
  
  // Public Methods
  
  virtual Port *getPort(const std::string &if_name, int idx = -1);

  Port* getCpuPort(int idx);

  void virtual init();

  void wakeup();

  void incrementCheckCompletions() { m_checks_completed++; }

  void printStats(ostream& out) const {}
  void clearStats() {}
  void printConfig(ostream& out) const {}

  void print(ostream& out) const;

 protected:
  class CheckStartEvent : public Event
  {
  private:
    RubyTester *tester;
    
  public:
    CheckStartEvent(RubyTester *_tester) : Event(CPU_Tick_Pri), tester(_tester) {}
    void process() { tester->wakeup(); }
    virtual const char *description() const { return "RubyTester tick"; }
  };
  
  CheckStartEvent checkStartEvent;


 private:
  // Private Methods

  void hitCallback(NodeID proc, SubBlock* data);

  void checkForDeadlock();

  // Private copy constructor and assignment operator
  RubyTester(const RubyTester& obj);
  RubyTester& operator=(const RubyTester& obj);
  
  // Data Members (m_ prefix)
  
  CheckTable* m_checkTable_ptr;
  Vector<Time> m_last_progress_vector;

  uint64 m_checks_completed;
  std::vector<CpuPort*> ports;
  uint64 m_checks_to_complete;
  int m_deadlock_threshold;
  int m_num_cpu_sequencers;
  int m_wakeup_frequency;
};

// Output operator declaration
ostream& operator<<(ostream& out, const RubyTester& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const RubyTester& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //RUBY_TESTER_H
