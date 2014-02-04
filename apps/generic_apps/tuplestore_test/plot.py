#!/usr/bin/env python3

import re
import matplotlib.pyplot as plt
import csv
from matplotlib import rc
import sys
import os
import os.path
rc('font',**{'family':'serif','serif':['Palatino'], 'size': 6})
rc('text', usetex=True)

# avg over 64 measurements,
# 3000 measurements per sec.
# measurement interval is in seconds

MEASUREMENT_INTERVAL = 64.0 / 3000.0

# mA
CURRENT_FACTOR = 70.0 / 4095.0
BASELINE_ENERGY = 0.568206918430233


# Gateway hostname -> DB hostname
gateway_to_db = {
    'inode015': 'inode016',
    'inode007': 'inode010',
    'inode011': 'inode014',
    'inode009': 'inode008'
}

blacklist = [
    { 'job': '24818', 'mode': 'find', 'database': 'antelope' },
    { 'job': '24817', 'mode': 'find', 'database': 'antelope' },
    { 'job': '24814', 'mode': 'find', 'database': 'antelope' },
    { 'job': '24815', 'mode': 'find', 'database': 'antelope' },
]

# Experiment class => { 'ts': [...], 'vs': [...], 'cls': cls }
experiments = {}

def median(l):
    if len(l) == 0: return None
    #print("{} -> {}".format(l, sorted(l)[int(len(l) / 2)]))
    sl = sorted(l)
    if len(l) % 2:
        return sorted(l)[int(len(l) / 2)]
    return (sl[int(len(l) / 2)] + sl[int(len(l) / 2 - 1)]) / 2.0

def main():
    process_directories(
        sys.argv[1:],
        #lambda k: k.mode == 'find' and k.database == 'antelope'
    )
    
    fig_i_e = plt.figure()
    ax_i_e = plt.subplot(111)

    fig_i_t = plt.figure()
    ax_i_t = plt.subplot(111)

    fig_f_e = plt.figure()
    ax_f_e = plt.subplot(111)

    fig_f_t = plt.figure()
    ax_f_t = plt.subplot(111)

    #ax_i_e.set_yscale('log')

    shift_i = 1
    shift_f = 1
    l_f = 1
    l_e = 1
    for k, exp in experiments.items():
        #plot_energies([v], k.reprname() + '.pdf')
        if k.mode == 'insert':
            pos_e = [x + shift_i for x in exp.tuplecounts[:len(exp.energy)]]
            pos_t = [x + shift_i for x in exp.tuplecounts[:len(exp.time)]]

            # If for some weird reason we have more data points than
            # sent out packets, put the rest at pos 100 so we notice something
            # is up (but still can see most of the values)
            pos_e += [100] * (len(exp.energy) - len(pos_e))
            pos_t += [100] * (len(exp.time) - len(pos_t))

            pos_e, es = cleanse(pos_e, exp.energy)
            pos_t, ts = cleanse(pos_t, exp.time)

            print(k.mode, k.database, [len(x) for x in es])

            for e in es:
                print(e)
            ax_i_e.boxplot(exp.energy, positions=pos_e)
            ax_i_e.plot(pos_e, [median(x) for x in es], '^-', label=k.database)

            ax_i_t.boxplot(exp.time, positions=pos_t)
            ax_i_t.plot(pos_t, [median(x) for x in es], label=k.database)
            shift_i -= 1

        elif k.mode == 'find':
            #if k.database == 'antelope': continue

            pos_e = [x + shift_f for x in exp.tuplecounts[:len(exp.energy)]]
            pos_t = [x + shift_f for x in exp.tuplecounts[:len(exp.time)]]

            # If for some weird reason we have more data points than
            # sent out packets, put the rest at pos 100 so we notice something
            # is up (but still can see most of the values)
            pos_e += [100] * (len(exp.energy) - len(pos_e))
            pos_t += [100] * (len(exp.time) - len(pos_t))

            pos_e, es = cleanse(pos_e, exp.energy)
            pos_t, ts = cleanse(pos_t, exp.time)

            print(k.mode, k.database, [len(x) for x in es])
            print(pos_e, [median(x) for x in es])

            ax_f_e.boxplot(es, positions=pos_e)
            ax_f_e.plot(pos_e, [median(x) for x in es], label=k.database)

            ax_f_t.boxplot(ts, positions=pos_t)
            ax_f_t.plot(pos_t, [median(x) for x in ts], label=k.database)

            shift_f -= 1

    ax_i_e.legend()
    ax_i_t.legend()
    ax_f_e.legend()
    ax_f_t.legend()

    fig_i_e.savefig('pdf_out/energies_insert.pdf')
    fig_i_t.savefig('pdf_out/times_insert.pdf')
    fig_f_e.savefig('pdf_out/energies_find.pdf')
    fig_f_t.savefig('pdf_out/times_find.pdf')
            

