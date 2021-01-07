#!/usr/bin/python

import sys
import getopt
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.lines as mlines

#===========================================
def autoLabel (rects, ax):
#===========================================
    """Attach a text label above each bar in *rects*, displaying its height."""
    for rect in rects:
        height = rect.get_height()
        ax.annotate('{}'.format(height),
                    xy=(rect.get_x() + rect.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom')

#===========================================
def helpInfo():
#===========================================
    print ('Usage: [-h --help] [-t --type <loss/tx/mix/channelloss/retx>] [-j --json <json file name>]')
    print ('Options:')
    print ('\t-t --type\ttype of measurement to be plotted\tDefault: loss')
    print ('\t-j --json\tjson file name')
    print ('\t-h --help\tthis help documentation')

#===========================================
def plotRetransmissionStat (data, data_relay, fig, ax, measure_date, average_hop_one_channel_loss_estimation, average_hop_two_channel_loss_estimation):
#===========================================
    hop_one_retx_stat = {}
    hop_two_retx_stat = {}
    for index in range (len (data)):
        if data[index]['type'] == 'no_coding':
            if hop_one_retx_stat.has_key (data[index]['tx_num']) == True:
                hop_one_retx_stat[data[index]['tx_num']] = hop_one_retx_stat[data[index]['tx_num']] + 1
            else:
                hop_one_retx_stat[data[index]['tx_num']] = 1
            if hop_two_retx_stat.has_key (data_relay[index]['fwd_num']) == True:
                hop_two_retx_stat[data_relay[index]['fwd_num']] = hop_two_retx_stat[data_relay[index]['fwd_num']] + 1
            else:
                hop_two_retx_stat[data_relay[index]['fwd_num']] = 1

    sorted (hop_one_retx_stat.keys())
    total_num = sum (hop_one_retx_stat.values())
    for index in range (len (hop_one_retx_stat)):
        hop_one_retx_stat[hop_one_retx_stat.keys()[index]] = float (hop_one_retx_stat[hop_one_retx_stat.keys()[index]]) / total_num
    sorted (hop_two_retx_stat.keys())
    total_num = sum (hop_two_retx_stat.values())
    for index in range (len (hop_two_retx_stat)):
        hop_two_retx_stat[hop_two_retx_stat.keys()[index]] = float (hop_two_retx_stat[hop_two_retx_stat.keys()[index]]) / total_num

    x = hop_one_retx_stat.keys()  # the label locations
    ax.bar (x, hop_one_retx_stat.values())
    ax.set_xticks (hop_one_retx_stat.keys())
    ax.set_title ('hop one (channel loss ~%d%%)' % getAverageChannelLoss (average_hop_one_channel_loss_estimation), fontsize=20)
    ax.set_xlabel ('number of transmission', fontsize=20)
    ax.set_ylabel ('percentage', fontsize=20)
    #ax.set_ylim([0,1])
    fig.savefig (fname='two_hop_%s_hop_one_retx_stat.pdf' % measure_date, bbox_inches='tight')

    x = hop_two_retx_stat.keys()  # the label locations
    ax.bar (x, hop_two_retx_stat.values())
    ax.set_xticks (hop_two_retx_stat.keys())
    ax.set_title ('hop two (channel loss ~%d%%)' % getAverageChannelLoss (average_hop_two_channel_loss_estimation), fontsize=20)
    ax.set_xlabel ('number of transmission', fontsize=20)
    ax.set_ylabel ('percentage', fontsize=20)
    #ax.set_ylim([0,1])
    fig.savefig (fname='two_hop_%s_hop_two_retx_stat.pdf' % measure_date, bbox_inches='tight')


#===========================================
def getDate (json_file):
#===========================================
    if json_file[len (json_file) - 7] == '_':
        return json_file[len (json_file) - 11 : len (json_file) - 7]
    else:
        return json_file[len (json_file) - 9 : len (json_file) - 5]

#===========================================
def getAverageChannelLoss (average_channel_loss_estimation):
#===========================================
    return int (sum (average_channel_loss_estimation) / len (average_channel_loss_estimation) / 10 * 1000)

#===========================================
def plotPercentile (fig, ax, x, y, loss_data, tx_data, extra_tx, plot_type, color):
#===========================================
    loss_lower = np.mean (loss_data) - np.std (loss_data, ddof=1) * 2.0211 / np.sqrt (len (loss_data))
    loss_upper = np.mean (loss_data) + np.std (loss_data, ddof=1) * 2.0211 / np.sqrt (len (loss_data))

    if plot_type == 'mix':
        tx_lower = np.mean (tx_data) - np.std (tx_data, ddof=1) * 2.0211 / np.sqrt (len (tx_data))
        tx_upper = np.mean (tx_data) + np.std (tx_data, ddof=1) * 2.0211 / np.sqrt (len (tx_data))
    elif plot_type == 'final':
        energy_stat = []
        for i in range (len (tx_data)):
            energy_stat.append ((tx_data[i] + extra_tx) * 4)
        tx_lower = np.mean (energy_stat) - np.std (energy_stat, ddof=1) * 2.0211 / np.sqrt (len (energy_stat))

        tx_upper = np.mean (energy_stat) + np.std (energy_stat, ddof=1) * 2.0211 / np.sqrt (len (energy_stat))
    else:
        print 'wrong type!'
        return
    l1 = mlines.Line2D([loss_lower,loss_upper], [y,y], color=color, zorder=0, alpha=0.5, marker='|', markersize=10)
    ax.add_line(l1)
    l2 = mlines.Line2D([x,x], [tx_lower,tx_upper], color=color, zorder=0, alpha=0.5, marker='_', markersize=10)
    ax.add_line(l2)

#===========================================
def main():
#===========================================
    opts, argv = getopt.getopt (sys.argv[1:],'-h-t:-j:-l:',['help', 'type', 'json', 'link'])
    if len (opts) == 0:
        helpInfo()
        exit()

    plot_loss = True
    plot_tx = False
    plot_mix = False
    plot_channel_loss = False
    plot_retx_stat = False
    plot_hop_one = True
    plot_hop_two = False
    plot_total = False
    plot_final = False
    json_file = ''
    json_file_nc_relay = ''
    json_file_no_src = ''
    json_file_no_relay = ''
    json_file_no_dst = ''

    # cmd arguments parsing
    for name, value in opts:
        if name in ('-h', '--help'):
            helpInfo()
            exit()
        elif name in ('-t', '--type'):
            if value == 'loss':
                pass
            elif value == 'tx':
                plot_loss = False
                plot_tx = True
            elif value == 'mix':
                plot_loss = False
                plot_mix = True
            elif value == 'channelloss':
                plot_loss = False
                plot_channel_loss = True
            elif value == 'retx':
                plot_loss = False
                plot_retx_stat = True
            elif value == 'final':
                plot_loss = False
                plot_final = True
            else:
                print ('wrong type!')
                helpInfo()
                exit()
        elif name in ('-j', '--json'):
            json_file = value + '.dump'
            json_file_nc_relay = value + '_nc_relay.dump'
            json_file_no_src = value + '_no_src.dump'
            json_file_no_relay = value + '_no_relay.dump'
            json_file_no_dst = value + '_no_dst.dump'
        elif name in ('-l', '--link'):
            if value == '1':
                plot_hop_one = True
                plot_hop_two = False
                plot_total = False
            elif value == '2':
                plot_hop_one = False
                plot_hop_two = True
                plot_total = False
            elif value == '3':
                plot_hop_one = False
                plot_hop_two = False
                plot_total = True
            else:
                print ('wrong link number!')
                helpInfo()
                exit()
        else:
            helpInfo()
            exit()

    # open json file
    with open (json_file, 'r') as f:
        data = json.load(f)
    with open (json_file_nc_relay, 'r') as f:
        data_nc_relay = json.load(f)
    with open (json_file_no_src, 'r') as f:
        data_no_src = json.load(f)
    with open (json_file_no_relay, 'r') as f:
        data_no_relay = json.load(f)
    with open (json_file_no_dst, 'r') as f:
        data_no_dst = json.load(f)

    measure_date = getDate (json_file)

    # OT ARQ
    no_coding_loss_rate = []
    no_coding_hop_one_tx_num = []
    no_coding_hop_two_tx_num = []
    no_coding_total_tx_num = []

    # NC
    nc_0_loss_rate = []
    nc_0_tx_num = []
    nc_50_loss_rate = []
    nc_50_tx_num = []
    nc_75_loss_rate = []
    nc_75_tx_num = []
    nc_100_loss_rate = []
    nc_100_tx_num = []

    # SNC
    snc_50_loss_rate = []
    snc_50_tx_num = []
    snc_75_loss_rate = []
    snc_75_tx_num = []
    snc_100_loss_rate = []
    snc_100_tx_num = []

    # RNC
    rnc_50_loss_rate = []
    rnc_50_tx_num = []
    rnc_75_loss_rate = []
    rnc_75_tx_num = []
    rnc_100_loss_rate = []
    rnc_100_tx_num = []

    hop_one_channel_loss_estimation = []
    hop_two_channel_loss_estimation = []
    total_channel_loss_estimation = []

    # OT ARQ
    average_no_coding_loss_rate = []
    average_no_coding_total_tx_num = []

    # NC
    average_nc_0_loss_rate = []
    average_nc_50_loss_rate = []
    average_nc_75_loss_rate = []
    average_nc_100_loss_rate = []
    average_nc_0_tx_num = []
    average_nc_50_tx_num = []
    average_nc_75_tx_num = []
    average_nc_100_tx_num = []

    # SNC
    average_snc_50_loss_rate = []
    average_snc_75_loss_rate = []
    average_snc_100_loss_rate = []
    average_snc_50_tx_num = []
    average_snc_75_tx_num = []
    average_snc_100_tx_num = []

    # RNC
    average_rnc_50_loss_rate = []
    average_rnc_75_loss_rate = []
    average_rnc_100_loss_rate = []
    average_rnc_50_tx_num = []
    average_rnc_75_tx_num = []
    average_rnc_100_tx_num = []

    average_hop_one_channel_loss_estimation = []
    average_hop_two_channel_loss_estimation = []
    average_total_channel_loss_estimation = []

    for index in range (len (data)):
        # NC 4 + 0
        if data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0:
            if data[index]['loss_rate'] > 0:
                nc_0_loss_rate.append (1)
            else:
                nc_0_loss_rate.append (0)
            nc_0_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
            total_channel_loss_estimation.append (data[index]['loss_rate'])
            hop_one_channel_loss_estimation.append ((float(data[index]['tx_num']) - data_nc_relay[index]['fwd_num']) / data[index]['tx_num'])
            if data_nc_relay[index]['fwd_num'] == 0:
                hop_two_channel_loss_estimation.append (0)
            else:
                hop_two_channel_loss_estimation.append ((data_nc_relay[index]['fwd_num'] - (1 - data[index]['loss_rate']) * data[index]['gen_size']) / data_nc_relay[index]['fwd_num'])
            if len(nc_0_loss_rate) == 50:
                average_nc_0_loss_rate.append (np.mean (nc_0_loss_rate))
                average_nc_0_tx_num.append (np.mean (nc_0_tx_num))
                average_total_channel_loss_estimation.append (np.mean (total_channel_loss_estimation))
                average_hop_one_channel_loss_estimation.append (np.mean (hop_one_channel_loss_estimation))
                average_hop_two_channel_loss_estimation.append (np.mean (hop_two_channel_loss_estimation))
                nc_0_loss_rate = []
                nc_0_tx_num = []
                hop_one_channel_loss_estimation = []
                hop_two_channel_loss_estimation = []
                total_channel_loss_estimation = []
        # NC 4 + 2
        elif data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0.5:
            if data[index]['loss_rate'] > 0:
                nc_50_loss_rate.append (1)
            else:
                nc_50_loss_rate.append (0)
            nc_50_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
            if len(nc_50_loss_rate) == 50:
                average_nc_50_loss_rate.append (np.mean (nc_50_loss_rate))
                average_nc_50_tx_num.append (np.mean (nc_50_tx_num))
                nc_50_loss_rate = []
                nc_50_tx_num = []
        # NC 4 + 3
        elif data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0.75:
            if data[index]['loss_rate'] > 0:
                nc_75_loss_rate.append (1)
            else:
                nc_75_loss_rate.append (0)
            nc_75_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
            if len(nc_75_loss_rate) == 50:
                average_nc_75_loss_rate.append (np.mean (nc_75_loss_rate))
                average_nc_75_tx_num.append (np.mean (nc_75_tx_num))
                nc_75_loss_rate = []
                nc_75_tx_num = []
        # NC 4 + 4
        elif data[index]['type'] == 'block_full' and data[index]['redundancy'] == 1:
            if data[index]['loss_rate'] > 0:
                nc_100_loss_rate.append (1)
            else:
                nc_100_loss_rate.append (0)
            nc_100_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
            if len(nc_100_loss_rate) == 50:
                average_nc_100_loss_rate.append (np.mean (nc_100_loss_rate))
                average_nc_100_tx_num.append (np.mean (nc_100_tx_num))
                nc_100_loss_rate = []
                nc_100_tx_num = []
        # SNC
        elif data[index]['type'] == 'block_sparse':
            if data[index]['redundancy'] == 0.5:
                if data[index]['loss_rate'] > 0:
                    snc_50_loss_rate.append (1)
                else:
                    snc_50_loss_rate.append (0)
                snc_50_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
                if len(snc_50_loss_rate) == 50:
                    average_snc_50_loss_rate.append (np.mean (snc_50_loss_rate))
                    average_snc_50_tx_num.append (np.mean (snc_50_tx_num))
                    snc_50_loss_rate = []
                    snc_50_tx_num = []
            if data[index]['redundancy'] == 0.75:
                if data[index]['loss_rate'] > 0:
                    snc_75_loss_rate.append (1)
                else:
                    snc_75_loss_rate.append (0)
                snc_75_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
                if len(snc_75_loss_rate) == 50:
                    average_snc_75_loss_rate.append (np.mean (snc_75_loss_rate))
                    average_snc_75_tx_num.append (np.mean (snc_75_tx_num))
                    snc_75_loss_rate = []
                    snc_75_tx_num = []
            if data[index]['redundancy'] == 1:
                if data[index]['loss_rate'] > 0:
                    snc_100_loss_rate.append (1)
                else:
                    snc_100_loss_rate.append (0)
                snc_100_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
                if len(snc_100_loss_rate) == 50:
                    average_snc_100_loss_rate.append (np.mean (snc_100_loss_rate))
                    average_snc_100_tx_num.append (np.mean (snc_100_tx_num))
                    snc_100_loss_rate = []
                    snc_100_tx_num = []
        # RNC
        elif data[index]['type'] == 'block_recode':
            if data[index]['redundancy'] == 0.5:
                if data[index]['loss_rate'] > 0:
                    rnc_50_loss_rate.append (1)
                else:
                    rnc_50_loss_rate.append (0)
                rnc_50_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
                if len(rnc_50_loss_rate) == 50:
                    average_rnc_50_loss_rate.append (np.mean (rnc_50_loss_rate))
                    average_rnc_50_tx_num.append (np.mean (rnc_50_tx_num))
                    rnc_50_loss_rate = []
                    rnc_50_tx_num = []
            if data[index]['redundancy'] == 0.75:
                if data[index]['loss_rate'] > 0:
                    rnc_75_loss_rate.append (1)
                else:
                    rnc_75_loss_rate.append (0)
                rnc_75_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
                if len(rnc_75_loss_rate) == 50:
                    average_rnc_75_loss_rate.append (np.mean (rnc_75_loss_rate))
                    average_rnc_75_tx_num.append (np.mean (rnc_75_tx_num))
                    rnc_75_loss_rate = []
                    rnc_75_tx_num = []
            if data[index]['redundancy'] == 1:
                if data[index]['loss_rate'] > 0:
                    rnc_100_loss_rate.append (1)
                else:
                    rnc_100_loss_rate.append (0)
                rnc_100_tx_num.append (data[index]['tx_num'] + data_nc_relay[index]['fwd_num'])
                if len(rnc_100_loss_rate) == 50:
                    average_rnc_100_loss_rate.append (np.mean (rnc_100_loss_rate))
                    average_rnc_100_tx_num.append (np.mean (rnc_100_tx_num))
                    rnc_100_loss_rate = []
                    rnc_100_tx_num = []

    # OT ARQ
    for index in range (len (data_no_src)):
        if data_no_src[index]['type'] == 'no_coding':
            if data_no_dst[index]['loss_rate'] == 0:
                no_coding_loss_rate.append (0)
            else:
                no_coding_loss_rate.append (1)
            no_coding_hop_one_tx_num.append (data_no_src[index]['tx_num'])
            no_coding_hop_two_tx_num.append (data_no_relay[index]['fwd_num'])
            no_coding_total_tx_num.append (data_no_src[index]['tx_num'] + data_no_dst[index]['rx_num'])
            if len(no_coding_loss_rate) == 50:
                average_no_coding_loss_rate.append (np.mean (no_coding_loss_rate))
                no_coding_loss_rate = []
                average_no_coding_total_tx_num.append (np.mean (no_coding_total_tx_num))
                no_coding_total_tx_num = []

    print 'OT ARQ loss 95% confidence interval: [', np.mean (average_no_coding_loss_rate) - np.std (average_no_coding_loss_rate, ddof=1) * 2.0211 / np.sqrt (len (average_no_coding_loss_rate)), '', np.mean (average_no_coding_loss_rate) + np.std (average_no_coding_loss_rate, ddof=1) * 2.0211 / np.sqrt (len (average_no_coding_loss_rate)), ']'
    print 'OT ARQ tx 95% confidence interval: [', np.mean (average_no_coding_total_tx_num) - np.std (average_no_coding_total_tx_num, ddof=1) * 2.0211 / np.sqrt (len (average_no_coding_total_tx_num)), '', np.mean (average_no_coding_total_tx_num) + np.std (average_no_coding_total_tx_num, ddof=1) * 2.0211 / np.sqrt (len (average_no_coding_total_tx_num)), ']'
    print 'std:', np.std (average_no_coding_total_tx_num, ddof=1)
    print 'len:', len (average_no_coding_total_tx_num)

    labels = ['OT ARQ', 'NC 4+0', 'NC 4+2', 'NC 4+3', 'NC 4+4', 'SNC 4+2', 'SNC 4+3', 'SNC 4+4', 'RNC 4+2', 'RNC 4+3', 'RNC 4+4']
    extra_tx = [0, 0, 0.025, 0.027, 0.030, 0.022, 0.023, 0.025, 0.040, 0.046, 0.052]
    #x = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11] # the label locations
    x = [0.5, 1, 1.5, 2, 2.5, 3, 3.5, 4, 4.5, 5, 5.5] # the label location
    width = 0.35  # the width of the bars

    plt.rcParams.update ({'font.size': 18})
    # plot loss statistics
    if plot_loss == True:
        plt.rcParams.update ({'figure.figsize': [8, 4.8]})
        fig, ax = plt.subplots()
        ax.boxplot ([average_no_coding_loss_rate, average_nc_0_loss_rate, average_nc_50_loss_rate, average_nc_75_loss_rate, average_nc_100_loss_rate, average_snc_50_loss_rate, average_snc_75_loss_rate, average_snc_100_loss_rate, average_rnc_50_loss_rate, average_rnc_75_loss_rate, average_rnc_100_loss_rate], showmeans=True, whis=[12.5, 87.5], positions=x, widths=width)
        ax.set_ylabel ('loss probability', fontsize=18)
        ax.set_title ('two hop (total channel loss ~%d%%, 75%% IWR)' % getAverageChannelLoss (average_total_channel_loss_estimation), fontsize=18)
        ax.set_xticks (x)
        ax.set_xticklabels (labels, fontsize=18, rotation=60, ha='right')
        ax.grid (axis='y', linestyle='--', alpha=0.5)
        fig.savefig (fname='two_hop_%s_loss.pdf' % measure_date, bbox_inches='tight')
    # plot transmission statistics
    elif plot_tx == True:
        plt.rcParams.update ({'figure.figsize': [8, 4.8]})
        fig, ax = plt.subplots()
        ax.boxplot ([average_no_coding_total_tx_num, average_nc_0_tx_num, average_nc_50_tx_num, average_nc_75_tx_num, average_nc_100_tx_num, average_snc_50_tx_num, average_snc_75_tx_num, average_snc_100_tx_num, average_rnc_50_tx_num, average_rnc_75_tx_num, average_rnc_100_tx_num], showmeans=True, whis=[12.5, 87.5], positions=x, widths=width)
        ax.set_ylabel ('number of transmissions', fontsize=18)
        ax.set_title ('two hop (total channel loss ~%d%%, 75%% IWR)' % getAverageChannelLoss (average_total_channel_loss_estimation), fontsize=18)
        ax.set_xticks(x)
        ax.set_xticklabels(labels, fontsize=18, rotation=60, ha='right')
        ax.set_ylim([0, max (max(average_no_coding_total_tx_num),max(average_nc_100_tx_num),max(average_snc_100_tx_num),max(average_rnc_100_tx_num)) + 1])
        ax.grid (axis='y', linestyle='--', alpha=0.5)
        fig.savefig (fname='two_hop_%s_tx.pdf' % measure_date, bbox_inches='tight')
    # plot loss vs tx statistics
    elif plot_mix == True:
        fig, ax = plt.subplots()
        plt.grid (linestyle='--', alpha=0.5)
        # NC points
        point_chart = ax.plot(np.mean (average_no_coding_loss_rate), np.mean (average_no_coding_total_tx_num), 'ro', label='OT ARQ', ms=6)
        point_chart = ax.plot(np.mean (average_nc_0_loss_rate), np.mean (average_nc_0_tx_num), 'b*-', label='NC 4+0', ms=6)
        point_chart = ax.plot(np.mean (average_nc_50_loss_rate), np.mean (average_nc_50_tx_num), 'bs-', label='NC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_nc_75_loss_rate), np.mean (average_nc_75_tx_num), 'bo-', label='NC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_nc_100_loss_rate), np.mean (average_nc_100_tx_num), 'b^-', label='NC 4+4', ms=6)
        # SNC points
        point_chart = ax.plot(np.mean (average_snc_50_loss_rate), np.mean (average_snc_50_tx_num), 'gs-', label='SNC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_snc_75_loss_rate), np.mean (average_snc_75_tx_num), 'go-', label='SNC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_snc_100_loss_rate), np.mean (average_snc_100_tx_num), 'g^-', label='SNC 4+4', ms=6)
        # RNC points
        point_chart = ax.plot(np.mean (average_rnc_50_loss_rate), np.mean (average_rnc_50_tx_num), 'ks-', label='RNC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_rnc_75_loss_rate), np.mean (average_rnc_75_tx_num), 'ko-', label='RNC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_rnc_100_loss_rate), np.mean (average_rnc_100_tx_num), 'k^-', label='RNC 4+4', ms=6)

        # NC lines
        l1 = mlines.Line2D([np.mean (average_nc_0_loss_rate),np.mean (average_nc_50_loss_rate)], [np.mean (average_nc_0_tx_num),np.mean (average_nc_50_tx_num)], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l1)
        l2 = mlines.Line2D([np.mean (average_nc_50_loss_rate),np.mean (average_nc_75_loss_rate)], [np.mean (average_nc_50_tx_num),np.mean (average_nc_75_tx_num)], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_nc_75_loss_rate),np.mean (average_nc_100_loss_rate)], [np.mean (average_nc_75_tx_num),np.mean (average_nc_100_tx_num)], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l3)
        # SNC lines
        l2 = mlines.Line2D([np.mean (average_snc_50_loss_rate),np.mean (average_snc_75_loss_rate)], [np.mean (average_snc_50_tx_num),np.mean (average_snc_75_tx_num)], color='g', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_snc_75_loss_rate),np.mean (average_snc_100_loss_rate)], [np.mean (average_snc_75_tx_num),np.mean (average_snc_100_tx_num)], color='g', linestyle=':', alpha=0.5)
        ax.add_line(l3)
        # RNC lines
        l2 = mlines.Line2D([np.mean (average_rnc_50_loss_rate),np.mean (average_rnc_75_loss_rate)], [np.mean (average_rnc_50_tx_num),np.mean (average_rnc_75_tx_num)], color='k', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_rnc_75_loss_rate),np.mean (average_rnc_100_loss_rate)], [np.mean (average_rnc_75_tx_num),np.mean (average_rnc_100_tx_num)], color='k', linestyle=':', alpha=0.5)
        ax.add_line(l3)

        # plot percentile intervals
        # OT ARQ
        plotPercentile (fig, ax, np.mean (average_no_coding_loss_rate), np.mean (average_no_coding_total_tx_num), average_no_coding_loss_rate, average_no_coding_total_tx_num, extra_tx[0], 'mix', 'r')
        # NC
        plotPercentile (fig, ax, np.mean (average_nc_0_loss_rate), np.mean (average_nc_0_tx_num), average_nc_0_loss_rate, average_nc_0_tx_num, extra_tx[1], 'mix', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_50_loss_rate), np.mean (average_nc_50_tx_num), average_nc_50_loss_rate, average_nc_50_tx_num, extra_tx[2], 'mix', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_75_loss_rate), np.mean (average_nc_75_tx_num), average_nc_75_loss_rate, average_nc_75_tx_num, extra_tx[3], 'mix', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_100_loss_rate), np.mean (average_nc_100_tx_num), average_nc_100_loss_rate, average_nc_100_tx_num, extra_tx[4], 'mix', 'b')
        # SNC
        plotPercentile (fig, ax, np.mean (average_snc_50_loss_rate), np.mean (average_snc_50_tx_num), average_snc_50_loss_rate, average_snc_50_tx_num, extra_tx[5], 'mix', 'g')
        plotPercentile (fig, ax, np.mean (average_snc_75_loss_rate), np.mean (average_snc_75_tx_num), average_snc_75_loss_rate, average_snc_75_tx_num, extra_tx[6], 'mix', 'g')
        plotPercentile (fig, ax, np.mean (average_snc_100_loss_rate), np.mean (average_snc_100_tx_num), average_snc_100_loss_rate, average_snc_100_tx_num, extra_tx[7], 'mix', 'g')
        # RNC
        plotPercentile (fig, ax, np.mean (average_rnc_50_loss_rate), np.mean (average_rnc_50_tx_num), average_rnc_50_loss_rate, average_rnc_50_tx_num, extra_tx[8], 'mix', 'k')
        plotPercentile (fig, ax, np.mean (average_rnc_75_loss_rate), np.mean (average_rnc_75_tx_num), average_rnc_75_loss_rate, average_rnc_75_tx_num, extra_tx[9], 'mix', 'k')
        plotPercentile (fig, ax, np.mean (average_rnc_100_loss_rate), np.mean (average_rnc_100_tx_num), average_rnc_100_loss_rate, average_rnc_100_tx_num, extra_tx[10], 'mix', 'k')

        ax.set_title ('two hop (total channel loss ~%d%%)' % getAverageChannelLoss (average_total_channel_loss_estimation), fontsize=18)
        ax.set_xlabel ('loss probability', fontsize=18)
        ax.set_ylabel ('number of transmissions', fontsize=18)
        #ax.set_ylim([4, 21])
        ax.set_xlim([0, 0.5])
        plt.legend(fontsize=16, ncol=1, loc='center right', bbox_to_anchor=(1.4, 0.5))
        fig.savefig (fname='two_hop_%s_mix.pdf' % measure_date, bbox_inches='tight')
    # plot channel loss statistics
    elif plot_channel_loss == True:
        fig, ax = plt.subplots()
        plt.grid (linestyle='--', alpha=0.5)
        x = np.arange (len (average_hop_one_channel_loss_estimation))  # the label locations
        ax.plot (x, average_hop_one_channel_loss_estimation, 'g-', label='hop one')
        ax.plot (x, average_hop_two_channel_loss_estimation, 'b-', label='hop two')
        ax.plot (x, average_total_channel_loss_estimation, 'r-', label='total')
        ax.set_title ('total channel loss ~%d%%' % getAverageChannelLoss (average_total_channel_loss_estimation), fontsize=18)
        ax.set_ylim ([0, max (average_total_channel_loss_estimation) + 0.1])
        plt.legend (fontsize=16)
        ax.set_xlabel ('time [round]', fontsize=18)
        ax.set_ylabel ('channel loss estimation', fontsize=18)
        fig.savefig (fname='two_hop_%s_channel_loss_estimation.pdf' % measure_date, bbox_inches='tight')

    # plot retransmission statistics
    elif plot_retx_stat == True:
        plotRetransmissionStat (data, data_relay, fig, ax, measure_date, average_hop_one_channel_loss_estimation, average_hop_two_channel_loss_estimation)
    # plot final energy vs loss staistics
    elif plot_final == True:
        fig, ax = plt.subplots()
        plt.grid (linestyle='--', alpha=0.5)
        # OT ARQ point
        point_chart = ax.plot(np.mean (average_no_coding_loss_rate), 4 * (np.mean (average_no_coding_total_tx_num) + extra_tx[0]), 'ro', label='OT ARQ', ms=6)
        # NC points
        point_chart = ax.plot(np.mean (average_nc_0_loss_rate), 4 * (np.mean (average_nc_0_tx_num) + extra_tx[1]), 'b*-', label='NC 4+0', ms=6)
        point_chart = ax.plot(np.mean (average_nc_50_loss_rate), 4 * (np.mean (average_nc_50_tx_num) + extra_tx[2]), 'bs-', label='NC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_nc_75_loss_rate), 4 * (np.mean (average_nc_75_tx_num) + extra_tx[3]), 'bo-', label='NC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_nc_100_loss_rate), 4 * (np.mean (average_nc_100_tx_num) + extra_tx[4]), 'b^-', label='NC 4+4', ms=6)
        # SNC points
        point_chart = ax.plot(np.mean (average_snc_50_loss_rate), 4 * (np.mean (average_snc_50_tx_num) + extra_tx[5]), 'gs-', label='SNC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_snc_75_loss_rate), 4 * (np.mean (average_snc_75_tx_num) + extra_tx[6]), 'go-', label='SNC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_snc_100_loss_rate), 4 * (np.mean (average_snc_100_tx_num) + extra_tx[7]), 'g^-', label='SNC 4+4', ms=6)
        # RNC points
        point_chart = ax.plot(np.mean (average_rnc_50_loss_rate), 4 * (np.mean (average_rnc_50_tx_num) + extra_tx[8]), 'ks-', label='RNC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_rnc_75_loss_rate), 4 * (np.mean (average_rnc_75_tx_num) + extra_tx[9]), 'ko-', label='RNC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_rnc_100_loss_rate), 4 * (np.mean (average_rnc_100_tx_num) + extra_tx[10]), 'k^-', label='RNC 4+4', ms=6)

        # NC lines
        l1 = mlines.Line2D([np.mean (average_nc_0_loss_rate),np.mean (average_nc_50_loss_rate)], [4 * (np.mean (average_nc_0_tx_num) + extra_tx[1]),4 * (np.mean (average_nc_50_tx_num) + extra_tx[2])], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l1)
        l2 = mlines.Line2D([np.mean (average_nc_50_loss_rate),np.mean (average_nc_75_loss_rate)], [4 * (np.mean (average_nc_50_tx_num) + extra_tx[2]),4 * (np.mean (average_nc_75_tx_num) + extra_tx[3])], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_nc_75_loss_rate),np.mean (average_nc_100_loss_rate)], [4 * (np.mean (average_nc_75_tx_num) + extra_tx[3]),4 * (np.mean (average_nc_100_tx_num) + extra_tx[4])], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l3)
        # SNC lines
        l2 = mlines.Line2D([np.mean (average_snc_50_loss_rate),np.mean (average_snc_75_loss_rate)], [4 * (np.mean (average_snc_50_tx_num) + extra_tx[5]),4 * (np.mean (average_snc_75_tx_num) + extra_tx[6])], color='g', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_snc_75_loss_rate),np.mean (average_snc_100_loss_rate)], [4 * (np.mean (average_snc_75_tx_num) + extra_tx[6]),4 * (np.mean (average_snc_100_tx_num) + extra_tx[7])], color='g', linestyle=':', alpha=0.5)
        ax.add_line(l3)
        # RNC lines
        l2 = mlines.Line2D([np.mean (average_rnc_50_loss_rate),np.mean (average_rnc_75_loss_rate)], [4 * (np.mean (average_rnc_50_tx_num) + extra_tx[8]),4 * (np.mean (average_rnc_75_tx_num) + extra_tx[9])], color='k', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_rnc_75_loss_rate),np.mean (average_rnc_100_loss_rate)], [4 * (np.mean (average_rnc_75_tx_num) + extra_tx[9]),4 * (np.mean (average_rnc_100_tx_num) + extra_tx[10])], color='k', linestyle=':', alpha=0.5)
        ax.add_line(l3)

        # plot percentile intervals
        # OT ARQ
        plotPercentile (fig, ax, np.mean (average_no_coding_loss_rate), 4 * (np.mean (average_no_coding_total_tx_num) + extra_tx[0]), average_no_coding_loss_rate, average_no_coding_total_tx_num, extra_tx[0], 'final', 'r')
        # NC
        plotPercentile (fig, ax, np.mean (average_nc_0_loss_rate), 4 * (np.mean (average_nc_0_tx_num) + extra_tx[1]), average_nc_0_loss_rate, average_nc_0_tx_num, extra_tx[1], 'final', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_50_loss_rate), 4 * (np.mean (average_nc_50_tx_num) + extra_tx[2]), average_nc_50_loss_rate, average_nc_50_tx_num, extra_tx[2], 'final', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_75_loss_rate), 4 * (np.mean (average_nc_75_tx_num) + extra_tx[3]), average_nc_75_loss_rate, average_nc_75_tx_num, extra_tx[3], 'final', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_100_loss_rate), 4 * (np.mean (average_nc_100_tx_num) + extra_tx[4]), average_nc_100_loss_rate, average_nc_100_tx_num, extra_tx[4], 'final', 'b')
        # SNC
        plotPercentile (fig, ax, np.mean (average_snc_50_loss_rate), 4 * (np.mean (average_snc_50_tx_num) + extra_tx[5]), average_snc_50_loss_rate, average_snc_50_tx_num, extra_tx[5], 'final', 'g')
        plotPercentile (fig, ax, np.mean (average_snc_75_loss_rate), 4 * (np.mean (average_snc_75_tx_num) + extra_tx[6]), average_snc_75_loss_rate, average_snc_75_tx_num, extra_tx[6], 'final', 'g')
        plotPercentile (fig, ax, np.mean (average_snc_100_loss_rate), 4 * (np.mean (average_snc_100_tx_num) + extra_tx[7]), average_snc_100_loss_rate, average_snc_100_tx_num, extra_tx[7], 'final', 'g')
        # RNC
        plotPercentile (fig, ax, np.mean (average_rnc_50_loss_rate), 4 * (np.mean (average_rnc_50_tx_num) + extra_tx[8]), average_rnc_50_loss_rate, average_rnc_50_tx_num, extra_tx[8], 'final', 'k')
        plotPercentile (fig, ax, np.mean (average_rnc_75_loss_rate), 4 * (np.mean (average_rnc_75_tx_num) + extra_tx[9]), average_rnc_75_loss_rate, average_rnc_75_tx_num, extra_tx[9], 'final', 'k')
        plotPercentile (fig, ax, np.mean (average_rnc_100_loss_rate), 4 * (np.mean (average_rnc_100_tx_num) + extra_tx[10]), average_rnc_100_loss_rate, average_rnc_100_tx_num, extra_tx[10], 'final', 'k')
        # plot configuration
        ax.set_title ('two hop (total channel loss ~%d%%)' % getAverageChannelLoss (average_total_channel_loss_estimation), fontsize=18)
        ax.set_xlabel ('loss probability', fontsize=18)
        ax.set_ylabel ('energy consumption [uJ]', fontsize=18)
        #ax.set_ylim([20, 85])
        #ax.set_xlim([0, 0.5])
        plt.legend(fontsize=16, ncol=1, loc='center right', bbox_to_anchor=(1.4, 0.5))
        fig.savefig (fname='two_hop_%s_final.pdf' % measure_date, bbox_inches='tight')

    #fig.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
