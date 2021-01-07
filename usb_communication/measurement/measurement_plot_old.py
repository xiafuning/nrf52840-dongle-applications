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
def plotRetransmissionStat (data, fig, ax, measure_date):
#===========================================
    retx_stat = {}
    for index in range (len (data)):
        if data[index]['type'] == 'no_coding':
            if retx_stat.has_key (data[index]['tx_num']) == True:
                retx_stat[data[index]['tx_num']] = retx_stat[data[index]['tx_num']] + 1
            else:
                retx_stat[data[index]['tx_num']] = 1
    sorted (retx_stat.keys())
    total_num = sum (retx_stat.values())
    for index in range (len (retx_stat)):
        retx_stat[retx_stat.keys()[index]] = float (retx_stat[retx_stat.keys()[index]]) / total_num

    x = retx_stat.keys()  # the label locations
    ax.bar (x, retx_stat.values())
    ax.set_xticks (retx_stat.keys())
    ax.set_title ('one hop (channel loss ~%d%%)' % getAverageChannelLoss (channel_loss_estimation), fontsize=20)
    ax.set_xlabel ('number of transmission', fontsize=20)
    ax.set_ylabel ('percentage', fontsize=20)
    #ax.set_ylim([0,1])
    fig.savefig (fname='one_hop_%s_retx_stat.pdf' % measure_date, bbox_inches='tight')

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
def plotCiInBoxplot (fig, ax, x, data):
#===========================================
    bound_lower = np.mean (data) - np.std (data, ddof=1) * 2.0211 / np.sqrt (len (data))
    bound_upper = np.mean (data) + np.std (data, ddof=1) * 2.0211 / np.sqrt (len (data))

    l1 = mlines.Line2D([x,x], [bound_lower,bound_upper], color='b', zorder=0, marker='_', markersize=20)
    ax.add_line(l1)

