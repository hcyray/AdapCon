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

#ifndef UDP_Communicate_SERVER_H
#define UDP_Communicate_SERVER_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include <map>
#include <string>



namespace ns3 {

class Socket;
class Packet;

const uint8_t TYPE_SOLICIT = 21;
const uint8_t TYPE_ACK = 23;
const uint8_t TYPE_BLOCK[10240] = "BLOCK";
const uint8_t TYPE_PREPARE[80] = "PREPARED";
const uint8_t TYPE_COMMIT[80] = "COMMIT";

/**
 * \ingroup applications 
 * \defgroup udpcommunicate Udpcommunicate
 */

/**
 * \ingroup udpcommunicate
 * \brief A Udp communicate server
 *
 * Every packet received is sent back.
 */
class UdpCommunicateServer : public Application 
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  UdpCommunicateServer ();
  virtual ~UdpCommunicateServer ();

  Ptr<Packet> ComputePacketToSend();
  uint8_t GetNodeId(void);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void Send (void);
  void ScheduleTransmit (Time dt);
  void HandleRead (Ptr<Socket> socket);

  uint8_t node_id;
  uint16_t m_port; //!< Port on which we listen for incoming packets.
  std::map<uint8_t, Ipv4Address> map_node_addr;
  uint32_t m_sent;
  uint32_t m_count= 1;

  Ptr<Socket> m_socket_receive; //!< IPv4 Socket
  Ptr<Socket> m_socket_send;

  Time m_interval = Seconds(2.0); 
  EventId m_sendEvent;
  
  

  /// Callbacks for tracing the packet Rx events
  // TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  // TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* UDP_Communicate_SERVER_H */

