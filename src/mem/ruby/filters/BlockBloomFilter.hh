
/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
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

/*
 * BlockBloomFilter.hh
 *
 * Description:
 *
 *
 */

#ifndef BLOCK_BLOOM_FILTER_H
#define BLOCK_BLOOM_FILTER_H

#include "mem/gems_common/Map.hh"
#include "mem/ruby/common/Global.hh"
#include "mem/ruby/common/Address.hh"
#include "mem/ruby/filters/AbstractBloomFilter.hh"

class BlockBloomFilter : public AbstractBloomFilter {
public:

  ~BlockBloomFilter();
  BlockBloomFilter(string config);

  void clear();
  void increment(const Address& addr);
  void decrement(const Address& addr);
  void merge(AbstractBloomFilter * other_filter);
  void set(const Address& addr);
  void unset(const Address& addr);

  bool isSet(const Address& addr);
  int getCount(const Address& addr);
  int getTotalCount();
  int getIndex(const Address& addr);
  int readBit(const int index);
  void writeBit(const int index, const int value);

  void print(ostream& out) const;

private:

  int get_index(const Address& addr);

  Vector<int> m_filter;
  int m_filter_size;
  int m_filter_size_bits;

  int m_count_bits;
  int m_count;
};


#endif