#===========================================
def main():
#===========================================
    opts, argv = getopt.getopt (sys.argv[1:],'-h-t:-j:',['help', 'type', 'json'])
    if len (opts) == 0:
        helpInfo()
        exit()

    plot_loss = True
    plot_tx = False
    plot_mix = False
    plot_channel_loss = False
    plot_retx_stat = False
    plot_final = False
    json_file = ''

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
            json_file = value
        else:
            helpInfo()
            exit()

    # open json file
    with open (json_file, 'r') as f:
        data = json.load(f)

    measure_date = getDate (json_file)

    no_coding_loss_rate = []
    no_coding_tx_num = []
    nc_0_loss_rate = []
    nc_0_tx_num = []
    nc_50_loss_rate = []
    nc_50_tx_num = []
    nc_75_loss_rate = []
    nc_75_tx_num = []
    nc_100_loss_rate = []
    nc_100_tx_num = []
    channel_loss_estimation = []

    average_no_coding_loss_rate = []
    average_nc_0_loss_rate = []
    average_nc_50_loss_rate = []
    average_nc_75_loss_rate = []
    average_nc_100_loss_rate = []
    average_no_coding_tx_num = []
    average_nc_0_tx_num = []
    average_nc_50_tx_num = []
    average_nc_75_tx_num = []
    average_nc_100_tx_num = []
    average_channel_loss_estimation = []

    for index in range (len (data)):
        # NC 4 + 0
        if data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0:
            if data[index]['loss_rate'] > 0:
                nc_0_loss_rate.append (1)
            else:
                nc_0_loss_rate.append (0)
            nc_0_tx_num.append (data[index]['tx_num'])
            channel_loss_estimation.append (data[index]['loss_rate'])
            if len(nc_0_loss_rate) == 50:
                average_nc_0_loss_rate.append (np.mean (nc_0_loss_rate))
                average_nc_0_tx_num.append (np.mean (nc_0_tx_num))
                average_channel_loss_estimation.append (np.mean (channel_loss_estimation))
                nc_0_loss_rate = []
                nc_0_tx_num = []
                channel_loss_estimation = []
        # NC 4 + 2
        elif data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0.5:
            if data[index]['loss_rate'] > 0:
                nc_50_loss_rate.append (1)
            else:
                nc_50_loss_rate.append (0)
            nc_50_tx_num.append (data[index]['tx_num'])
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
            nc_75_tx_num.append (data[index]['tx_num'])
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
            nc_100_tx_num.append (data[index]['tx_num'])
            if len(nc_100_loss_rate) == 50:
                average_nc_100_loss_rate.append (np.mean (nc_100_loss_rate))
                average_nc_100_tx_num.append (np.mean (nc_100_tx_num))
                nc_100_loss_rate = []
                nc_100_tx_num = []
        # OT ARQ
        elif data[index]['type'] == 'no_coding':
            no_coding_loss_rate.append (data[index]['loss_rate'])
            no_coding_tx_num.append (data[index]['tx_num'])
            if len(no_coding_loss_rate) == 50:
                average_no_coding_loss_rate.append (np.mean (no_coding_loss_rate))
                average_no_coding_tx_num.append (np.mean (no_coding_tx_num))
                no_coding_loss_rate = []
                no_coding_tx_num = []

    labels = ['OT ARQ', 'NC 4+0', 'NC 4+2', 'NC 4+3', 'NC 4+4']
    extra_tx = [0, 0, 0.025, 0.027, 0.030]
    x = [1, 2, 3, 4, 5]  # the label locations
    width = 0.35  # the width of the bars

    plt.rcParams.update({'font.size': 18})
    fig, ax = plt.subplots()
    # plot loss statistics
    if plot_loss == True:
        # plot box plot
        ax.boxplot ([average_no_coding_loss_rate, average_nc_0_loss_rate, average_nc_50_loss_rate, average_nc_75_loss_rate, average_nc_100_loss_rate], showmeans=True, whis=[2.5, 97.5])
        # plot confidence interval
        # OT ARQ
        plotCiInBoxplot (fig, ax, 1, average_no_coding_loss_rate)
        # NC 4 + 0
        plotCiInBoxplot (fig, ax, 2, average_nc_0_loss_rate)
        # NC 4 + 2
        plotCiInBoxplot (fig, ax, 3, average_nc_50_loss_rate)
        # NC 4 + 3
        plotCiInBoxplot (fig, ax, 4, average_nc_75_loss_rate)
        # NC 4 + 4
        plotCiInBoxplot (fig, ax, 5, average_nc_100_loss_rate)

        ax.set_ylabel ('loss probability', fontsize=18)
        ax.set_title ('one hop (channel loss ~%d%%, 95%% IWR)' % getAverageChannelLoss (average_channel_loss_estimation), fontsize=18)
        ax.set_xticks (x)
        ax.set_xticklabels (labels, fontsize=18, rotation=60, ha='right')
        ax.grid (axis='y', linestyle='--', alpha=0.5)
        fig.savefig (fname='one_hop_%s_loss.pdf' % measure_date, bbox_inches='tight')
    # plot transmission statistics
    elif plot_tx == True:
        # plot box plot
        ax.boxplot ([average_no_coding_tx_num, average_nc_0_tx_num, average_nc_50_tx_num, average_nc_75_tx_num, average_nc_100_tx_num], showmeans=True, whis=[2.5, 97.5])
        # plot confidence interval
        # OT ARQ
        plotCiInBoxplot (fig, ax, 1, average_no_coding_tx_num)
        # NC 4 + 0
        plotCiInBoxplot (fig, ax, 2, average_nc_0_tx_num)
        # NC 4 + 2
        plotCiInBoxplot (fig, ax, 3, average_nc_50_tx_num)
        # NC 4 + 3
        plotCiInBoxplot (fig, ax, 4, average_nc_75_tx_num)
        # NC 4 + 4
        plotCiInBoxplot (fig, ax, 5, average_nc_100_tx_num)

        ax.set_ylabel ('number of transmissions', fontsize=18)
        ax.set_title ('one hop (channel loss ~%d%%, 95%% IWR)' % getAverageChannelLoss (average_channel_loss_estimation), fontsize=18)
        ax.set_xticks(x)
        ax.set_xticklabels(labels, fontsize=18, rotation=60, ha='right')
        ax.set_ylim([0,max(max(average_no_coding_tx_num), max(average_nc_100_tx_num))+1])
        ax.grid (axis='y', linestyle='--', alpha=0.5)
        fig.savefig (fname='one_hop_%s_tx.pdf' % measure_date, bbox_inches='tight')
    # plot loss vs tx statistics
    elif plot_mix == True:
        plt.grid (linestyle='--', alpha=0.5)
        # plot points
        point_chart = ax.plot(np.mean (average_no_coding_loss_rate), np.mean (average_no_coding_tx_num), 'ro', label='OT ARQ', ms=6)
        point_chart = ax.plot(np.mean (average_nc_0_loss_rate), np.mean (average_nc_0_tx_num), 'b*-', label='NC 4+0', ms=6)
        point_chart = ax.plot(np.mean (average_nc_50_loss_rate), np.mean (average_nc_50_tx_num), 'bs-', label='NC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_nc_75_loss_rate), np.mean (average_nc_75_tx_num), 'bo-', label='NC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_nc_100_loss_rate), np.mean (average_nc_100_tx_num), 'b^-', label='NC 4+4', ms=6)
        # plot lines
        l1 = mlines.Line2D([np.mean (average_nc_0_loss_rate),np.mean (average_nc_50_loss_rate)], [np.mean (average_nc_0_tx_num),np.mean (average_nc_50_tx_num)], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l1)
        l2 = mlines.Line2D([np.mean (average_nc_50_loss_rate),np.mean (average_nc_75_loss_rate)], [np.mean (average_nc_50_tx_num),np.mean (average_nc_75_tx_num)], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_nc_75_loss_rate),np.mean (average_nc_100_loss_rate)], [np.mean (average_nc_75_tx_num),np.mean (average_nc_100_tx_num)], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l3)
        # plot percentile intervals
        # ot arq
        plotPercentile (fig, ax, np.mean (average_no_coding_loss_rate), np.mean (average_no_coding_tx_num), average_no_coding_loss_rate, average_no_coding_tx_num, extra_tx[0], 'mix', 'r')
        # rlnc
        plotPercentile (fig, ax, np.mean (average_nc_0_loss_rate), np.mean (average_nc_0_tx_num), average_nc_0_loss_rate, average_nc_0_tx_num, extra_tx[1], 'mix', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_50_loss_rate), np.mean (average_nc_50_tx_num), average_nc_50_loss_rate, average_nc_50_tx_num, extra_tx[2], 'mix', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_75_loss_rate), np.mean (average_nc_75_tx_num), average_nc_75_loss_rate, average_nc_75_tx_num, extra_tx[3], 'mix', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_100_loss_rate), np.mean (average_nc_100_tx_num), average_nc_100_loss_rate, average_nc_100_tx_num, extra_tx[4], 'mix', 'b')
        ax.set_title ('one hop (channel loss ~%d%%)' % getAverageChannelLoss (average_channel_loss_estimation), fontsize=18)
        ax.set_xlabel ('loss probability', fontsize=18)
        ax.set_ylabel ('number of transmissions', fontsize=18)
        #ax.set_ylim([3,10])
        ax.set_xlim([-0.01, 0.4])
        #ax.set_xlim([0, 0.8])
        plt.legend(fontsize=16, ncol=1, loc='center right', bbox_to_anchor=(1.38, 0.5))
        fig.savefig (fname='one_hop_%s_mix.pdf' % measure_date, bbox_inches='tight')
   # plot channel loss statistics
    elif plot_channel_loss == True:
        plt.grid (linestyle='--')
        x = np.arange(len(average_channel_loss_estimation))  # the label locations
        ax.plot(x, average_channel_loss_estimation, 'b-', label='channel loss')
        ax.set_title ('one hop (channel loss ~%d%%)' % getAverageChannelLoss (average_channel_loss_estimation), fontsize=18)
        ax.set_xlabel ('time [round]', fontsize=18)
        ax.set_ylabel ('channel loss estimation', fontsize=18)
        ax.set_ylim([0, max (average_channel_loss_estimation) + 0.1])
        ax.grid (linestyle='--', alpha=0.5)
        fig.savefig (fname='one_hop_%s_channel_loss_estimation.pdf' % measure_date, bbox_inches='tight')
    # plot retransmission statistics
    elif plot_retx_stat == True:
        plotRetransmissionStat (data, fig, ax, measure_date)
    # plot energy vs loss statistics
    elif plot_final == True:
        plt.grid (linestyle='--', alpha=0.5)
        # ot arq point
        point_chart = ax.plot(np.mean (average_no_coding_loss_rate), 4 * (np.mean (average_no_coding_tx_num) + extra_tx[0]), 'ro', label='OT ARQ', ms=6)
        # rlnc points
        point_chart = ax.plot(np.mean (average_nc_0_loss_rate), 4 * (np.mean (average_nc_0_tx_num) + extra_tx[1]), 'b*-', label='NC 4+0', ms=6)
        point_chart = ax.plot(np.mean (average_nc_50_loss_rate), 4 * (np.mean (average_nc_50_tx_num) + extra_tx[2]), 'bs-', label='NC 4+2', ms=6)
        point_chart = ax.plot(np.mean (average_nc_75_loss_rate), 4 * (np.mean (average_nc_75_tx_num) + extra_tx[3]), 'bo-', label='NC 4+3', ms=6)
        point_chart = ax.plot(np.mean (average_nc_100_loss_rate), 4 * (np.mean (average_nc_100_tx_num) + extra_tx[4]), 'b^-', label='NC 4+4', ms=6)

        # rlnc lines
        l1 = mlines.Line2D([np.mean (average_nc_0_loss_rate),np.mean (average_nc_50_loss_rate)], [4 * (np.mean (average_nc_0_tx_num) + extra_tx[1]),4 * (np.mean (average_nc_50_tx_num) + extra_tx[2])], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l1)
        l2 = mlines.Line2D([np.mean (average_nc_50_loss_rate),np.mean (average_nc_75_loss_rate)], [4 * (np.mean (average_nc_50_tx_num) + extra_tx[2]),4 * (np.mean (average_nc_75_tx_num) + extra_tx[3])], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l2)
        l3 = mlines.Line2D([np.mean (average_nc_75_loss_rate),np.mean (average_nc_100_loss_rate)], [4 * (np.mean (average_nc_75_tx_num) + extra_tx[3]),4 * (np.mean (average_nc_100_tx_num) + extra_tx[4])], color='b', linestyle=':', alpha=0.5)
        ax.add_line(l3)

        # plot percentile intervals
        # ot arq
        plotPercentile (fig, ax, np.mean (average_no_coding_loss_rate), 4 * (np.mean (average_no_coding_tx_num) + extra_tx[0]), average_no_coding_loss_rate, average_no_coding_tx_num, extra_tx[0], 'final', 'r')
        # rlnc
        plotPercentile (fig, ax, np.mean (average_nc_0_loss_rate), 4 * (np.mean (average_nc_0_tx_num) + extra_tx[1]), average_nc_0_loss_rate, average_nc_0_tx_num, extra_tx[1], 'final', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_50_loss_rate), 4 * (np.mean (average_nc_50_tx_num) + extra_tx[2]), average_nc_50_loss_rate, average_nc_50_tx_num, extra_tx[2], 'final', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_75_loss_rate), 4 * (np.mean (average_nc_75_tx_num) + extra_tx[3]), average_nc_75_loss_rate, average_nc_75_tx_num, extra_tx[3], 'final', 'b')
        plotPercentile (fig, ax, np.mean (average_nc_100_loss_rate), 4 * (np.mean (average_nc_100_tx_num) + extra_tx[4]), average_nc_100_loss_rate, average_nc_100_tx_num, extra_tx[4], 'final', 'b')

        # plot configuration
        ax.set_title ('one hop (channel loss ~%d%%)' % getAverageChannelLoss (average_channel_loss_estimation), fontsize=18)
        ax.set_xlabel ('loss probability', fontsize=18)
        ax.set_ylabel ('energy consumption [uJ]', fontsize=18)
        ax.set_ylim([15, 35])
        #ax.set_xlim([-0.01, 0.4])
        ax.set_xlim([0, 0.8])
        plt.legend(fontsize=16, ncol=1, loc='center right', bbox_to_anchor=(1.38, 0.5))
        fig.savefig (fname='one_hop_%s_final.pdf' % measure_date, bbox_inches='tight')

    #fig.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
