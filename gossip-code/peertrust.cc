#include <iostream>
#include <utility>
#include <map>
#include <vector>
#include <random>
#include <time>

using namespace std;

const int NUM=10;
const int PRICE_BOUND = 20;


// map<pair<int, int>, int> tx_num_gen(int max_n)
// {
// 	srand(Time(NULL));
// 	map<pair<int, int>, int> map_tx_num;
// 	for(int i=0; i<NUM; i++)
// 	{
// 		for(int j=i+1; j<NUM; j++)
// 		{
// 			int x = rand() % max_n;
// 			float xx = x / max_n;
// 			pair<int, int> tmp;
// 			tmp.first = i;
// 			tmp.second = j;
// 			tx_num_gen[tmp] = xx;
// 		}
// 	}

// 	return map_tx_num;
// }

// map<pair<int, int>, vector<pair<float, float> > > tx_gen(map<pair<int, int>, int> tx_num)
// {
// 	srand(Time(NULL));
// 	map<pair<int, int>, pair<float, float> > map_tx;
// 	for(int i=0; i<NUM; i++)
// 	{
// 		for(int j=i+1; j<NUM; j++)
// 		{
// 			pair<int, int> tmp;
// 			tmp.first = i;
// 			tmp.second = j;
// 			vector<pair<float, float> > vec_tx;
// 			for(int k=0; k<tx_num[tmp])
// 			{
// 				int x = rand() % 100;
// 				int y = rand() % 100;
// 				float xx = x / 100;
// 				float yy = y / 100;
// 				pair<float, float> q;
// 				q.first = xx;
// 				q.second = yy;
// 				vec_tx.push_back(q);
// 			}
// 			map_tx[tmp] = vec_tx;
// 		}
// 	}
// 	return vec_tx;
// }

class TX
{
public:
	int id;
	int peer1;
	int peer2;
	float s_1_2;
	float s_2_1;
	float price;


}

int main()
{

	return 0;
}