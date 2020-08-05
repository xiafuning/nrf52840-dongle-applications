#ifndef LOWPAN_H
#define LOWPAN_H


#define FIRST_FRAG_HDR_SIZE     4       // First fragment header size in octets.
#define OTHER_FRAG_HDR_SIZE     5       // Subsequent fragment header size in octets.
#define MAX_PACKET_SIZE         1500
#define MAX_MSDU_SIZE           100
#define IPHC_TOTAL_SIZE         3       // LOWPAN_IPHC(2 bytes) + DST_ADDR(1 byte)
#define UDPHC_TOTAL_SIZE        7
#define FIRST_FRAG_DATA_SIZE    (MAX_MSDU_SIZE- \
                                IPHC_TOTAL_SIZE- \
                                UDPHC_TOTAL_SIZE- \
                                FIRST_FRAG_HDR_SIZE) / 8 * 8 - 8
#define OTHER_FRAG_DATA_SIZE    (MAX_MSDU_SIZE- \
                                OTHER_FRAG_HDR_SIZE) / 8 * 8
#define MAX_FRAG_NUM            ((MAX_PACKET_SIZE-(FIRST_FRAG_DATA_SIZE))/ \
                                (OTHER_FRAG_DATA_SIZE) + 2)
#define FIRST_FRAG_DATA_OFFSET  (FIRST_FRAG_HDR_SIZE+ \
                                IPHC_TOTAL_SIZE+ \
                                UDPHC_TOTAL_SIZE)
#define OTHER_FRAG_DATA_OFFSET  OTHER_FRAG_HDR_SIZE


typedef struct
{
    uint8_t packet[MAX_MSDU_SIZE];
    uint16_t length;
} virtual_packet_t;

enum
{
    k_first_frag_type_mask = 0xc0,  // 0b1100_0000
    k_other_frag_type_mask = 0xe0,  // 0b1110_0000
};

/**
 * @brief check if a packet needs fragmentation
 */
bool need_fragmentation (uint16_t length);

/**
 * @brief generate a packet without fragmentation
 */
void generate_normal_packet (virtual_packet_t* tx_packet, uint8_t* payload, uint16_t length);

/**
 * @brief function for doing fragmentation
 */
void do_fragmentation (virtual_packet_t tx_packet[], uint8_t* payload, uint16_t length);

/**
 * @brief initialize a new fragment header
 */
int init_fragment_header (uint8_t* fragment_type, uint8_t* packet_buffer, uint16_t length);

/**
 * @brief set datagram offset in fragment header
 */
void set_datagram_offset (uint8_t* offset_offset);

/**
 * @brief get datagram offset in fragment header
 */
uint16_t get_datagram_offset (uint8_t* offset_offset);

/**
 * @brief set datagram size in fragment header
 */
void set_datagram_size (uint8_t* size_offset, uint16_t datagram_size);

/**
 * @brief get datagram size
 */
uint16_t get_datagram_size (uint8_t* size_offset);

/**
 * @brief set datagram tag in fragment header
 */
void set_datagram_tag (uint8_t* fragment_type, uint8_t* size_offset);

/**
 * @brief get datagram tag
 */
uint16_t get_datagram_tag (uint8_t* size_offset);

/**
 * @brief virtually send a packet (only used for testing)
 */
void virtual_send (virtual_packet_t* tx_packet, uint8_t* packet, uint16_t length);

/**
 * @brief fill IPv6 header with 'I'
 */
void set_ip_header (uint8_t* iphc_offset);

/**
 * @brief fill UDP header with 'U'
 */
void set_udp_header (uint8_t* udphc_offset);

/**
 * @brief function for getting fragment number
 */
uint8_t get_fragment_num (void);

/**
 * @brief function for getting tail payload size
 */
uint8_t get_tail_size (void);

#endif /* LOWPAN_H */
