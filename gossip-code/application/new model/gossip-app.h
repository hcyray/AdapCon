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
#include <fstream>
#include "/home/hqw/repos/ns-3-allinone/ns-3-dev/scratch/data-struct.h"

namespace ns3 {

class Socket;
class Packet;

const int TOTAL_EPOCH_FOR_SIMULATION = 10;

const int AP_NUMBER = 18;
const int NODE_NUMBER = 52;
const int OUT_GOSSIP_ROUND = 5;
const int BLOCK_PIECE_NUMBER = 2;
const float FIXED_EPOCH_LEN = 2.5;
const float DETERMINE_INTERVAL = 0.1;




class GossipApp: public Application
{
public:
	static TypeId GetTypeId(void);
	GossipApp();
	virtual ~GossipApp();
	void ConsensProcess();

	void GossipEcho();
	void GossipTime();
	void GossipTimeEcho();
	void GossipVote();

	void if_leader();
	void record_block(int from_node);
	float InitializeEpoch();

	Block BlockPropose();
	void GossipBlockOut(Block b);
	void GossipBlockAfterReceive (int from_node, Block b);
	void SendBlock (int dest, Block b);
	void SendBlockPiece (int dest, int piece, Block b);


protected:
	virtual void DoDispose (void);


private:

	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void HandleAccept(Ptr<Socket> s, const Address& from);
	void HandleRead (Ptr<Socket> socket);
	
	void GetNeighbor (int node_number, int out_neighbor_choosed[]);
	uint8_t GetNodeId(void);
	std::vector<std::string> SplitMessage (const std::string &str, const char pattern);

	int m_node_id;
	uint16_t m_port;
	int m_epoch;
	bool m_leader;
	Block block_received;
	Block EMPTY_BLOCK;
	Block GENESIS_BLOCK;
	uint32_t block_name;
	int block_height;
	std::vector<uint32_t> m_local_ledger;
	std::vector<int> m_ledger_built_epoch;

	int view;
	bool block_got;
	bool gossip_action;
	float block_got_time;
	float cur_epoch_len;
	float m_epoch_beginning;

	std::vector<Ptr<Socket>> m_socket_receive;
	std::vector<Ptr<Socket>> m_socket_send;
	int out_neighbor_choosed[OUT_GOSSIP_ROUND];

	EventId id0;
	EventId id1;
	EventId id2;
	EventId id3;
	EventId id4;

	std::map<uint8_t, Ipv4Address> map_node_addr;
	std::map<Ipv4Address, uint8_t> map_addr_node; 
	std::map<int, std::map<int, int> > map_node_blockpiece_received;
	std::map<int, int> map_node_blockrcvcase;
	std::map<int, float> map_node_blockrcvtime;
	std::map<int, int> map_node_vote;
	std::map<int, float> map_node_recvtime;

	std::ofstream log_time_file;
	std::ofstream log_rep_file;
	std::ofstream log_link_file;
};

} // namespace ns3

#endif