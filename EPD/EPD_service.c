/* Copyright (c) 2012 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

#include <string.h>
#include "sdk_macros.h"
#include "ble_srv_common.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "EPD_service.h"
#include "nrf_log.h"

#if defined(S112)
#define EPD_CFG_DEFAULT {0x14, 0x13, 0x06, 0x05, 0x04, 0x03, 0x02, 0x03, 0xFF, 0x12, 0x07}
#else
#define EPD_CFG_DEFAULT {0x05, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x01, 0x07}
#endif

#ifndef EPD_CFG_DEFAULT
#define EPD_CFG_DEFAULT {0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x03, 0x09, 0x03}
#endif

#define BLE_EPD_BASE_UUID                  {{0XEC, 0X5A, 0X67, 0X1C, 0XC1, 0XB6, 0X46, 0XFB, \
                                             0X8D, 0X91, 0X28, 0XD8, 0X22, 0X36, 0X75, 0X62}}
#define BLE_UUID_EPD_CHARACTERISTIC        0x0002

#define ARRAY_SIZE(arr)                    (sizeof(arr) / sizeof((arr)[0]))
#define EPD_CONFIG_SIZE                    (sizeof(epd_config_t) / sizeof(uint8_t))

/**@brief Function for handling the @ref BLE_GAP_EVT_CONNECTED event from the S110 SoftDevice.
 *
 * @param[in] p_epd     EPD Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_connect(ble_epd_t * p_epd, ble_evt_t * p_ble_evt)
{
    p_epd->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}


/**@brief Function for handling the @ref BLE_GAP_EVT_DISCONNECTED event from the S110 SoftDevice.
 *
 * @param[in] p_epd     EPD Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_disconnect(ble_epd_t * p_epd, ble_evt_t * p_ble_evt)
{
    UNUSED_PARAMETER(p_ble_evt);
    p_epd->conn_handle = BLE_CONN_HANDLE_INVALID;
}

static void epd_service_process(ble_epd_t * p_epd, uint8_t * p_data, uint16_t length)
{
    if (p_data == NULL || length <= 0) return;
    NRF_LOG_DEBUG("[EPD]: CMD=0x%02x, LEN=%d\n", p_data[0], length);

    if (p_epd->epd_cmd_cb != NULL) {
        if (p_epd->epd_cmd_cb(p_data[0], length > 1 ? &p_data[1] : NULL, length - 1))
            return;
    }

    switch (p_data[0])
    {
      case EPD_CMD_SET_PINS:
          if (length < 8) return;

          DEV_Module_Exit();

          EPD_MOSI_PIN = p_epd->config.mosi_pin = p_data[1];
          EPD_SCLK_PIN = p_epd->config.sclk_pin = p_data[2];
          EPD_CS_PIN = p_epd->config.cs_pin = p_data[3];
          EPD_DC_PIN = p_epd->config.dc_pin = p_data[4];
          EPD_RST_PIN = p_epd->config.rst_pin = p_data[5];
          EPD_BUSY_PIN = p_epd->config.busy_pin = p_data[6];
          EPD_BS_PIN = p_epd->config.bs_pin = p_data[7];
          if (length > 8)
            EPD_EN_PIN = p_epd->config.en_pin = p_data[8];
          epd_config_save(&p_epd->config);

          DEV_Module_Init();
          break;

      case EPD_CMD_INIT:
          if (length > 1)
          {
              if (epd_driver_set(p_data[1]))
              {
                  p_epd->driver = epd_driver_get();
                  p_epd->config.driver_id = p_epd->driver->id;
                  epd_config_save(&p_epd->config);
              }
          }

          NRF_LOG_INFO("[EPD]: DRIVER=%d\n", p_epd->driver->id);
          p_epd->driver->init();
          break;

      case EPD_CMD_CLEAR:
          p_epd->driver->clear();
          break;

      case EPD_CMD_SEND_COMMAND:
          if (length < 2) return;
          p_epd->driver->send_command(p_data[1]);
          break;

      case EPD_CMD_SEND_DATA:
          p_epd->driver->send_data(&p_data[1], length - 1);
          break;

      case EPD_CMD_DISPLAY:
          p_epd->driver->refresh();
          break;

      case EPD_CMD_SLEEP:
          p_epd->driver->sleep();
          break;

      case EPD_CMD_SET_CONFIG:
          if (length < 2) return;
          memcpy(&p_epd->config, &p_data[1], (length - 1 > EPD_CONFIG_SIZE) ? EPD_CONFIG_SIZE : length - 1);
          epd_config_save(&p_epd->config);
          break;
      
      case EPD_CMD_CFG_ERASE:
          epd_config_clear(&p_epd->config);
          nrf_delay_ms(100); // required
          NVIC_SystemReset();
          break;

      default:
        break;
    }
}

/**@brief Function for handling the @ref BLE_GATTS_EVT_WRITE event from the S110 SoftDevice.
 *
 * @param[in] p_epd     EPD Service structure.
 * @param[in] p_ble_evt Pointer to the event received from BLE stack.
 */
static void on_write(ble_epd_t * p_epd, ble_evt_t * p_ble_evt)
{
    ble_gatts_evt_write_t * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (
        (p_evt_write->handle == p_epd->char_handles.cccd_handle)
        &&
        (p_evt_write->len == 2)
       )
    {
        if (ble_srv_is_notification_enabled(p_evt_write->data))
        {
            NRF_LOG_DEBUG("notification enabled\n");
            p_epd->is_notification_enabled = true;
            static uint16_t length = sizeof(epd_config_t);
            NRF_LOG_DEBUG("send epd config\n");
            uint32_t err_code = ble_epd_string_send(p_epd, (uint8_t *)&p_epd->config, length);
            if (err_code != NRF_ERROR_INVALID_STATE)
                APP_ERROR_CHECK(err_code);
        }
        else
        {
            p_epd->is_notification_enabled = false;
        }
    }
    else if (p_evt_write->handle == p_epd->char_handles.value_handle)
    {
        epd_service_process(p_epd, p_evt_write->data, p_evt_write->len);
    }
    else
    {
        // Do Nothing. This event is not relevant for this service.
    }
}