def cleanse(l1, l2):
    """
    Given two lists l1 and l2 return two sublists l1' and l2' such that
    whenever an element at position i in either l1 or l2 is None or the empty
    list, data from that
    position at l1 or l2 will not be present in the returned lists.

    Note that specifically considers only None and [], not other values that
    evaluate to false like 0, False or the empty string.

    As a side effect it ensures the returned lists are cut to the same length

    >>> cleanse([1, 2, None, 4, [], 6], [None, 200, 300, 400, 500, 600, 70, 88])
    ([2, 4, 6], [200, 400, 600])

    >>> cleanse([1, [None, None, 2, None, 3]], [[None, 100], [200, 300]])
    ([1, [2, 3]], [[100], [200, 300]])

    """

    r1 = []
    r2 = []
    for x, y in zip(l1, l2):
        if isinstance(x, list):
            x = list(filter(lambda x: x is not None and x != [], x))
        if isinstance(y, list):
            y = list(filter(lambda z: z is not None and z != [], y))

        if x is not None and x != [] and y is not None and y != []:
            r1.append(x)
            r2.append(y)
    return r1, r2

def plot_energies(experiments, fname='energies.pdf'):
    fig = plt.figure()
    ax = plt.subplot(111)
    #ax.set_xticklabels([str(x) for x in experiments[0].tuplecounts])
    for exp in experiments:
        if exp.energy:
            #print(len(exp.energy), len(exp.tuplecounts))
            ax.boxplot(exp.energy, positions=exp.tuplecounts[:len(exp.energy)])
    print("writing {}.".format(fname))
    fig.savefig(fname)

def plot_times(experiments, fname='times.pdf'):
    fig = plt.figure()
    ax = plt.subplot(111)
    #ax.set_xticklabels([str(x) for x in experiments[0].tuplecounts])
    for exp in experiments:
        #print(exp.energy)
        if exp.energy:
            ax.boxplot(exp.energy, positions=exp.tuplecounts)
    print("writing {}.".format(fname))
    fig.savefig(fname)

class ExperimentClass:
    def __init__(self, d):
        self.__dict__.update(d)

    def __eq__(self, other):
        return (
            self.dataset == other.dataset and
            self.mode == other.mode and
            self.debug == other.debug and
            self.database == other.database
        )

    def reprname(self):
        return self.database + '_' + self.dataset.replace('.rdf', '') + '_' + self.mode + (' DBG' if
self.debug else '')

    def __hash__(self):
        return hash(self.dataset) ^ hash(self.mode) ^ hash(self.debug) ^ hash(self.database)

class Experiment:
    def __init__(self):
        self.time = []
        self.energy = []
        self.tuplecounts = []

    def add_measurement(self, i, t, e):
        while len(self.time) < i + 1:
            self.energy.append([])
            self.time.append([])
        self.time[i].append(t)
        self.energy[i].append(e)

    def set_tuplecounts(self, tcs):
        self.tuplecounts = cum(tcs)

def add_experiment(cls):
    global experiments
    if cls not in experiments:
        experiments[cls] = Experiment()
    return experiments[cls]

def inode_to_mote_id(s): return '10' + s[5:]

def process_directories(dirs,f=lambda x: True):
    for d in dirs:
        process_directory(d, f)

