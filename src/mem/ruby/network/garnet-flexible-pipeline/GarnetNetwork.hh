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
 * GarnetNetwork.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef GARNET_NETWORK_H
#define GARNET_NETWORK_H

#include "mem/ruby/network/garnet-fixed-pipeline/NetworkHeader.hh"
#include "mem/gems_common/Vector.hh"
#include "mem/ruby/network/garnet-flexible-pipeline/NetworkConfig.hh"
#include "mem/ruby/network/Network.hh"

class NetworkInterface;
class MessageBuffer;
class Router;
class Topology;
class NetDest;
class NetworkLink;

class GarnetNetwork : public Network{
public:
        GarnetNetwork(int nodes);

        ~GarnetNetwork();

        // returns the queue requested for the given component
        MessageBuffer* getToNetQueue(NodeID id, bool ordered, int network_num);
        MessageBuffer* getFromNetQueue(NodeID id, bool ordered, int network_num);

        void clearStats();
        void printStats(ostream& out) const;
        void printConfig(ostream& out) const;
        void print(ostream& out) const;

        bool isVNetOrdered(int vnet) { return m_ordered[vnet]; }
        bool validVirtualNetwork(int vnet) { return m_in_use[vnet]; }

        Time getRubyStartTime();
        int getNumNodes(){ return m_nodes; }

        void reset();

        // Methods used by Topology to setup the network
        void makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight,  int bw_multiplier, bool isReconfiguration);
        void makeInLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration);
        void makeInternalLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration);

private:
        void checkNetworkAllocation(NodeID id, bool ordered, int network_num);

// Private copy constructor and assignment operator
        GarnetNetwork(const GarnetNetwork& obj);
        GarnetNetwork& operator=(const GarnetNetwork& obj);

/***********Data Members*************/
        int m_virtual_networks;
        int m_nodes;

        Vector<bool> m_in_use;
        Vector<bool> m_ordered;

        Vector<Vector<MessageBuffer*> > m_toNetQueues;
        Vector<Vector<MessageBuffer*> > m_fromNetQueues;

        Vector<Router *> m_router_ptr_vector;   // All Routers in Network
        Vector<NetworkLink *> m_link_ptr_vector; // All links in the network
        Vector<NetworkInterface *> m_ni_ptr_vector;     // All NI's in Network

        Topology* m_topology_ptr;
        Time m_ruby_start;
};

// Output operator declaration
ostream& operator<<(ostream& out, const GarnetNetwork& obj);

// ******************* Definitions *******************
// Output operator definition
extern inline
ostream& operator<<(ostream& out, const GarnetNetwork& obj)
{
        obj.print(out);
        out << flush;
        return out;
}

#endif //NETWORK_H
