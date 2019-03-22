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
  // std::cout<<"Server starts"<<std::endl;
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
      // m_socket->ShutdownSend();
    }
  m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address &> (), 
    MakeCallback(&ServerTraffic::HandleAccept, this));
  m_socket->SetRecvCallback (MakeCallback (&ServerTraffic::HandleTraffic, this));

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
  // float sum = total_traffic / (1024*1024);
  // float sum = total_traffic;
  // std::cout<<"total traffic received in server: "<<sum<< "Byte server stops at "<<Simulator::Now().GetSeconds()<<"s" << std::endl;
}



void ServerTraffic::HandleAccept(Ptr<Socket> s, const Address& from)
{
  // std::cout<<"Server handles accept at: "<<Simulator::Now().GetSeconds()<<"s"<<std::endl;
  s->SetRecvCallback (MakeCallback (&ServerTraffic::HandleTraffic, this));
}


// void
// ServerTraffic::HandleAccept(Ptr<Socket> socket)
// {
//   socket->SetRecvCallback(MakeCallback(&ServerTraffic::HandleTraffic, this));
// }

void 
ServerTraffic::HandleTraffic (Ptr<Socket> socket)
{
  // NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      uint8_t content_[30];
      packet->CopyData(content_, 30);
      std::string str_of_content (content_, content_ + 30);
      std::vector<std::string> res = SplitMessage (str_of_content, '+');
      float traffic_queried = (atoi) (res[1].c_str ());
      // std::cout<<"Server received "<<packet->GetSize()<<" bytes "<<content_<<" from "<<InetSocketAddress::ConvertFrom (from).GetIpv4 ()<<
      //   " at "<<Simulator::Now().GetSeconds() <<"s"<<std::endl;
      // total_traffic += packet->GetSize();
      // std::cout<<"Echoing packet"<<std::endl;
      // socket->SendTo (packet, 0, from);
      int loop_number = traffic_queried / (2048);
      for(int l=0; l<loop_number; l++)
      {
        Ptr<Packet> p;
        std::string str1 = "MOBILE_DOWNLOAD_TRAFFIC+";
        str1.append(std::to_string(l));
        const uint8_t *buffer = reinterpret_cast<const uint8_t *> (str1.c_str ());
        p = Create<Packet> (buffer, 2048);
        socket->Send(p);
      }
      
      // std::string str_of_content(content_, content_+20);
      // std::vector<std::string> res = SplitMessage(str_of_content, '+');
      // const char* time_of_recived_message = res[0].c_str();
    }
}

} // Namespace ns3
