/**
 * author: Xia, Funing
 */

#ifndef USB_CDC_ACM_H_INCLUDED
#define USB_CDC_ACM_H_INCLUDED

#include "nrf.h"
#include "nrf_drv_usbd.h"
#include "nrf_drv_clock.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_drv_power.h"

#include "app_error.h"
#include "app_util.h"
#include "app_usbd_core.h"
#include "app_usbd.h"
#include "app_usbd_string_desc.h"
#include "app_usbd_cdc_acm.h"
#include "app_usbd_serial_num.h"

#include "boards.h"
#include "bsp.h"
#include "app_timer.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "usb_cdc_acm.h"
#include "fsm.h"

#define READ_SIZE 1

void usb_cdc_acm_init(void);
app_usbd_cdc_acm_t const* usb_cdc_acm_inst_get();
char* rx_buffer_get(void);

char m_rx_buffer[READ_SIZE];

#endif /* USB_CDC_ACM_H_INCLUDED */
