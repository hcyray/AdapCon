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

#ifndef GOSSIP_APP_H
#define GOSSIP_APP_H

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

const int TOTAL_EPOCH_FOR_SIMULATION = 3;

const int AP_NUMBER = 10;
const int NODE_NUMBER = 33;
const int FAN_OUT = 4;
const int OUT_GOSSIP_ROUND = 5;
const int IN_GOSSIP_ROUND = 3;
const int SOLICIT_ROUND = 1;
const int SOLICIT_INTERVAL = 5;
const double DETERMINECOMMIT_INTERVAL = 0.2;
const double DETERMINECONSENS_INTERVAL = 0.2;

const uint8_t TYPE_BLOCK[1000] = "BLOCK";
const uint8_t TYPE_SOLICIT[80] = "SOLICIT";
const uint8_t TYPE_ACK[80] = "ACK";
const uint8_t TYPE_PREPARE[80] = "PREPARE";
const uint8_t TYPE_COMMIT[80] = "COMMIT";



enum MESSAGE_TYPE {BLOCK, SOLICIT, ACK, PREPARE, COMMIT};




class GossipApp : public Application 
{
public:
  
  static TypeId GetTypeId (void);
  GossipApp ();
  virtual ~GossipApp ();
  void ConsensProcess();

  Ptr<Packet> ComputeWhatToSend();
  // void ChooseNeighbor(int number, int x[]);
  // void ChooseNeighbor(int number, int x[], int node_excluded);
  void GetNeighbor(int n, int x[]);
  uint8_t GetNodeId(void);
  void if_leader(void);
  void if_get_block(void);

  void ScheduleTransmit (Time dt, int dest, int type);
  void GossipBlockOut();
  void GossipBlockAfterReceive(int from_node);
  void GossipVotingMessageOut(int type);
  void RelayVotingMessage(Ptr<Packet> p);
  void DetermineCommit();
  void DetermineConsens();
  void SolicitMessageFromOthers();
  std::string MessagetypeToString(int x);
  std::vector<std::string> SplitMessage(const std::string& str, const char pattern);


protected:
  virtual void DoDispose (void);


private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void Send (int dest, MESSAGE_TYPE);
  void SendBlock(int dest);
  void HandleRead (Ptr<Socket> socket);


  uint8_t m_node_id;
  uint16_t m_port; 
  std::map<uint8_t, Ipv4Address> map_node_addr;
  std::map<Ipv4Address, uint8_t> map_addr_node; 
  std::map<int, int> map_epoch_consensed;


  // uint32_t m_sent;
  // uint32_t m_count;

  uint8_t m_height;
  uint8_t m_epoch;
  double len_phase1;
  double len_phase2;
  double waitting_time;

  // bool current_consensus_success;
  bool m_leader;
  bool block_got;
  std::map<int, int> map_piece_received;
  std::map<int, int> map_node_PREPARE;
  std::map<int, int> map_node_COMMIT;
  std::map<double, double> map_node_BLOCK_time;
  std::map<double, double> map_node_PREPARE_time;
  std::map<double, double> map_node_COMMIT_time;

  // int neighborsforpush[OUT_GOSSIP_ROUND];
  // int neighborsforpull[SOLICIT_ROUND]; 
  Ptr<Socket> m_socket_receive; 
  std::vector<Ptr<Socket>> m_socket_send;
  int out_neighbor_choosed[OUT_GOSSIP_ROUND];
  int in_neighbor_choosed[IN_GOSSIP_ROUND];

  Time m_interval = Seconds(2.0); 
  double m_epoch_beginning;
  // EventId m_sendEvent;
  
  

  /// Callbacks for tracing the packet Rx events
  // TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  // TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* GOSSIP_APP_H */

