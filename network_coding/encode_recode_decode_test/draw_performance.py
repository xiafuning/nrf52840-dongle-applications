#!/usr/bin/python  

import json
import numpy as np
import matplotlib.pyplot as plt  

def main():
    with open('/home/xiafuning/diplom-arbeit/src/kodo-rlnc/examples/encode_recode_decode_test/temp.dump', 'r') as f:
        data = json.load(f)

    loss_rate = [0.1, 0.3, 0.5, 0.7]
    no_coding = []
    block_full = []

    for index in range(len(data)):
        for i in range(len(loss_rate)):
            if data[index]['type'] == 'block_full' and data[index]['loss_rate'] == loss_rate[i]:
                block_full.append(data[index]['average_tx_num'])
            elif data[index]['type'] == 'no_coding' and data[index]['loss_rate'] == loss_rate[i]:
                no_coding.append(data[index]['average_tx_num'])

    l1=plt.plot(loss_rate, no_coding, 'ro-', label='no coding')
    l2=plt.plot(loss_rate, block_full, 'go-', label='block full')

    plt.title('generation size = 10', fontsize=20)
    plt.xlabel('loss rate', fontsize=20)
    plt.ylabel('average transmission number n', fontsize=20)
    plt.legend(fontsize=20)
    plt.grid()    
    plt.show()

if __name__ == '__main__':
    main()
