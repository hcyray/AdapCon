/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"
#include "ns3/pointer.h"

#include "server-traffic.h"

#include <stdlib.h>
#include <time.h>
#include <algorithm>
#include <fstream>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ServerTrafficApplication");

NS_OBJECT_ENSURE_REGISTERED (ServerTraffic);



TypeId
ServerTraffic::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ServerTraffic")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<ServerTraffic> ()
    // .AddAttribute ("NodeId", "Node on which the application runs.",
    //                UintegerValue (0),
    //                MakeUintegerAccessor (&ServerTraffic::m_node_id),
    //                MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&ServerTraffic::m_port),
                   MakeUintegerChecker<uint16_t> ())
    // .AddTraceSource ("Rx", "A packet has been received",
    //                  MakeTraceSourceAccessor (&ServerTraffic::m_rxTrace),
    //                  "ns3::Packet::TracedCallback")
    // .AddTraceSource ("RxWithAddresses", "A packet has been received",
    //                  MakeTraceSourceAccessor (&ServerTraffic::m_rxTraceWithAddresses),
    //                  "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

ServerTraffic::ServerTraffic ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

}



ServerTraffic::~ServerTraffic()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
}



std::vector<std::string> ServerTraffic::SplitMessage(const std::string& str, const char pattern)
{
    std::vector<std::string> res;
    std::stringstream input(str);   
    std::string temp;
    while(getline(input, temp, pattern))
    {
        res.push_back(temp);
    }
    return res;
}

void
ServerTraffic::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
ServerTraffic::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  std::cout<<"Server starts"<<std::endl;

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::TcpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if(m_socket->Listen()==0)
        std::cout<<"Server is listening!"<<std::endl;
      m_socket->ShutdownSend();
    }
  m_socket->SetAcceptCallback (MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
    MakeCallback (&ServerTraffic::HandleAccept, this));

}

void 
ServerTraffic::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  // printf("%s\n", "Server stops!");
  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}



void ServerTraffic::HandleAccept(Ptr<Socket> s, const Address& from)
{
  s->SetRecvCallback (MakeCallback (&ServerTraffic::HandleTraffic, this));
}



void 
ServerTraffic::HandleTraffic (Ptr<Socket> socket)
{
  // NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      // uint8_t content_[20];
      // packet->CopyData(content_, 20);
      // Ipv4Address from_addr = InetSocketAddress::ConvertFrom (from).GetIpv4 ();
      // int from_node = (int)map_addr_node[from_addr];
      // std::cout<<"node "<<(int)GetNodeId()<<" received a "<<content_<<" "<<packet->GetSize()
      //   <<" bytes from node "<<from_node<<" at "<<Simulator::Now().GetSeconds()<<" s"<<std::endl;
      std::cout<<"Server received "<<packet->GetSize()<<" bytes from "<<InetSocketAddress::ConvertFrom (from).GetIpv4 ()<<std::endl;

      // std::string str_of_content(content_, content_+20);
      // std::vector<std::string> res = SplitMessage(str_of_content, '+');
      // const char* time_of_recived_message = res[0].c_str();
    }
}

} // Namespace ns3
