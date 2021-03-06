#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "lowpan.h"
#include "config.h"
#include "serial.h"

// variable definitions
static uint8_t m_fragment_num = 0;
static uint8_t m_tail_size = 0;
static uint8_t m_packet_buffer[MAX_MSDU_SIZE];
static uint8_t m_first_frag_size = FIRST_FRAG_DATA_SIZE;
static uint8_t m_other_frag_size = OTHER_FRAG_DATA_SIZE;
static uint16_t m_datagram_tag = 0;
static uint16_t m_current_datagram_offset = 0;

/**
 * @brief check if a packet needs fragmentation
 */
bool need_fragmentation (uint16_t length)
{
    return length > MAX_MSDU_SIZE;
}

/**
 * @brief generate a packet without fragmentation
 */
void generate_normal_packet (virtual_packet_t* tx_packet, uint8_t* payload, uint16_t length)
{
    memset (m_packet_buffer, 0, sizeof m_packet_buffer);
    // copy payload
    memcpy (m_packet_buffer, payload, length);
    // set payload length in first byte of IP header
    *m_packet_buffer = (uint8_t)length;
    // virtual send
    virtual_send (tx_packet, m_packet_buffer, length);
}

/**
 * @brief function for doing fragmentation
 */
void do_fragmentation (virtual_packet_t tx_packet[], uint8_t* payload, uint16_t length)
{
    bool first_fragment = true;
    m_fragment_num = (length - m_first_frag_size) / m_other_frag_size + 2;
    m_tail_size = (length - m_first_frag_size) % m_other_frag_size;
    for (int i = 0; i < m_fragment_num; i++)
    {
        memset (m_packet_buffer, 0, sizeof m_packet_buffer);
        if (first_fragment == true) // first fragment
        {
            // set fragment header
            init_fragment_header ("first", m_packet_buffer, length);
            // copy payload
            memcpy (&m_packet_buffer[FIRST_FRAG_DATA_OFFSET],
                    payload,
                    m_first_frag_size + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE);
            m_current_datagram_offset = m_first_frag_size;
            // virtual send
            virtual_send (&tx_packet[i],
                          m_packet_buffer,
                          FIRST_FRAG_HDR_SIZE + m_first_frag_size);
            first_fragment = false;
        }
        else if (i == m_fragment_num - 1) // last fragment
        {
            // set fragment header
            init_fragment_header ("other", m_packet_buffer, length);
            // copy payload
            memcpy (&m_packet_buffer[OTHER_FRAG_DATA_OFFSET],
                    payload + m_current_datagram_offset,
                    m_tail_size);
            // virtual send
            virtual_send (&tx_packet[i],
                          m_packet_buffer,
                          OTHER_FRAG_HDR_SIZE + m_tail_size);
        }
        else // other fragments
        {
            // set fragment header
            init_fragment_header ("other", m_packet_buffer, length);
            // copy payload
            memcpy (&m_packet_buffer[OTHER_FRAG_DATA_OFFSET],
                    payload + m_current_datagram_offset,
                    m_other_frag_size);
            m_current_datagram_offset = m_first_frag_size + i * m_other_frag_size;
            // virtual send
            virtual_send (&tx_packet[i],
                          m_packet_buffer,
                          OTHER_FRAG_HDR_SIZE + m_other_frag_size);
        }
    }
}

/**
 * @brief initialize a new fragment header
 */
int init_fragment_header (const char* fragment_type, uint8_t* packet_buffer, uint16_t length)
{
    // set fragment type mask and datagram tag
    if (!strcmp (fragment_type, "first")) // first fragment
    {
        *packet_buffer = *packet_buffer | k_first_frag_type_mask;
        set_datagram_tag (fragment_type, packet_buffer + 2);
    }
    else if (!strcmp (fragment_type, "other")) // other fragment
    {
        *packet_buffer = *packet_buffer | k_other_frag_type_mask;
        set_datagram_tag (fragment_type, packet_buffer + 2);
        set_datagram_offset (packet_buffer + 4);
    }
    else // error unknown fragment type
        return -1;
    // set datagram size
    set_datagram_size (packet_buffer, length);
    return 0;
}

/**
 * @brief set datagram offset in fragment header
 */
void set_datagram_offset (uint8_t* offset_offset)
{
    *offset_offset = (uint8_t)(m_current_datagram_offset >> 3);
}

/**
 * @brief get datagram offset in fragment header
 */
uint16_t get_datagram_offset (uint8_t* offset_offset)
{
    uint16_t offset = *offset_offset;
    offset <<= 3;
    return offset;
}

/**
 * @brief set datagram size in fragment header
 */
void set_datagram_size (uint8_t* size_offset, uint16_t datagram_size)
{
    *size_offset = *size_offset | (datagram_size >> 8);             // set high 8 bits
    *(size_offset+1) = *(size_offset+1) | (uint8_t)datagram_size;   // set low 8 bits
}

/**
 * @brief get datagram size
 */
