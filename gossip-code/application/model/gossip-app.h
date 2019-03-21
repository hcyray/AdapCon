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
#include <queue>
#include "/home/hqw/repos/ns-3-allinone/ns-3-dev/scratch/data-struc.h"


namespace ns3 {

class Socket;
class Packet;

const int TOTAL_EPOCH_FOR_SIMULATION = 12;

const int AP_NUMBER = 5;
const int NODE_NUMBER = 16;
const int OUT_GOSSIP_ROUND = 5;
const int BLOCK_PIECE_NUMBER = 24;
const int IN_GOSSIP_ROUND = 3;
const int SOLICIT_ROUND = 1;
const int SOLICIT_INTERVAL = 10;
const float DETERMINECOMMIT_INTERVAL = 0.3;
const float DETERMINECONSENS_INTERVAL = 0.3;


const int WINDOW_SIZE = 2;
const float EPSILON1 = 5.0;
const float EPSILON2 = 3.0;
const int PATCH = 3;



enum MESSAGE_TYPE {BLOCK, SOLICIT, ACK, PREPARE, COMMIT, REPUTATION};




class GossipApp : public Application 
{
public:
  
  static TypeId GetTypeId (void);
  GossipApp ();
  virtual ~GossipApp ();
  void ConsensProcess();

  void GetNeighbor(int n, int x[]);
  uint8_t GetNodeId(void);
  void if_leader(void);
  void if_get_block(void);

  void ScheduleTransmit (Time dt, int dest, int type);
  void InitializeTimeMessage();
  std::pair<float, float> InitializeEpoch();
  void InitializeState();
  void EndSummary();

  std::pair<float, float> NewLen();
  void UpdateCRGain();
  void UpdateCR();
  void UpdateBR();
  float DistanceOfPermu(int i, std::vector<float> v1, std::vector<float> v2);
  float AvgByCR(int node, std::vector<float> vec_CR, std::map<int, float> map_node_time);


  Block BlockPropose();
  void LeaderGossipBlockOut(Block b);
  void GossipViewplusplusMsg();
  void GossipPrepareOut();
  void GossipBlockAfterReceive(int from_node, Block b);
  void GossipTimeMessage(int t);
  void RelayVotingMessage(int dest, Ptr<Packet> p);
  void RelayTimeMessage(int dest, Ptr<Packet> p);
  void GossipCommitOut();
  void DetermineConsens();
  void RelayViewpluplusMessage(int dest, Ptr<Packet> p);
  void ReplyHistorySolicit(int dest, int h);
  void RecoverHistory(std::vector<uint32_t> b, std::vector<int> b_epo, int dest);
  void SolicitConsensusMessageFromOthers();

  void SilenceAttack();
  void InductionAttack();

  std::string MessagetypeToString(int x);
  std::vector<std::string> SplitMessage(const std::string& str, const char pattern);


protected:
  virtual void DoDispose (void);


private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);
  void SendBlock (int dest, Block b);
  void SendBlockPiece (int dest, int piece, Block b);
  void SendBlockAck(int dest, Block b);
  void SendBlockAckPiece(int dest, int piece, Block b);
  void SendPrepare(int dest, Block b);
  void SendCommit(int dest, Block b);
  void SendTimeMessage(int dest, int t);
  void SendViewplusplus(int dest);
  void SolicitHistory(int dest, int h);
  void SolicitBlock(int dest);
  void HandleRead (Ptr<Socket> socket);


  uint8_t m_node_id;
  uint16_t m_port; 
  std::map<uint8_t, Ipv4Address> map_node_addr;
  std::map<Ipv4Address, uint8_t> map_addr_node; 
  std::map<int, int> map_epoch_consensed;
  std::vector<uint32_t> m_local_ledger;  //block name 
  std::vector<int> m_ledger_built_epoch;  //in which epoch the block was added

  // uint32_t m_sent;
  // uint32_t m_count;

  int m_epoch;
  float len_phase1;
  float len_phase2;
  float waitting_time;

  // bool current_consensus_success;
  bool m_leader;
  bool block_got;
  bool consensed_this_epoch;
  int view;
  int receive_viewplusplus_time;
  int receive_history;
  int state;  //  state = 0,1,2,3, means Init, Ped, Ced, Tced
  State_Quad quad;
  Block block_received;
  Block EMPTY_BLOCK;
  Block GENESIS_BLOCK;
  uint32_t block_name;
  int block_height;
  std::map<int, int> map_blockpiece_received;
  std::map<int, int> map_node_PREPARE;
  std::map<int, int> map_node_COMMIT;
  std::map<float, float> map_node_BLOCK_time;
  std::map<float, float> map_node_PREPARE_time;
  std::map<float, float> map_node_COMMIT_time;

  // int neighborsforpush[OUT_GOSSIP_ROUND];
  // int neighborsforpull[SOLICIT_ROUND]; 
  Ptr<Socket> m_socket_receive;
  std::vector<Ptr<Socket>> m_socket_send;
  int out_neighbor_choosed[OUT_GOSSIP_ROUND];
  int in_neighbor_choosed[IN_GOSSIP_ROUND];

  Time m_interval = Seconds(2.0); 
  float m_epoch_beginning;
  std::pair<float, float> m_len;
  // EventId m_sendEvent;
  EventId id1;
  EventId id2;
  EventId id3;
  EventId id4;
  
  int get_block_or_not;
  int get_committed_or_not;
  float get_block_time;
  float get_prepared_time;
  float get_committed_time;
  
  std::map<int, int> map_node_getblockornot;
  std::map<int, int> map_node_getcommitedornot;
  std::map<int, float> map_node_getblocktime;
  std::map<int, float> map_node_getpreparedtime;
  std::map<int, float> map_node_getcommittedtime;
  
  
  std::map<int, std::map<int, int> > map_epoch_node_getblockornot;
  std::map<int, std::map<int, int> > map_epoch_node_getcommitedornot;
  std::map<int, std::map<int, float> > map_epoch_node_getblocktime;
  std::map<int, std::map<int, float> > map_epoch_node_getpreparedtime;
  std::map<int, std::map<int, float> > map_epoch_node_getcommittedtime;
  std::map<int, float> map_epoch_len_phase1;
  std::map<int, float> map_epoch_len_phase2;
  std::map<int, float> map_epoch_start_time;
  std::map<int, float> map_epoch_viewchange_happen;

  std::map<int, std::map<int, float> > map_epoch_node_CR_gain;
  std::map<int, std::map<int, float> > map_epoch_node_CR;
  std::map<int, float> map_node_BR;
  
  

  /// Callbacks for tracing the packet Rx events
  // TracedCallback<Ptr<const Packet> > m_rxTrace;

  /// Callbacks for tracing the packet Rx events, includes source and destination addresses
  // TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;
};

} // namespace ns3

#endif /* GOSSIP_APP_H */


