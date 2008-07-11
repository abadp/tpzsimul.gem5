/*
 * Copyright (c) 2005 The Regents of The University of Michigan
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
 * Authors: Ron Dreslinski
 *          Steve Reinhardt
 */

/**
 * @file
 * Stride Prefetcher template instantiations.
 */

#include "mem/cache/prefetch/stride.hh"

void
StridePrefetcher::calculatePrefetch(PacketPtr &pkt, std::list<Addr> &addresses,
                                    std::list<Tick> &delays)
{
//	Addr blkAddr = pkt->paddr & ~(Addr)(this->blkSize-1);
    int cpuID = pkt->req->getCpuNum();
    if (!useCPUId) cpuID = 0;

    /* Scan Table for IAddr Match */
/*	std::list<strideEntry*>::iterator iter;
  for (iter=table[cpuID].begin();
  iter !=table[cpuID].end();
  iter++) {
  if ((*iter)->IAddr == pkt->pc) break;
  }

  if (iter != table[cpuID].end()) {
  //Hit in table

  int newStride = blkAddr - (*iter)->MAddr;
  if (newStride == (*iter)->stride) {
  (*iter)->confidence++;
  }
  else {
  (*iter)->stride = newStride;
  (*iter)->confidence--;
  }

  (*iter)->MAddr = blkAddr;

  for (int d=1; d <= degree; d++) {
  Addr newAddr = blkAddr + d * newStride;
  if (this->pageStop &&
  (blkAddr & ~(TheISA::VMPageSize - 1)) !=
  (newAddr & ~(TheISA::VMPageSize - 1)))
  {
  //Spanned the page, so now stop
  this->pfSpanPage += degree - d + 1;
  return;
  }
  else
  {
  addresses.push_back(newAddr);
  delays.push_back(latency);
  }
  }
  }
  else {
  //Miss in table
  //Find lowest confidence and replace

  }
*/
}