def process_directory(d, f=lambda x: True):
    d = str(d).strip('/')
    print(d)
    energy = read_energy(d + '.csv')
    for gw, db in gateway_to_db.items():
        fn_gwinfo = d + '/' + gw + '/' + gw + '.vars'
        #fn_dbout = d + '/' + db + '/output.txt'
        fn_gwout = d + '/' + gw + '/output.txt'
        if not os.path.exists(fn_gwinfo):
            print("{} not found, ignoring that area.".format(fn_gwinfo))
            continue

        print(" processing {}, {}".format(gw, db))

        v = read_vars(fn_gwinfo)
        cls = ExperimentClass(v)
        cls.job = d
        print("  {}: {} {}".format(cls.database, cls.dataset, cls.mode))

        # Is this experiment blacklisted?
        blacklisted = False
        for b in blacklist:
            for k, x in b.items():
                if getattr(cls, k) != x:
                    # does not match this blacklist entry
                    break
            else:
                blacklisted = True
                break

        if blacklisted:
            print("  (blacklisted)")
            continue

        if not f(cls):
            print("  (ignored by filter)")
            continue

        exp = add_experiment(cls)
        tc = parse_tuple_counts(open(fn_gwout, 'r', encoding='latin1'), d)
        if not tc:
            print("  no tuplecounts found in {}, using default".format(fn_gwout))
            tc = [7, 6, 8, 11, 11, 10, 11, 9]
        exp.set_tuplecounts(tc)
        #print(energy[inode_to_mote_id(db)])
        mid = inode_to_mote_id(db)
        if mid not in energy:
            print("  no energy values for {}. got values for {}".format(mid, str(energy.keys())))
            continue

        print("energy", type(energy))
        print("v", type(v))
        runs_t, runs_e = process_energy(energy[mid], v['mode'], lbl=v['mode'] + '_' + v['database']
+ '_' + db)

        runs_count = 0
        for ts, es in zip(runs_t, runs_e):
            runs_count += 1
            for i, (t, e) in enumerate(zip(ts, es)):
                if t != 0 or e != 0:
                    exp.add_measurement(i, t, e)
        print("  processed {} experiment runs.".format(runs_count))

def read_vars(fn):
    r = {}
    for line in open(fn, 'r'):
        k, v = line.split('=')
        k = k.strip()
        v = v.strip()
        k = k.lower()
        if v.startswith('"'):
            r[k] = v[1:-1]
        else:
            r[k] = int(v)
    if 'database' not in r:
        r['database'] = 'db'

    return r

def read_energy(fn):
    r = {}

    #reader = csv.DictReader(f, delimiter=';', quotechar='"')
    reader = csv.DictReader(open(fn, 'r'), delimiter='\t', quotechar='"')
    #t = {}
    for row in reader:
        mote_id = row['motelabMoteID']
        if mote_id not in r: r[mote_id] = {'ts':[], 'vs':[]}

        t = r[mote_id]['ts'][-1] + MEASUREMENT_INTERVAL if len(r[mote_id]['ts']) else 0
        r[mote_id]['ts'].append(t)
        v = float(row['avg']) * CURRENT_FACTOR
        r[mote_id]['vs'].append(v)

    return r

def parse_tuple_counts(f, expid):
    r = []
    for line in f:
        m = re.search(r'sent ([0-9]+) tuples chk', line)
        if m is not None:
            if int(expid) >= 24638:
                # starting from exp 24638 tuple counts are reported
                # incrementally
                r.append(int(m.groups()[0]) - (sum(r) if len(r) else 0))
            else:
                r.append(int(m.groups()[0]))
    return r