#if defined(S112)
void ble_epd_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    if (p_context == NULL || p_ble_evt == NULL) return;
    
    ble_epd_t *p_epd = (ble_epd_t *)p_context;
    ble_epd_on_ble_evt(p_epd, (ble_evt_t *)p_ble_evt);
}
#endif

void ble_epd_on_ble_evt(ble_epd_t * p_epd, ble_evt_t * p_ble_evt)
{
    if ((p_epd == NULL) || (p_ble_evt == NULL))
    {
        return;
    }

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_epd, p_ble_evt);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_epd, p_ble_evt);
            break;

        case BLE_GATTS_EVT_WRITE:
            on_write(p_epd, p_ble_evt);
            break;

        default:
            // No implementation needed.
            break;
    }
}


static uint32_t epd_service_init(ble_epd_t * p_epd)
{
    ble_uuid_t            ble_uuid;
    ble_uuid128_t         base_uuid = BLE_EPD_BASE_UUID;
    ble_add_char_params_t add_char_params;
 
    VERIFY_SUCCESS(sd_ble_uuid_vs_add(&base_uuid, &p_epd->uuid_type));

    ble_uuid.type = p_epd->uuid_type;
    ble_uuid.uuid = BLE_UUID_EPD_SERVICE;
    VERIFY_SUCCESS(sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                            &ble_uuid,
                                            &p_epd->service_handle));

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                     = BLE_UUID_EPD_CHARACTERISTIC;
    add_char_params.uuid_type                = p_epd->uuid_type;
    add_char_params.max_len                  = BLE_EPD_MAX_DATA_LEN;
    add_char_params.init_len                 = sizeof(uint8_t);
    add_char_params.is_var_len               = true;
    add_char_params.char_props.notify        = 1;
    add_char_params.char_props.write         = 1;
    add_char_params.char_props.write_wo_resp = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;
    add_char_params.cccd_write_access = SEC_OPEN;

    return characteristic_add(p_epd->service_handle, &add_char_params, &p_epd->char_handles);
}

static void ble_epd_config_load(ble_epd_t * p_epd)
{
    bool is_empty_config = true;

    for (uint8_t i = 0; i < EPD_CONFIG_SIZE; i++)
    {
        if (((uint8_t *)&p_epd->config)[i] != 0xFF)
        {
            is_empty_config = false;
        }
    }
    NRF_LOG_DEBUG("is_empty_config: %d\n", is_empty_config);
    // write default config
    if (is_empty_config)
    {
        uint8_t cfg[] = EPD_CFG_DEFAULT;
        memcpy(&p_epd->config, cfg, ARRAY_SIZE(cfg));
        epd_config_save(&p_epd->config);
    }

    // load config
    EPD_MOSI_PIN = p_epd->config.mosi_pin;
    EPD_SCLK_PIN = p_epd->config.sclk_pin;
    EPD_CS_PIN = p_epd->config.cs_pin;
    EPD_DC_PIN = p_epd->config.dc_pin;
    EPD_RST_PIN = p_epd->config.rst_pin;
    EPD_BUSY_PIN = p_epd->config.busy_pin;
    EPD_BS_PIN = p_epd->config.bs_pin;
    EPD_EN_PIN = p_epd->config.en_pin;
    EPD_LED_PIN = p_epd->config.led_pin;

    epd_driver_set(p_epd->config.driver_id);
	p_epd->driver = epd_driver_get();

    // blink LED on start
    if (EPD_LED_PIN != 0xFF)
    {
        pinMode(EPD_LED_PIN, OUTPUT);
        EPD_LED_ON();
        delay(100);
        EPD_LED_OFF();
    }
}

void ble_epd_sleep_prepare(ble_epd_t * p_epd)
{
    // Turn off led
    EPD_LED_OFF();
    // Prepare wakeup pin
    if (p_epd->config.wakeup_pin != 0xFF)
    {
        nrf_gpio_cfg_sense_input(p_epd->config.wakeup_pin, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_SENSE_HIGH);
    }
}

uint32_t ble_epd_init(ble_epd_t * p_epd, epd_callback_t cmd_cb)
{
    if (p_epd == NULL) return NRF_ERROR_NULL;
    p_epd->epd_cmd_cb = cmd_cb;

    // Initialize the service structure.
    p_epd->max_data_len = BLE_EPD_MAX_DATA_LEN;
    p_epd->conn_handle             = BLE_CONN_HANDLE_INVALID;
    p_epd->is_notification_enabled = false;

    epd_config_init(&p_epd->config);
    epd_config_load(&p_epd->config);
    ble_epd_config_load(p_epd);

    // Add the service.
    return epd_service_init(p_epd);
}

uint32_t ble_epd_string_send(ble_epd_t * p_epd, uint8_t * p_string, uint16_t length)
{
    ble_gatts_hvx_params_t hvx_params;

    if (p_epd == NULL)
    {
        return NRF_ERROR_NULL;
    }

    if ((p_epd->conn_handle == BLE_CONN_HANDLE_INVALID) || (!p_epd->is_notification_enabled))
    {
        return NRF_ERROR_INVALID_STATE;
    }

    if (length > p_epd->max_data_len)
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_epd->char_handles.value_handle;
    hvx_params.p_data = p_string;
    hvx_params.p_len  = &length;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;

    return sd_ble_gatts_hvx(p_epd->conn_handle, &hvx_params);
}
