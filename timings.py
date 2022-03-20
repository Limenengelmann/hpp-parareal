import matplotlib.pyplot as plt
import sys
import re
import numpy as np

if len(sys.argv) < 2:
    print("Usage: " + sys.argv[0] + " timings.data")
    sys.exit()

fname = sys.argv[1];
fnameomp = fname + ".omp";
regex = "timings.t(\d+).sw(\d+).K(\d+).data.(\w+)"
match = re.search(regex, fname);
num_threads = int(match.group(1))
sw = int(match.group(2))
piters = int(match.group(3))
ptype  = match.group(4)

print("script             : ", sys.argv[0])
print("Plotting file      : ", fname)
print("piters             : ", piters)
print("sw                 : ", sw)
print("num_threads        : ", num_threads)
print("ptype              : ", ptype)

print("number of arguments: ", len(sys.argv))

# load data
data = np.loadtxt(fname)

colors = ['red', 'green', 'blue']
labels = [' init', ' indep. part', ' dep. part']
lines = [0,0,0];


fig, ax = plt.subplots()

for i in range(0, len(data), 2):
    l = np.ubyte(data[i, 2])
    # -data[0,0] to start time from 0
    lines[l], = ax.plot(data[i:i+2,0]-data[0,0], data[i:i+2, 1], color=colors[l], linewidth=20, solid_capstyle='butt')
    if l==2:    # new iteration starts
        ax.plot(data[i+1, 0]-data[0,0], data[i+1,1], marker="|", color='black', ms=40, solid_capstyle='butt')

for i in range(0, 3):
    lines[i].set_label(labels[i])

leg = plt.legend(bbox_to_anchor=(0.85,0.25), loc="center left", fontsize=15)
for l in leg.get_lines():
    l.set_linewidth(4.0)

plt.xlabel('time in s', fontsize=15)
plt.ylabel('thread id', fontsize=15)
plt.yticks(np.arange(0, max(data[:, 1])+1, 1.0))
plt.title(f"{ptype}: K{piters}, sw{sw}", fontsize=15)

plt.savefig(f"work.{ptype}.K{piters}.sw{sw}.png", bbox_inches="tight")
plt.show()
