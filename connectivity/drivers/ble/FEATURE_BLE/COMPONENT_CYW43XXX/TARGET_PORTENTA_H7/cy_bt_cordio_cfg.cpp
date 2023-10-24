/*******************************************************************************
* \file cy_bt_cordio_cfg.cpp
* \version 1.0
*
*
* Low Power Assist BT Pin configuration implementation.
*
********************************************************************************
* \copyright
* Copyright 2019 Cypress Semiconductor Corporation
* SPDX-License-Identifier: Apache-2.0
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#include <stdio.h>
#include "ble/driver/CordioHCIDriver.h"
#include "hci_api.h"
#include "hci_cmd.h"
#include "hci_core.h"
#include "bstream.h"
#include "assert.h"
#include <stdbool.h>
#include "hci_mbed_os_adaptation.h"
#include "CyH4TransportDriver.h"

#define cyhal_gpio_to_rtos(x)   (x)
#define CYCFG_BT_HOST_WAKE_IRQ_EVENT  WAKE_EVENT_ACTIVE_LOW
#define CYCFG_BT_DEV_WAKE_POLARITY    WAKE_EVENT_ACTIVE_LOW

static bool cycfg_bt_lp_enabled = true;

/*******************************************************************************
* Function Name: ble_cordio_get_h4_transport_driver
********************************************************************************
*
* Strong implementation of function which calls CyH4TransportDriver constructor and return it
*
* \param none
*
* \return
* Returns the transport driver object
*******************************************************************************/
ble::vendor::cypress_ble::CyH4TransportDriver& ble_cordio_get_h4_transport_driver()
{
#if (defined(MBED_TICKLESS) && DEVICE_SLEEP && DEVICE_LPTICKER)
   if (cycfg_bt_lp_enabled) {
      static ble::vendor::cypress_ble::CyH4TransportDriver s_transport_driver(
         /* TX */ cyhal_gpio_to_rtos(CYBSP_BT_UART_TX),
         /* RX */ cyhal_gpio_to_rtos(CYBSP_BT_UART_RX),
         /* cts */ cyhal_gpio_to_rtos(CYBSP_BT_UART_CTS),
         /* rts */ cyhal_gpio_to_rtos(CYBSP_BT_UART_RTS),
         /* power */ cyhal_gpio_to_rtos(CYBSP_BT_POWER),
         DEF_BT_BAUD_RATE,
         cyhal_gpio_to_rtos(CYBSP_BT_HOST_WAKE),
         cyhal_gpio_to_rtos(CYBSP_BT_DEVICE_WAKE),
         CYCFG_BT_HOST_WAKE_IRQ_EVENT,
         CYCFG_BT_DEV_WAKE_POLARITY
      );
      return s_transport_driver;
   } else { /* cycfg_bt_lp_enabled */
      static ble::vendor::cypress_ble::CyH4TransportDriver s_transport_driver(
         /* TX */ cyhal_gpio_to_rtos(CYBSP_BT_UART_TX),
         /* RX */ cyhal_gpio_to_rtos(CYBSP_BT_UART_RX),
         /* cts */ cyhal_gpio_to_rtos(CYBSP_BT_UART_CTS),
         /* rts */ cyhal_gpio_to_rtos(CYBSP_BT_UART_RTS),
         /* power */ cyhal_gpio_to_rtos(CYBSP_BT_POWER),
         DEF_BT_BAUD_RATE);
      return s_transport_driver;
   }
#else /* (defined(MBED_TICKLESS) && DEVICE_SLEEP && DEVICE_LPTICKER) */
    static ble::vendor::cypress_ble::CyH4TransportDriver s_transport_driver(
       /* TX */ cyhal_gpio_to_rtos(CYBSP_BT_UART_TX),
       /* RX */ cyhal_gpio_to_rtos(CYBSP_BT_UART_RX),
       /* cts */ cyhal_gpio_to_rtos(CYBSP_BT_UART_CTS),
       /* rts */ cyhal_gpio_to_rtos(CYBSP_BT_UART_RTS),
       /* power */ cyhal_gpio_to_rtos(CYBSP_BT_POWER),
       DEF_BT_BAUD_RATE);
    return s_transport_driver;
#endif /* (defined(MBED_TICKLESS) && DEVICE_SLEEP && DEVICE_LPTICKER) */
}

/*******************************************************************************
* Function Name: ble_cordio_set_cycfg_bt_lp_mode
********************************************************************************
*
* Sets the low-power mode configuration for the Bluetooth stack. 
* By default, low-power mode is enabled.
*
* \param status
* - `true` to enable low-power mode.
* - `false` to disable low-power mode.
*******************************************************************************/
void ble_cordio_set_cycfg_bt_lp_mode(bool status)
{
   cycfg_bt_lp_enabled = status;
}
