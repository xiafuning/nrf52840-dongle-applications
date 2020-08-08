#ifndef REASSEMBLE_H
#define REASSEMBLE_H

typedef struct
{
    uint8_t reassemble_buffer[MAX_PACKET_SIZE];
    uint16_t datagram_tag;
    uint16_t datagram_size;
    uint16_t filled_size;
    uint8_t fragment_num;
    uint8_t rx_num_order[MAX_FRAG_NUM];
    uint8_t current_frame;
} reassembler_t;


/**
 * @brief check if a packet needs reassemble
 */
bool need_reassemble (uint8_t* frame);

/**
 * @brief initialize reassemble pool
 */
void init_reassembler (void);

/**
 * @brief read and process a frame
 */
uint8_t read_frame (uint8_t* frame, uint16_t length);

/**
 * @brief start a new reassemble process
 */
void start_new_reassemble (uint8_t* frame);

/**
 * @brief calculate rx number order
 */
void calculate_rx_num_order (void);

/**
 * @brief calculate fragment number based on datagram size
 */
void calculate_fragment_num (void);

/**
 * @brief copy frame tail to reassemble buffer
 */
uint8_t copy_frame_tail (uint8_t* frame_tail, uint8_t length);

/**
 * @brief copy payload to reassemble buffer
 */
void copy_payload (uint8_t* frame, uint16_t length);

/**
 * @brief check if a fragment belongs to a new packet
 */
bool is_new_packet (uint8_t* frame);

/**
 * @brief check if a packet is completely reassembled
 */
bool is_reassemble_complete (void);

/**
 * @brief extract a reassembled packet to another buffer
 */
void extract_packet (uint8_t* extract_buffer);

/**
 * @brief get reassembler
 */
reassembler_t* get_reassembler (void);

#endif /* REASSEMBLE_H */
