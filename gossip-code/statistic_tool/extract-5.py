import re
import numpy as np
import math


def split_according_to_space(list1):
    i = 1
    k = 0
    res = {}
    tmp_sublist = []
    while(k<len(list1)):
        if(len(list1[k])>1):
            tmp_sublist.append(list1[k])
        else:
            res[i] = tmp_sublist
            tmp_sublist = []
            i += 1
        k += 1
    return res

epoch_timemap = {}
# key: epoch; value: node_node_transtime map
node_recvtime = {}
# key: node; value: receiving time
epoch_node_recvtime = {}
# key: epoch; value: map {node: recvtime}
node_node_recvtime = {}
# key: (transmitter, receiver); value: absolute receiving time
node_node_transtime = {}
# key: (transmitter, receiver); value: transmission time between them

N = 52
for t in range(1, 21):
    node_recvtime = {}
    node_node_transtime = {}
    node_node_recvtime = {}
    for i in range(N):
        str1 = "statistics_data/log_cmjj_5_23_20_03/node_"+str(i)+"_receiving_time_log.txt"
        infile1 = open(str1)
        list1 = infile1.readlines()
        res1 = split_according_to_space(list1)

        pattern1 = re.compile(r'(\d+) (\d+(\.\d+)?)')

        sublist1 = []
        for x in res1[t][1:]:
            res2 = pattern1.search(x)
            res3 = res2.groups()[0:2]
            node_node_recvtime[(int(res3[0]), i)] = float(res3[1])
            sublist1.append(float(res3[1]))
        if(i!=1):
            node_recvtime[i] = min(sublist1)
        else:
            node_recvtime[i] = 0
    for x in node_node_recvtime.keys():
        if(node_recvtime[x[0]]>0 or x[0]==1):
            node_node_transtime[x] = node_node_recvtime[x] - node_recvtime[x[0]]
            # if(node_node_transtime[x] < 0):
            #     print(node_node_recvtime[x], node_recvtime[x[0]])
            #     print("*****", node_node_transtime[x])
        else:
            node_node_transtime[x] = "****"

    epoch_node_recvtime[t] = node_recvtime
    epoch_timemap[t] = node_node_transtime

# for x in epoch_timemap.keys():
#     print(x)
#     dict1 = epoch_timemap[x]
#     for y in dict1.keys():
#         print(y, dict1[y])
#     print("\n\n")


key_set = set()
for i in range(1,21):
    key_set = key_set.union(set(epoch_timemap[i].keys()))

key_list = {}
for x in key_set:
    list2 = []
    list3 = []
    for i in range(1,21):
        if x in epoch_timemap[i].keys():
            y = epoch_timemap[i][x]
            list2.append(y)
            list3.append(math.ceil(y))
        else:
            list3.append("*")
    key_list[x] = np.var(list2)
    print(("%10s %6d %30s") % (x, key_list[x], list3))

print(len(key_set))
