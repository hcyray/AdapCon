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

const int TOTAL_EPOCH_FOR_SIMULATION = 20;

const int AP_NUMBER = 18;
const int NODE_NUMBER = 52;
const int OUT_NEIGHBOR_NUMBER = 3;
const int MAX_IN_NEIGHBOR_NUMBER = 9;
const int BLOCK_PIECE_NUMBER = 8;
const float FIXED_EPOCH_LEN = 2.5;
const float DETERMINE_INTERVAL = 0.1;
const float TIMEOUT_FOR_TCP = 2.0;




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
	void EndSummary();

	void RecordNeighbor();
	void RecordTime();

	void if_leader();
	void record_block(int from_node);
	float InitializeEpoch();

	Block BlockPropose();
	void GossipBlockOut(Block b);
	void GossipBlockAfterReceive (int from_node, Block b);
	void SendBlockInv (int dest);
	void SendGetdata (int dest);
	void SendBlockAck (int dest);
	void SendBlock (int dest, Block b);
	void SendBlockPiece (int dest, int piece, Block b);
	void SendEcho(int dest);
	void SendVote(int dest);
	void SendTime(int dest);
	void SendTimeEcho(int dest);

	void Send_data_watchdog(int n);
	void Receive_data_watchdog(int n);

protected:
	virtual void DoDispose (void);


private:

	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void HandleAccept(Ptr<Socket> s, const Address& from);
	void HandleRead (Ptr<Socket> socket);
	bool try_tcp_connect(int dest);
	
	int GetOutNeighbor ();
	int GetAllNeighbor ();
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
	int in_neighbor_number;
	bool remote_state;

	float time_inv;
	float time_getdata;
	float time_block;
	float time_ack;

	bool attacker_pretend_lost;
	bool attacker_do_not_send;
	int time_query_for_block;
	float time_start_rcv_block;
	float time_end_rcv_block;

	std::map<int, float> map_node_getdata_time;
	std::map<int, float> map_node_ack_time;


	std::vector<Ptr<Socket>> m_socket_receive;
	std::vector<Ptr<Socket>> m_socket_send;
	std::vector<int> in_neighbor_choosed;
	std::vector<int> out_neighbor_choosed;
	std::vector<int> neighbor_choosed;
	std::vector<int> vec_nodes_sendme_inv;

	EventId id0;
	EventId id1;
	EventId id2;
	EventId id3;
	EventId id4;
	EventId id_end;

	std::map<uint8_t, Ipv4Address> map_node_addr;
	std::map<Ipv4Address, uint8_t> map_addr_node; 
	std::map<int, std::map<int, int> > map_node_blockpiece_received;
	std::map<int, int> map_node_blockrcv_state;
	std::map<int, float> map_node_blockrcvtime;
	std::map<int, int> map_node_vote;
	std::map<int, float> map_node_recvtime;
	std::map<int, float> map_sourece_node_blockrevtime;

	std::ofstream log_time_file;
	std::ofstream log_rep_file;
	std::ofstream log_link_file;
};

} // namespace ns3

#endif