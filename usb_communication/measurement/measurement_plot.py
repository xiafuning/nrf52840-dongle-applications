#!/usr/bin/python

import sys
import getopt
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.lines as mlines


#===========================================
def autolabel (rects, ax):
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
    print ('Usage: [-h --help] [-t --type <loss/tx/mix>] [-j --json <json file name>]')
    print ('Options:')
    print ('\t-t --type\ttype of measurement to be plotted\tDefault: loss')
    print ('\t-j --json\tjson file name')
    print ('\t-h --help\tthis help documentation')

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

    no_coding_loss_rate = []
    no_coding_tx_num = []
    nc_0_loss_rate = []
    nc_0_tx_num = []
    nc_50_loss_rate = []
    nc_50_tx_num = []
    nc_100_loss_rate = []
    nc_100_tx_num = []
    channel_loss_estimation = []

    average_no_coding_loss_rate = []
    average_no_coding_tx_num = []
    average_nc_0_loss_rate = []
    average_nc_50_loss_rate = []
    average_nc_100_loss_rate = []
    average_channel_loss_estimation = []

    for index in range (len (data)):
        if data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0:
            if data[index]['loss_rate'] > 0:
                nc_0_loss_rate.append (1)
            else:
                nc_0_loss_rate.append (0)
            nc_0_tx_num.append (data[index]['tx_num'])
            channel_loss_estimation.append (data[index]['loss_rate'])
            if len(nc_0_loss_rate) == 50:
                average_nc_0_loss_rate.append (np.mean (nc_0_loss_rate))
                average_channel_loss_estimation.append (np.mean (channel_loss_estimation))
                nc_0_loss_rate = []
                channel_loss_estimation = []
        elif data[index]['type'] == 'block_full' and data[index]['redundancy'] == 0.5:
            if data[index]['loss_rate'] > 0:
                nc_50_loss_rate.append (1)
            else:
                nc_50_loss_rate.append (0)
            nc_50_tx_num.append (data[index]['tx_num'])
            if len(nc_50_loss_rate) == 50:
                average_nc_50_loss_rate.append (np.mean (nc_50_loss_rate))
                nc_50_loss_rate = []
        elif data[index]['type'] == 'block_full' and data[index]['redundancy'] == 1:
            if data[index]['loss_rate'] > 0:
                nc_100_loss_rate.append (1)
            else:
                nc_100_loss_rate.append (0)
            nc_100_tx_num.append (data[index]['tx_num'])
            if len(nc_100_loss_rate) == 50:
                average_nc_100_loss_rate.append (np.mean (nc_100_loss_rate))
                nc_100_loss_rate = []
        elif data[index]['type'] == 'no_coding':
            no_coding_loss_rate.append (data[index]['loss_rate'])
            no_coding_tx_num.append (data[index]['tx_num'])
            if len(no_coding_loss_rate) == 50:
                average_no_coding_loss_rate.append (np.mean (no_coding_loss_rate))
                average_no_coding_tx_num.append (np.mean (no_coding_tx_num))
                no_coding_loss_rate = []
                no_coding_tx_num = []

    labels = ['', 'OT ARQ', 'NC 4+0', 'NC 4+2', 'NC 4+4']

    x = np.arange(len(labels))  # the label locations
    width = 0.35  # the width of the bars

    plt.rcParams.update({'font.size': 20})
    fig, ax = plt.subplots()
    if plot_loss == True:
        ax.boxplot ([average_no_coding_loss_rate, average_nc_0_loss_rate, average_nc_50_loss_rate, average_nc_100_loss_rate, []], labels=x, showmeans=True)
        ax.set_ylabel ('loss probability', fontsize=20)
        ax.set_title ('one hop (channel loss ~30%)', fontsize=20)
        ax.set_xticks (x)
        ax.set_xticklabels (labels, fontsize=20, rotation=60, ha='right')
        #plt.grid (linestyle='--')
        fig.savefig (fname='one_hop_%.2f_loss.pdf' % (np.mean (average_channel_loss_estimation) * 100), bbox_inches='tight')
    elif plot_tx == True:
        ax.boxplot ([average_no_coding_tx_num, nc_0_tx_num, nc_50_tx_num, nc_100_tx_num, []], labels=x, showmeans=True)
        ax.set_ylabel ('number of transmission', fontsize=20)
        ax.set_title ('one hop (channel loss ~30%)', fontsize=20)
        ax.set_xticks(x)
        ax.set_xticklabels(labels, fontsize=20, rotation=60, ha='right')
        ax.set_ylim([0,16])
        fig.savefig (fname='one_hop_%.2f_tx.pdf' % (np.mean (average_channel_loss_estimation) * 100), bbox_inches='tight')
    elif plot_mix == True:
        point_chart = ax.plot(np.mean (average_no_coding_loss_rate), np.mean (average_no_coding_tx_num), 'ro', label='OT ARQ', ms=10)
        point_chart = ax.plot(np.mean (average_nc_0_loss_rate), np.mean (nc_0_tx_num), 'b*-', label='NC 4+0', ms=10)
        point_chart = ax.plot(np.mean (average_nc_50_loss_rate), np.mean (nc_50_tx_num), 'b+-', label='NC 4+2', ms=10)
        point_chart = ax.plot(np.mean (average_nc_100_loss_rate), np.mean (nc_100_tx_num), 'b^-', label='NC 4+4', ms=10)
        l1 = mlines.Line2D([np.mean (average_nc_0_loss_rate),np.mean (average_nc_50_loss_rate)], [np.mean (nc_0_tx_num),np.mean (nc_50_tx_num)], color='b')
        ax.add_line(l1)
        l2 = mlines.Line2D([np.mean (average_nc_50_loss_rate),np.mean (average_nc_100_loss_rate)], [np.mean (nc_50_tx_num),np.mean (nc_100_tx_num)], color='b')
        ax.add_line(l2)
        ax.set_title ('one hop (channel loss ~30%)', fontsize=20)
        ax.set_xlabel ('loss probability', fontsize=20)
        ax.set_ylabel ('number of transmission', fontsize=20)
        ax.set_ylim([0,16])
        plt.legend(fontsize=20, ncol=2, loc='lower center', bbox_to_anchor=(0.5, -0.5))
        fig.savefig (fname='one_hop_%.2f_mix.pdf' % (np.mean (average_channel_loss_estimation) * 100), bbox_inches='tight')
        '''
        for i in range(len(labels)):
            ax.annotate(labels[i],
                        xy=(loss_rate[i],
                        tx_num[i]), xytext=(0, 3),
                        textcoords="offset points",
                        ha='center', va='bottom',
                        fontsize=16)
        '''
    #fig.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()
