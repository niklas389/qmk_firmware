/*
Copyright 2023 @ Nuphy <https://nuphy.com/>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "host_driver.h"
#include "rf_driver.h"
#include "host.h"
#include "user_kb.h"
#include "usb_device_state.h"

/* Variable declaration */
extern DEV_INFO_STRUCT dev_info;

/* Host driver */
static uint8_t rf_keyboard_leds(void);
static void    rf_send_keyboard(report_keyboard_t *report);
static void    rf_send_nkro(report_nkro_t *report);
static void    rf_send_mouse(report_mouse_t *report);
static void    rf_send_extra(report_extra_t *report);
#ifdef RAW_ENABLE
static void rf_send_raw_hid(uint8_t *data, uint8_t length);
#endif
host_driver_t rf_host_driver = {
    .keyboard_leds = rf_keyboard_leds,
    .send_keyboard = rf_send_keyboard,
    .send_nkro     = rf_send_nkro,
    .send_mouse    = rf_send_mouse,
    .send_extra    = rf_send_extra,
#ifdef RAW_ENABLE
    .send_raw_hid = rf_send_raw_hid,
#endif
};

/* defined in user_kb.c */
extern host_driver_t *m_host_driver;

/* defined in rf.c */
extern void uart_send_report_keyboard(report_keyboard_t *report);
extern void uart_send_report_nkro(report_nkro_t *report);
extern void uart_send_mouse_report(report_mouse_t *report);
extern void uart_send_consumer_report(report_extra_t *report);
extern void uart_send_system_report(report_extra_t *report);

static uint8_t rf_keyboard_leds(void) {
    return dev_info.rf_led;
}

/**
 * @brief  Force HID report protocol so NKRO reports are used in RF mode.
 */
void rf_force_report_protocol(void) {
    if (usb_device_state_get_protocol() != USB_PROTOCOL_REPORT) {
        usb_device_state_set_protocol(USB_PROTOCOL_REPORT);
    }
}

static void rf_send_keyboard(report_keyboard_t *report) {
    rf_force_report_protocol();
    uart_send_report_keyboard(report);
}

static void rf_send_nkro(report_nkro_t *report) {
    rf_force_report_protocol();
    uart_send_report_nkro(report);
}

static void rf_send_mouse(report_mouse_t *report) {
    uart_send_mouse_report(report);
}

static void rf_send_extra(report_extra_t *report) {
    if (report->report_id == REPORT_ID_CONSUMER) {
        uart_send_consumer_report(report);
    } else if (report->report_id == REPORT_ID_SYSTEM) {
        uart_send_system_report(report);
    }
}

#ifdef RAW_ENABLE
/**
 * @brief  Raw HID (VIA) is USB only; keep it on the USB driver while in RF mode.
 */
static void rf_send_raw_hid(uint8_t *data, uint8_t length) {
    if (m_host_driver && m_host_driver->send_raw_hid) {
        m_host_driver->send_raw_hid(data, length);
    }
}
#endif
