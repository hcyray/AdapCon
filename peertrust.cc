#include <iostream>
#include <time>
#include <vector>
#include <set>
#include <map>
#include <random>


const int NUM_NODE = 50
const int NUM_GOOD = 40
const float PRICE = 10
const int OMIGA = 2

class TX
{
public:
	int p1;
	int p2;
	float score;
	float price;

	TX(int m_p1, int m_p2, float m_score, float m_price)
	{
		p1 = m_p1;
		p2 = m_p2;
		socre = m_score;
		price = m_price;
	}
}

int TX_gen(int p1, int p2)
{
	if(p1<NUM_GOOD && p2<NUM_GOOD)
        res = TX(p1, p2, 1., 10);
    else if(p1<NUM_GOOD && p2>=NUM_GOOD)
        res = TX(p1, p2, 0., 10);
    else if(p1>=NUM_GOOD && p2<NUM_GOOD)
        res = TX(p1, p2, 0., 10);
    else
        res = TX(p1, p2, 1., 10);
    return res;
}

vector<TX> Extract_tx_for_node(int node, vector<TX> TX_list)
{
	vector<TX> res;
	int ite = 0;
	for(; ite<TX_list.size(); ite++)
	{
		if(TX_list[ite].p1==node)
			res.push_back(TX_list[ite]);
	}
	return res;
}

vector<TX> Extract_tx_for_two_nodes(int node1, int node2, vector<TX> TX_list)
{
	vector<TX> res;
	int ite = 0;
	for(; ite<TX_list.size(); ite++)
	{
		if(TX_list[ite].p1==node1 && TX_list[ite].p2==node2)
			res.push_back(TX_list[ite]);
	}
	return res;
}

vector<int> Extract_s_for_node(int node, vector<TX> TX_list)
// those who eval node
{
	vector<int> res;
	int ite = 0;
	for(; ite<TX_list.size(); ite++)
	{
		if(TX_list[ite].p1==node)
			res.push_back(TX_list[ite].p2);
	}
	// remove duplicate
}

vector<int> Extract_js_for_two_nodes(int node1, int node2, vector<TX> TX_list)
{
	vector<int> res1;
	vector<int> res2;
	for(int ite=0; ite<TX_list.size(); ite++)
	{
		if(TX_list[ite].p2==node1)
			res1.push_back(TX_list[ite]);
		if(TX_list[ite].p2==node1)
			res2.push_back(TX_list[ite]);
	}
	// remove duplicate and joint set
}


float Similarity_of_two_nodes(int node1, int node2, vector<TX> TX_list)
{
	if(node1==node2)
		return 1;
	else
	{
		vector<int> joint_node_set = Extract_js_for_two_nodes(node1, node2, TX_list);
		if(joint_node_set.size()==0)
			return 0;
		else
		{
			int summ = 0;
			for(int ite=0; ite<joint_node_set.size(); ite++)
			{
				vector<TX> tx_list_x_node1 = Extract_tx_for_two_nodes(x, node1, TX_list);
				vector<TX> tx_list_x_node2 = Extract_tx_for_two_nodes(x, node2, TX_list);
				vector<float> eval_node1_to_x;
				vector<float> eval_node2_to_x;
				for(int ite=0; ite<tx_list_x_node1.size(); ite++)
					eval_node1_to_x.push_back(tx_list_x_node1[ite].score);
				for(int ite=0; ite<tx_list_x_node2.size(); ite++)
					eval_node2_to_x.push_back(tx_list_x_node2[ite].score);
				// vector avg
			}
			float res = 1 - (summ / joint_node_set.size())**0.5;
			return res;
		}
	}
}

flaot TVM_for_node(int node, int omiga, vector<TX> TX_list)
{
	if(node==omiga)
		return 1;
	else
	{
		float res;
		vector<TX> tx_list_of_node = Extract_tx_for_node(node, TX_list);
		vector<int> list_who_eval_node = Extract_s_for_node(node, TX_list);	
		map<int, float> sim_val;
		for(int ite=0; ite<list_who_eval_node.size(); ite++)
		{	
			int n = list_who_eval_node[ite];
			sim_val[n] = Similarity_of_two_nodes(n, omiga, TX_list);
		}
		vector<float> TVM_sim;
		vector<float< TVM_node;
		for(int ite=0; ite<tx_list_of_node.size(); ite++)
		{	
			TX tx = tx_list_of_node[ite];
			TVM_sim.push_back(sim_val[tx.p2]);
			TVM_node.push_back(tx.score*sim_val[tx.p2]);
		}
		if(sum(TVM_sim)==0)
			res = 0;
		else:
			res = sum(TVM_node) / sum(TVM_sim);
		return res;
	}
}

float TrustValue_of_node(int node, int omiga, vector<TX> TX_list)
{
	vector<TX> tx_list_of_node = Extract_tx_for_node(node, TX_list);
	map<int, float> tvm_for_node;
	vector<int> list_who_eval_node = Extract_s_for_node(node, TX_list);
	for(int ite=0; ite<list_who_eval_node.size(); ite++)
	{
		int n = list_who_eval_node[ite];
		tvm_for_node[n] = TVM_for_node(n, omiga, TX_list);
	}
	vector<float> tmp;
	for(int ite=0; ite<tx_list_of_node.size(); ite++)
	{
		TX tx = tx_list_of_node[ite];
		tmp.push_back(tx.score*tvm_for_node[tx.p2]);
	}
	float res = sum(tmp); // sum
	return res;
}

vector<int> Ran_num(num, j)
{
	vector<int> res;
	// random n
	return res;
}


int main()
{
	clock_t start,finish;
    double totaltime;
    start=clock();





    




    finish=clock();
    totaltime=(double)(finish-start)/CLOCKS_PER_SEC;
    std::cout<<"\n run time: "<<totaltime<<"s！"<<std::endl;
	
    return 0;
}