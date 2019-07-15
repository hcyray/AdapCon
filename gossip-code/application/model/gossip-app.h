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

const int TOTAL_EPOCH_FOR_SIMULATION = 4;

const int AP_NUMBER = 18;
const int NODE_NUMBER = 52;
const int OUT_NEIGHBOR_NUMBER = 3;
const int MAX_IN_NEIGHBOR_NUMBER = 5;
const int BLOCK_PIECE_NUMBER =8; // 64~1MB
const float FIXED_EPOCH_LEN = 10;
const float DETERMINE_INTERVAL = 0.1;
const float TIMEOUT_FOR_TCP = 2.0;
const float TIMEOUT_FOR_SMALL_MSG = 1000;
const float CONSERVE_LEN = 10.0;
const float BYZANTINE_P = 0.33;



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
	int Self_Report_to_file();

	void RecordNeighbor();

	void if_leader();
	void record_block(int from_node);
	float InitializeEpoch();


	Block BlockPropose();
	void GossipBlockOut(Block b);
	void GossipBlockAfterReceive (int from_node, Block b);
	void RelayVotingMessage (int dest, Ptr<Packet> p);
	void SendBlockInv (int dest, Block b);
	void SendBlockSYN (int dest, Block b);
	void SendGetdata (int dest);
	void SendBlockAck (int dest);
	void SendBlock (int dest, Block b);
	void SendBlockPiece (int dest, int piece, Block b);
	void SendEcho(int dest);
	void SendVote(int source, int dest);
	void SendTime(int dest, int size);
	void SendTimeEcho(int dest);

	// void Send_data_watchdog(int n);
	void record_block_piece(int from_node);
	void Receive_data_monitor();

protected:
	virtual void DoDispose (void);


private:

	virtual void StartApplication (void);
	virtual void StopApplication (void);
	void HandleAccept(Ptr<Socket> s, const Address& from);
	void HandleRead (Ptr<Socket> socket);
	// void try_tcp_connect(int dest);
	// void reply_tcp_connect(int dest);
	
	void GetOutNeighbor ();
	void GetAllNeighbor ();
	void ReadNeighborFromFile ();
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
	int consensus_success;
	bool block_got;
	bool gossip_action;
	float block_got_time;
	float cur_epoch_len;
	float m_epoch_beginning;
	int in_neighbor_number;
	int neighbor_number;
	bool remote_state;

	float time_inv;
	float time_getdata;
	float time_block;
	float time_ack;

	int neighbor_required;
	float wait_for_SYN;
	float wait_for_BLOCK;
	float wait_SYN_start_time;
	float wait_BLOCK_start_time;

	bool attacker_pretend_lost;
	bool attacker_do_not_send;
	int receive_data_monitor_trigger;
	float watchdog_timeout;


	// Ptr<Socket> socket_receive;
	std::vector<Ptr<Socket>> m_socket_receive;
	std::vector<Ptr<Socket>> m_socket_send;
	std::vector<int> in_neighbor_choosed;
	std::vector<int> out_neighbor_choosed;
	std::vector<int> neighbor_choosed;
	std::vector<Msg_INV> vec_INV;
	std::vector<Msg_SYN> vec_SYN;
	std::vector<Msg_ACK> vec_ACK;
	std::vector<int> vec_nodes_sendme_inv;
	std::vector<int> order_reply_inv;

	EventId id0;
	EventId id1;
	EventId id2;
	EventId id3;
	EventId id4;
	EventId id_end;


	std::map<uint8_t, Ipv4Address> map_node_addr;
	std::map<Ipv4Address, uint8_t> map_addr_node; 
	std::map<int, int> map_node_blockrcv_state;
	std::map<int, float> map_node_blockrcvtime;
	std::map<int, std::map<int, int> > map_node_blockpiece_received;
	std::map<int, float> map_node_block_rcv_time;
	std::vector<float> vec_block_rcv_time;
	std::map<int, int> map_node_vote;
	
	std::map<int, float> map_node_onehoptime;
	std::vector<float> vec_one_hop_time;
	std::map<int, std::vector<float> > map_node_trustval;
	std::map<int, int> map_node_failedSYN;
	std::vector<int> formal_res_quorum;
	std::vector<int> curr_res_quorum;
	
	
	// std::ofstream log_link_file;
	std::ofstream self_report_file;

	float delta;
	float srtt;
	float rttvar;
	float rtt;

	void Read_data();
	float Set_timeout();
	float Epoch_len_computation();
	void Res_quorum_update();
	void Trust_val_update();
	float Kickout_malicious();
	float timeout_probability(float x);
	float failrcv_probability(int n);



};

} // namespace ns3

#endif