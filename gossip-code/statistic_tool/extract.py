import re
import matplotlib.pyplot as plt

len1 = 80
len2 = 81

f = open("node_1_receiving_time_log.txt")
sentence_lisphase1_len = f.readlines()
sentence_lisphase2_len = sentence_lisphase1_len[1:-1:2]
sentence_list3 = sentence_lisphase1_len[::2]


pattern2 = re.compile(r"epoch (\d+(\.\d+)?): block propagation time: (\d+(\.\d+)?), voting time: (\d+(\.\d+)?)")
pattern3 = re.compile(r"epoch (\d+(\.\d+)?): starts at (\d+(\.\d+)?)s")
x = [i for i in range(len1)]
phase1_len = [0 for i in range(len1)]
phase2_len = [0 for i in range(len1)]
for k in range(len1):
    res = pattern2.search(sentence_lisphase2_len[k])
    if res:
        phase1_len[k] = float(res.groups()[2])
        phase2_len[k] = float(res.groups()[4])
time_start = []
for k in range(len2):
    res = pattern3.search(sentence_list3[k])
    time_start.append(float(res.groups()[2]))


phase_len = [(phase1_len[k] + phase2_len[k]) for k in range(len1)]
tps = [4000/x for x in phase_len]

g = open("datarate.txt")
tmp1 = g.readlines()
pattern4 = re.compile(r'(\d+(\.\d+)?)\+(\d+(\.\d+)?)')
tmp2 = []
for k in range(len(tmp1)):
    res = pattern4.search(tmp1[k])
    tmp2.append(float(res.groups()[2]))
bd = tmp2[:30]
time_bd_vary = [(i*60) for i in range(30)]

res_max = max(bd)
bd = [(x/res_max*300) for x in bd]
plt.plot(time_bd_vary, bd[:30], 'r')







plt.plot(time_start[0:60], tps[0:60], 'b')
plt.show()