#def find_tuple_spikes(ts, vs):
def process_energy(d, mode, lbl=''):
    ts = d['ts']
    vs = d['vs']
    fig_energy(ts, vs, lbl)

    # setup as follows:
    #
    # (1) first, start in high-load mode ( >= HIGH ),
    # (2) then, itll drop down to idle ( <= IDLE )
    # (3) then itll go above MEASUREMENT
    # (4) then fall below IDLE
    # then back to 1

    HIGH = 7.0
    RADIO = 1.5
    MEASUREMENT = 1.5

    class State:
        def __init__(self, s): self.s = s
        def __str__(self): return self.s

    idle_high = State('high')

    idle_before = State('idle before')
    insert = State('insert')
    idle_between = State('idle between')
    find = State('find')
    idle_after = State('idle_after')

    state = idle_high

    # unit: mA * s = mC
    #energy_sums_insert = []
    #energy_sums_find = []
    #time_sums_insert = []
    #time_sums_find = []
    sums_e = []
    sums_t = []
    runs_e = []
    runs_t = []

    thigh = 0
    t0 = 0
    tprev = 0
    esum = 0
    t = 0

    #esums_i = []
    #esums_f = []
    #tsums_i = []
    #tsums_f = []

    baseline_estimate = 0
    baseline_estimate_n = 0

    def change_state(s):
        nonlocal state
        nonlocal thigh
        if s is not state:
            print("{}: {} -> {}".format(t, state, s))
            if s is idle_high:
                thigh = t
            state = s

    for t, v in zip(ts, vs):
        if state is idle_high:
            if v < RADIO:
                if t - thigh > 500:
                    print("  reboot t={} thigh={}".format(t, thigh))
                    # reboot
                    runs_e.append(sums_e)
                    runs_t.append(sums_t)
                    sums_e = []
                    sums_t = []
                    
                    #esums_i.append(energy_sums_insert)
                    #esums_f.append(energy_sums_find)
                    #tsums_i.append(time_sums_insert)
                    #tsums_f.append(time_sums_find)
                    #energy_sums_insert = []
                    #energy_sums_find = []
                    #time_sums_insert = []
                    #time_sums_find = []
                else:
                    print("  t-tigh={} t={}".format(t-thigh, t))
                change_state(idle_before)

        elif state is idle_before:
            if v > MEASUREMENT:
                change_state(insert)
                if mode == 'insert':
                    t0 = t
                    esum = (t - tprev) * (v - BASELINE_ENERGY)
                    #tprev = t
            else:
                baseline_estimate *= baseline_estimate_n / (baseline_estimate_n + 1.0)
                baseline_estimate_n += 1.0
                baseline_estimate += v / baseline_estimate_n 

        elif state is insert:
            if v < MEASUREMENT:
                change_state(idle_between)
                if mode == 'insert':
                    esum += (t - tprev) * (v - BASELINE_ENERGY)
                    sums_t.append(t - t0)
                    sums_e.append(esum)
            else:
                #print(v, t)
                #assert v < HIGH
                if v >= HIGH:
                    print("  insert abort high t={}".format(t))
                    sums_e.append(None)
                    sums_t.append(None)
                    change_state(idle_high)
                    #thigh = t
                elif mode == 'insert':
                    esum += (t - tprev) * (v - BASELINE_ENERGY)
                    #tprev = t

        elif state is idle_between:
            if v > HIGH:
                #print("  find measurement skipped high at {}".format(t))
                if mode == 'find':
                    sums_e.append(None)
                    sums_t.append(None)
                change_state(idle_high)
                #thigh = t
            elif v > MEASUREMENT:
                change_state(find)
                if mode == 'find':
                    t0 = t
                    #tprev = t
                    #esum = 0
                    esum = (t - tprev) * (v - BASELINE_ENERGY)
            else:
                baseline_estimate *= baseline_estimate_n / (baseline_estimate_n + 1.0)
                baseline_estimate_n += 1.0
                baseline_estimate += v / baseline_estimate_n 

        elif state is find:
            if v < MEASUREMENT:
                #print("find measurement at {} e {}".format(t, esum))
                change_state(idle_after)
                if mode == 'find':
                    esum += (t - tprev) * (v - BASELINE_ENERGY)
                    sums_t.append(t - t0)
                    sums_e.append(esum)
            elif v > HIGH:
                #print("  find measurement aborted high at {} t0={} esum={}".format(t, t0, esum - BASELINE_ENERGY))
                # what we thought was a find measurement was actually a
                # rising edge for the high idle state,
                # seems there was no (measurable) find process, record a
                # 0-measurement
                change_state(idle_high)
                #thigh = t
                if mode == 'find':
                    sums_t.append(None)
                    sums_e.append(None)
            else:
                if mode == 'find':
                    esum += (t - tprev) * (v - BASELINE_ENERGY)
                    #tprev = t

        elif state is idle_after:
            if v > HIGH:
                change_state(idle_high)
                #thigh = t
            else:
                baseline_estimate *= baseline_estimate_n / (baseline_estimate_n + 1.0)
                baseline_estimate_n += 1.0
                baseline_estimate += v / baseline_estimate_n 

        tprev = t


    print("  baseline estimate: {}".format(baseline_estimate))

    #state = "H"
    #oldstate = "H"
    #for t, v in zip(ts, vs):
        #if state == "H" and v < IDLE:
            #state = "I0"
        #elif state == "I1" and v > HIGH:
            #state = "H"
        #elif state == "I0" and v > MEASUREMENT:
            #state = "M"
            #t0 = t
            #tprev = t
            #esum = 0
        #elif state == "M" and v < IDLE:
            #state = "I1"
            #time_sums.append(t - t0)
            #energy_sums.append(esum)
        #elif state == "M":
            ##assert v < HIGH
            #esum += (t - tprev) * v
            #tprev = t
    
        #if state != oldstate:
            #print("{}: {} ({} -> {})".format(t, v, oldstate, state))
            #oldstate = state

    #esums_i.append(energy_sums_insert)
    #esums_f.append(energy_sums_find)
    #tsums_i.append(time_sums_insert)
    #tsums_f.append(time_sums_find)
    runs_t.append(sums_t)
    runs_e.append(sums_e)
    return (runs_t, runs_e)

