import numpy as np
import matplotlib.pyplot as plt
from math import sqrt

#needs to run the bench.sh script in the benchmarks branch 

reruns=10
t=0     # threads
sw=1    # serial work
K=2     # parareal iterations
vErr=3  # vanilla error
pErr=4  # pthread error
oErr=5  # omp error
vT=6    # vanilla time
vS=7    # vanilla speedup
vE=8    # vanilla efficiency
pT=9    # pthread time
pS=10   # pthread speedup
pE=11   # pthread efficiency
oT=12   # omp time
oS=13   # omp speedup
oE=14   # omp efficiency

def find(ind, best):
    wt = np.where(best[:, t] == ind[t]) 
    ws = np.where(best[:, sw] == ind[sw])
    wK = np.where(best[:, K] == ind[K])
    w = np.intersect1d(np.intersect1d(wt, ws), wK)
    return w

def speedup_v(t, sw, K):
    costf = t*sw/4096   # coarse_cost / fine_cost, with parallel steps fixed to 1024
    return 1/((1+K)*costf + K/t)

def speedup_pl(t, sw, K):
    costf = t*sw/4096
    return 1/((1+K/t)*costf + K/t)

# forgot squareroots of error before benchmark, so thats corrected now.
def pretty(row):
    sv = speedup_v(row[t], row[sw], row[K])
    ev = sv/row[t]
    sp = speedup_pl(row[t], row[sw], row[K])
    ep = sp/row[t]
    print(f"t:{int(row[t]):d} sw:{int(row[sw]):d} K:{int(row[K]):d} vErr:{sqrt(row[vErr]):.2e} pErr:{sqrt(row[pErr]):.2e} oErr:{sqrt(row[oErr]):.2e}")
    print(f"    vT:{row[vT]:.2f} vS:{row[vS]:.2f}({sv:.2f}) vE:{row[vE]:.2f}({ev:.2f})")
    print(f"    pT:{row[pT]:.2f} pS:{row[pS]:.2f}({sp:.2f}) pE:{row[pE]:.2f}({ep:.2f})")
    print(f"    oT:{row[oT]:.2f} oS:{row[oS]:.2f}({sp:.2f}) oE:{row[oE]:.2f}({ep:.2f})\n")

data = np.loadtxt('bench.out', delimiter=",")
best = np.zeros((int(len(data)/reruns), len(data[0])))

print(len(data))
for i in range(len(best)):
    best[i, t:K+1] = data[i*reruns, t:K+1]
    bv = data[i*reruns:(i+1)*reruns, vS].argmax() + i*reruns
    best[i, (vErr, vT, vS, vE)] = data[bv, (vErr, vT, vS, vE)];
    bp = data[i*reruns:(i+1)*reruns, pS].argmax() + i*reruns
    best[i, (pErr, pT, pS, pE)] = data[bp, (pErr, pT, pS, pE)];
    bo = data[i*reruns:(i+1)*reruns, oS].argmax() + i*reruns
    best[i, (oErr, oT, oS, oE)] = data[bo, (oErr, oT, oS, oE)];

np.savetxt('best.out', best, delimiter=",", fmt='% .2e')

Y = [0, 0, 0, 0, 0]
Ysv = [0, 0, 0, 0, 0]
Ysp = [0, 0, 0, 0, 0]
tX = [1, 2, 4, 8, 16]
swX = [32, 16, 8, 4, 2]


pretty(best[find([ 1, 32, 1], best)][0])
pretty(best[find([ 2, 16, 1], best)][0])
pretty(best[find([ 4,  8, 1], best)][0])
pretty(best[find([ 8,  4, 1], best)][0])
pretty(best[find([16,  2, 1], best)][0])

labels = [None]*15
labels[vE] = 'vanilla'
labels[pE] = 'pthread'
labels[oE] = 'omp'

lines = [None]*15;

fig, axs = plt.subplots(2, 2)
for k in range(1, 5):
    k_ind = ((k-1)//2, (k-1) % 2)
    print(k_ind)
    for impl in [vE, pE, oE]:
        for i in range(5):
            Y[i] = best[find([ tX[i], swX[i], k], best)][0][impl]
        lines[impl], = axs[k_ind].plot(tX, Y, marker="x")#, label=labels[impl])# linestyle="None")
        for i in range(5):
            Ysv[i] = speedup_v(tX[i], swX[i], k)/tX[i]
            Ysp[i] = speedup_pl(tX[i], swX[i], k)/tX[i]
        lines[0], = axs[k_ind].plot(tX, Ysv, linestyle = 'dashed')#, label = 'max speedup v')
        lines[1], = axs[k_ind].plot(tX, Ysp, linestyle = 'dashed')#, label = )
    axs[k_ind].set_xlabel('num threads', fontsize=15)
    axs[k_ind].set_ylabel('parallel efficieny', fontsize=15)
    axs[k_ind].set_title(f'K={k}', fontsize=15)

for i in [vE, pE, oE]:
    lines[i].set_label(labels[i])
lines[0].set_label('max E_v')
lines[1].set_label('max E_pl')

#axs[k_ind].legend([labels[vE], labels[pE], labels[oE], 'max speedup v', 'max speedup pl'])
plt.legend(bbox_to_anchor=(1.0,1.0), loc="center left", fontsize=13)
#plt.xlabel('num threads', fontsize=15)
#plt.ylabel('parallel efficieny', fontsize=15)
fig.suptitle("Efficiency Plot for constant Work", fontsize=16)
plt.show()

# only keep best run
