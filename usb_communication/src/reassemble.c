#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "lowpan.h"
#include "reassemble.h"
//#include "fsm.h"

// variable definitions
static reassembler_t m_reassembler;


/**
 * @brief check if a packet needs reassemble
 */
bool need_reassemble (uint8_t* frame)
{
    if ((*frame & k_other_frag_type_mask) == k_other_frag_type_mask ||
        (*frame & k_first_frag_type_mask) == k_first_frag_type_mask)
        return true;
    else
        return false;
}

/**
 * @brief initialize reassemble pool
 */
void init_reassembler (void)
{
    memset (&m_reassembler, 0, sizeof m_reassembler);
}

/**
 * @brief read and process a frame
 */
uint8_t read_frame (uint8_t* frame, uint16_t length)
{
    if (is_new_packet (frame) == true)
        // discard current reassemble process
        start_new_reassemble (frame);
    copy_payload (frame, length);
    printf ("filled size: %u\n", m_reassembler.filled_size);
    if (m_reassembler.rx_num_order[m_reassembler.current_frame] == length &&
        m_reassembler.current_frame != m_reassembler.fragment_num)
    {
        m_reassembler.current_frame++;
        // return size of the next frame
        return m_reassembler.rx_num_order[m_reassembler.current_frame];
    }
    else
        // return frame tail size
        return m_reassembler.rx_num_order[m_reassembler.current_frame] - length;
}

/**
 * @brief start a new reassemble process
 */
void start_new_reassemble (uint8_t* frame)
{
    printf ("start new reassemble process\nnew tag: %u\n", get_datagram_tag (frame + 2));
    init_reassembler ();
    m_reassembler.datagram_tag = get_datagram_tag (frame + 2);
    m_reassembler.datagram_size = get_datagram_size (frame);
    calculate_fragment_num ();
    calculate_rx_num_order ();
    m_reassembler.current_frame = 0;
}

/**
 * @brief calculate rx number order
 */
void calculate_rx_num_order (void)
{
    m_reassembler.rx_num_order[0] = FIRST_FRAG_DATA_SIZE +
                                    FIRST_FRAG_HDR_SIZE +
                                    IPHC_TOTAL_SIZE +
                                    UDPHC_TOTAL_SIZE;
    for (uint8_t i = 0; i < m_reassembler.fragment_num - 2; i++)
    {
        m_reassembler.rx_num_order[i+1] = OTHER_FRAG_DATA_SIZE +
                                          OTHER_FRAG_HDR_SIZE;
    }
    m_reassembler.rx_num_order[m_reassembler.fragment_num - 1] =
            (m_reassembler.datagram_size - (FIRST_FRAG_DATA_SIZE)) %
            (OTHER_FRAG_DATA_SIZE) + OTHER_FRAG_HDR_SIZE;
}

/**
 * @brief calculate fragment number based on datagram size
 */
void calculate_fragment_num (void)
{
    m_reassembler.fragment_num = (m_reassembler.datagram_size - (FIRST_FRAG_DATA_SIZE)) /
                                 (OTHER_FRAG_DATA_SIZE) + 2;
}

/**
 * @brief copy frame tail to reassemble buffer
 */
uint8_t copy_frame_tail (uint8_t* frame_tail, uint8_t length)
{
    memcpy (&m_reassembler.reassemble_buffer[m_reassembler.filled_size],
            frame_tail,
            length);
    m_reassembler.filled_size += length;
    m_reassembler.current_frame++;
    return m_reassembler.rx_num_order[m_reassembler.current_frame];
}

/**
 * @brief copy payload to reassemble buffer
 */
void copy_payload (uint8_t* frame, uint16_t length)
{
    if ((*frame & k_other_frag_type_mask) == k_other_frag_type_mask) // other fragment
    {
        memcpy (&m_reassembler.reassemble_buffer[get_datagram_offset (frame + 4)],
                frame + OTHER_FRAG_DATA_OFFSET,
                length - OTHER_FRAG_DATA_OFFSET);
        m_reassembler.filled_size += length - OTHER_FRAG_DATA_OFFSET;
    }
    else if ((*frame & k_first_frag_type_mask) == k_first_frag_type_mask) // first fragment
    {
        memcpy (&m_reassembler.reassemble_buffer[0],
                frame + FIRST_FRAG_DATA_OFFSET,
                length - FIRST_FRAG_DATA_OFFSET);
        m_reassembler.filled_size += length - FIRST_FRAG_DATA_OFFSET;
    }
}

/**
 * @brief check if a fragment belongs to a new packet
 */
bool is_new_packet (uint8_t* frame)
{
    if (get_datagram_tag (frame + 2) != m_reassembler.datagram_tag)
        return true;
    else
        return false;
}

/**
 * @brief check if a packet is completely reassembled
 */
bool is_reassemble_complete (void)
{
    if (m_reassembler.datagram_size == m_reassembler.filled_size)
        return true;
    else
        return false;
}

/**
 * @brief extract a reassembled packet to another buffer
 */
void extract_packet (uint8_t* extract_buffer)
{
    memcpy (extract_buffer,
            m_reassembler.reassemble_buffer,
            m_reassembler.filled_size);
}

/**
 * @brief get reassembler
 */
reassembler_t* get_reassembler (void)
{
    return &m_reassembler;
}