def frange(a, b, step):
    return [x * step for x in range(int(a / step), int(b / step))]


def fig_energy(ts, vs, n):
    fig = plt.figure()
    ax = plt.subplot(111)
    
    #ax.set_xticks(range(250, 311, 2))
    #ax.set_yticks(frange(0, 3, 0.2))

    ax.set_xlim((000, 800))
    #ax.set_ylim((0, 3))
    ax.grid()

    ax.plot(ts, vs)
    fig.savefig('energy_{}.pdf'.format(n))

def cum(l):
    s = 0
    r = []
    for v in l:
        s += v
        r.append(s)
    return r

def plot_experiment(n, ax, **kwargs):
    global fig
    ts, vs = parse_energy(open('{}.csv'.format(n), 'r'))
    fig_energy(ts, vs, n)
    #tc = parse_tuple_counts(open('{}/inode001/output.txt'.format(n), 'r', encoding='latin1'), int(n))
    d = find_tuple_spikes(ts, vs)
    
    tc = [7, 6, 8, 11, 11, 10, 11, 9]
    #ax = plt.subplot(111)
    for e in range(len(d['e_insert'])):
        es = d['e_insert'][e]
        fs = d['e_find'][e]
        fs = d['e_find'][e]

        print(d['e_insert'][e])
        print(d['t_insert'][e])

        #ax.plot(cum(tc[:len(es)]), ([x/y for x, y in zip(es, tc)]), 'o-', **kwargs) 
        ax.plot(cum(tc[:len(fs)]), ([x/y for x, y in zip(fs, tc)]), 'x-', **kwargs) 
        ax.plot(cum(tc[:len(fs)]), ([x/y for x, y in zip(d['t_find'][e], tc)]), 'x-', **kwargs) 
            

    ## incontextsensing
    ##tc = [7, 6, 8, 11, 11, 10, 11, 9]

    ##ax = plt.subplot(111)
    #print("XXX", energy_sums)
    #print("YYY", tc)
    #ax.plot(cum(tc[:len(energy_sums)]), ([x/y for x, y in zip(energy_sums, tc)]), 'o-',
            #**kwargs) 
    #ax.plot(cum(tc[:len(time_sums)]), ([x/y for x, y in zip(time_sums, tc)]), 'x-',
            #**kwargs)
    #ax.legend()

#ts, vs = parse_energy(open('tuplestore_24526_1242.csv', 'r'))
#ts, vs = parse_energy(open('tuplestore_24528_1242.csv', 'r'))
#fig_energy(ts, vs)
#print()

#fig = plt.figure()
#ax = plt.subplot(111)

#plot_experiment(24539, ax, label='antelope 39')

## unbaltreedict, list_dynamic container
##plot_experiment(24529, ax, label='ts')

## ts staticdict(100, 15), staticvector(76)
##plot_experiment(24578, ax, label='ts')

## antelope with more realistic config?
##plot_experiment(24579, ax, label='antelope2')

## ts staticdict(100, 15), staticvector(76), strncmp_etc_builtin
##plot_experiment(24580, ax, label='ts100')

## ts staticdict(200, 15), staticvector(76), strncmp_etc_builtin
##plot_experiment(24582, ax, label='ts200a')
## ts staticdict(200, 15), staticvector(76), strncmp_etc_builtin
##plot_experiment(24583, ax, label='ts200b')

# ts insert incontextsensing staticdict(200, 15), staticvector(76), strncmp_etc_builtin
#plot_experiment(24638, ax, label='ts200')
# ts insert incontextsensing staticdict(100, 15), staticvector(76), strncmp_etc_builtin
#plot_experiment(24639, ax, label='ts100')

## ts insert incontextsensing staticdict-le(100, 15), staticvector(76), strncmp_etc_builtin
#plot_experiment(24643, ax, label='ts100le 43')
#plot_experiment(24644, ax, label='ts100le 44')
#plot_experiment(24645, ax, label='ts100le 45')
##plot_experiment(24646, ax, label='ts100le 46')
#plot_experiment(24648, ax, label='ts100le 48')
#plot_experiment(24649, ax, label='ts100le 49')

#plot_experiment(24641, ax, label='antelope 41')

#plot_experiment(24650, ax, label='antelope 50')
#plot_experiment(24651, ax, label='antelope 51')
#plot_experiment(24652, ax, label='antelope 52')

#plot_experiment(24778, ax, label='antelope')

#fig.savefig('energy_inserts.pdf')

if __name__ == '__main__':
    main()