uint16_t get_datagram_size (uint8_t* size_offset)
{
    uint16_t size = 0;
    *((uint8_t*)&size) = *(size_offset + 1);
    if ((*size_offset & k_other_frag_type_mask) == k_other_frag_type_mask) // other fragment
        *((uint8_t*)&size + 1) = (~k_other_frag_type_mask) & (*size_offset);
    else if ((*size_offset & k_first_frag_type_mask) == k_first_frag_type_mask) // first fragment
        *((uint8_t*)&size + 1) = (~k_first_frag_type_mask) & (*size_offset);
    return size;
}

/**
 * @brief set datagram tag in fragment header
 */
void set_datagram_tag (const char* fragment_type, uint8_t* tag_offset)
{
    if (!strcmp (fragment_type, "first")) // first fragment
    {
        *(uint16_t*)tag_offset = (uint16_t)rand();
        m_datagram_tag = *(uint16_t*)tag_offset;
    }
    else if (!strcmp (fragment_type, "other")) // other fragment
        *(uint16_t*)tag_offset = m_datagram_tag;
}

/**
 * @brief get datagram tag
 */
uint16_t get_datagram_tag (uint8_t* tag_offset)
{
    return *(uint16_t*)tag_offset;
}

/**
 * @brief virtually send a packet (only used for testing)
 */
void virtual_send (virtual_packet_t* tx_packet, uint8_t* packet, uint16_t length)
{
    memcpy (tx_packet->packet, packet, length);
    tx_packet->length = length;
    //printf("send a packet\n");
}

/**
 * @brief fill IPv6 header with 'I'
 */
void set_ip_header (uint8_t* iphc_offset)
{
    memset (iphc_offset, 'I', IPHC_TOTAL_SIZE);
}

/**
 * @brief fill UDP header with 'U'
 */
void set_udp_header (uint8_t* udphc_offset)
{
    memset (udphc_offset, 'U', UDPHC_TOTAL_SIZE);
    set_udp_checksum (udphc_offset);
}

/**
 * @brief get UDP header
 */
uint8_t* get_udp_header (uint8_t* packet)
{
    return packet + IPHC_TOTAL_SIZE;
}

/**
 * @brief set UDP checksum
 */
void set_udp_checksum (uint8_t* udphc_offset)
{
   *(uint16_t*)(udphc_offset + 5) = (uint16_t)rand();
}

/**
 * @brief get UDP checksum
 */
uint16_t get_udp_checksum (uint8_t* checksum_offset)
{
   return *(uint16_t*)checksum_offset;
}

/**
 * @brief function for getting fragment number
 */
uint8_t get_fragment_num (void)
{
    return m_fragment_num;
}

/**
 * @brief function for getting tail payload size
 */
uint8_t get_tail_size (void)
{
    return m_tail_size;
}

/**
 * @brief generate an ack packet
 */
uint8_t generate_ack_packet (uint8_t* packet, uint8_t* ack)
{
    uint8_t length = 0;
    set_ip_header (packet);
    set_udp_header (packet + IPHC_TOTAL_SIZE);
    if (*ack == 'c')
    {
        memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                (uint8_t*)CLIENT_ACK,
                sizeof CLIENT_ACK);
        length = IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE + sizeof CLIENT_ACK;
    }
    else if (*ack == 'r')
    {
        memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                (uint8_t*)RELAY_ACK,
                sizeof RELAY_ACK);
        length = IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE + sizeof RELAY_ACK;
    }
    else if (*ack == 's')
    {
        memcpy (packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE,
                (uint8_t*)SERVER_ACK,
                sizeof SERVER_ACK);
        length = IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE + sizeof SERVER_ACK;
    }
    // set packet length in first byte of packet
    *packet = length;
    return length;
}

/**
 * @brief generate an ack packet
 */
bool is_ack_packet (uint8_t* packet)
{
    if (strcmp ((char*)packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE, RELAY_ACK) == 0 ||
        strcmp ((char*)packet + IPHC_TOTAL_SIZE + UDPHC_TOTAL_SIZE, SERVER_ACK) == 0)
        return true;
    else
        return false;
}

/**
 * @brief initialize forwarder
 */
void init_forwarder (lowpan_forwarder_t* forwarder)
{
   memset (forwarder, 0, sizeof *forwarder);
   forwarder->idle = true;
}

/**
 * @brief check if forwarding queue is empty
 */
bool is_forwarder_empty (lowpan_forwarder_t* forwarder)
{
    return forwarder->write_index == forwarder->read_index;
}

/**
 * @brief forward a frame
 */
bool forwarder_send (int fd, lowpan_forwarder_t* forwarder)
{
    int ret = 0;
    ret = write_serial_port (fd,
                             forwarder->queue[forwarder->read_index].packet,
                             forwarder->queue[forwarder->read_index].length);
    forwarder->frame_tries++;
    forwarder->idle = false;
    if (ret < 0)
        return -1;
    else
        return 0;
}
