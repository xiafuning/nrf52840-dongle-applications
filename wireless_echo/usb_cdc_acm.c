/**
 * author: Xia, Funing
 */

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

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION true
#endif

/**
 *@brief LED definitions
 */
#define LED_USB_RESUME      (BSP_BOARD_LED_0)	//LED1_G
#define LED_CDC_ACM_OPEN    (BSP_BOARD_LED_1)	//LED2_R
#define LED_CDC_ACM_RX      (BSP_BOARD_LED_2)	//LED2_G
#define LED_CDC_ACM_TX      (BSP_BOARD_LED_3)	//LED2_B

static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event);

#define CDC_ACM_COMM_INTERFACE  0
#define CDC_ACM_COMM_EPIN       NRF_DRV_USBD_EPIN2

#define CDC_ACM_DATA_INTERFACE  1
#define CDC_ACM_DATA_EPIN       NRF_DRV_USBD_EPIN1
#define CDC_ACM_DATA_EPOUT      NRF_DRV_USBD_EPOUT1


/**
 * @brief CDC_ACM class instance definition
 */
APP_USBD_CDC_ACM_GLOBAL_DEF(m_app_cdc_acm,
                            cdc_acm_user_ev_handler,
                            CDC_ACM_COMM_INTERFACE,
                            CDC_ACM_DATA_INTERFACE,
                            CDC_ACM_COMM_EPIN,
                            CDC_ACM_DATA_EPIN,
                            CDC_ACM_DATA_EPOUT,
                            APP_USBD_CDC_COMM_PROTOCOL_NONE);

app_usbd_cdc_acm_t const* usb_cdc_acm_inst_get()
{
	return &m_app_cdc_acm;
}


static char m_tx_buffer[NRF_DRV_USBD_EPSIZE]; // XFN_CHANGE NRF_DRV_USBD_EPSIZE=NRFX_USBD_EPSIZE=64

static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SUSPEND:
            bsp_board_led_off(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_DRV_RESUME:
            bsp_board_led_on(LED_USB_RESUME);
            break;
        case APP_USBD_EVT_STARTED:
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            bsp_board_leds_off();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");

            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}

void init_bsp(void)
{
    /* Configure LEDs */
    bsp_board_init(BSP_INIT_LEDS);
}

/**
 * @brief initialize USB CDC ACM
 */
void usb_cdc_acm_init()
{
	ret_code_t ret;
	static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };

	ret = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(ret);

    ret = app_timer_init();
    APP_ERROR_CHECK(ret);

    init_bsp();

    app_usbd_serial_num_generate();
	ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);
    NRF_LOG_INFO("USBD CDC ACM example started.");
	
	app_usbd_class_inst_t const * class_cdc_acm = app_usbd_cdc_acm_class_inst_get(&m_app_cdc_acm);
    ret = app_usbd_class_append(class_cdc_acm);
    APP_ERROR_CHECK(ret);

    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }
}

/**
 * @brief User event handler @ref app_usbd_cdc_acm_user_ev_handler_t (headphones)
 * */
static void cdc_acm_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_cdc_acm_user_event_t event)
{
    app_usbd_cdc_acm_t const * p_cdc_acm = app_usbd_cdc_acm_class_get(p_inst);

    switch (event)
    {
        case APP_USBD_CDC_ACM_USER_EVT_PORT_OPEN:
        {
            bsp_board_led_on(LED_CDC_ACM_OPEN);

            // Setup first transfer, necessary!
            ret_code_t ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                                   m_rx_buffer,
                                                   READ_SIZE);
            UNUSED_VARIABLE(ret);
			memset (m_rx_buffer, 0, sizeof m_rx_buffer);
            break;
        }
        case APP_USBD_CDC_ACM_USER_EVT_PORT_CLOSE:
            bsp_board_led_off(LED_CDC_ACM_OPEN);
            break;
		case APP_USBD_CDC_ACM_USER_EVT_TX_DONE:
		{
			fsm_event_post(E_USB_CDC_ACM_TX_DONE, NULL);
			bsp_board_led_invert(LED_CDC_ACM_TX);
            break;
		}
       	case APP_USBD_CDC_ACM_USER_EVT_RX_DONE:
        {
			/*
			ret_code_t ret;
            NRF_LOG_INFO("Bytes waiting: %d", app_usbd_cdc_acm_bytes_stored(p_cdc_acm));

			// XFN_CHANGE
			size_t rx_length = 0;
			memset (m_tx_buffer, 0, sizeof m_tx_buffer);
            do
            {
                // Get amount of data transfered
                size_t size = app_usbd_cdc_acm_rx_size(p_cdc_acm);
                NRF_LOG_INFO("RX: size: %lu char: %c", size, m_rx_buffer[0]);

				// XFN_CHANGE
				m_tx_buffer[rx_length] = m_rx_buffer[0];
				rx_length++;

			    // Fetch data until internal buffer is empty
                ret = app_usbd_cdc_acm_read(&m_app_cdc_acm,
                                            m_rx_buffer,
                                            READ_SIZE);
            } while (ret == NRF_SUCCESS);

			// XFN_CHANGE
			app_usbd_cdc_acm_write(&m_app_cdc_acm, m_tx_buffer, rx_length);
			*/
            bsp_board_led_invert(LED_CDC_ACM_RX);
			fsm_event_post(E_USB_CDC_ACM_RX_DONE, NULL);
            break;
        }
        default:
            break;
    }
}
