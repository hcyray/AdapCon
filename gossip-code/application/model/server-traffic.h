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

#ifndef SERVER_TRAFFIC_H
#define SERVER_TRAFFIC_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/address.h"
#include "ns3/ipv4-address.h"
#include "ns3/traced-callback.h"
#include <map>
#include <string>
#include <vector>


namespace ns3 {

class Socket;
class Packet;



class ServerTraffic : public Application 
{
public:

  static TypeId GetTypeId (void);
  ServerTraffic ();
  virtual ~ServerTraffic ();
  std::vector<std::string> SplitMessage(const std::string& str, const char pattern);


protected:
  virtual void DoDispose (void);


private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void HandleAccept(Ptr<Socket> s, const Address& from);
  void HandleTraffic(Ptr<Socket> socket);


  Ptr<Socket> m_socket;
  uint16_t m_port;
  float total_traffic;
  float xx;

  /// Callbacks for tracing the packet Rx events
  // TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  // TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* SERVER_TRAFFIC_H */

