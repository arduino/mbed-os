/***************************************************************************//**
* \file cybsp_wifi.c
*
* \brief
* Provides utility functions that are used by board support packages.
* 
********************************************************************************
* \copyright
* Copyright 2018-2019 Cypress Semiconductor Corporation
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

#include "cy_network_buffer.h"
#include "cyabs_rtos.h"
#include "whd_types.h"
#include "whd_config.h"
#include "whd_wifi_api.h"
#include "cyhal_sdio.h"
#include "cyhal_gpio.h"

#if defined(__cplusplus)
extern "C" {
#endif

static whd_driver_t whd_drv;

static whd_buffer_funcs_t buffer_ops =
{
    .whd_host_buffer_get = cy_host_buffer_get,
    .whd_buffer_release = cy_buffer_release,
    .whd_buffer_get_current_piece_data_pointer = cy_buffer_get_current_piece_data_pointer,
    .whd_buffer_get_current_piece_size = cy_buffer_get_current_piece_size,
    .whd_buffer_set_size = cy_buffer_set_size,
    .whd_buffer_add_remove_at_front = cy_buffer_add_remove_at_front,
};

static whd_netif_funcs_t netif_ops =
{
    .whd_network_process_ethernet_data = cy_network_process_ethernet_data,
};

//TODO: Need to use resource implemenatation from abstraction layer.
extern whd_resource_source_t resource_ops;

typedef enum
{
  CYBSP_WIFI_WL_REG_ON,
  CYBSP_WIFI_32K_CLK,
  CYBSP_LED1,
  CYBSP_LED2,
  CYBSP_WIFI_SDIO_CMD,
  CYBSP_WIFI_SDIO_CLK,
  CYBSP_WIFI_SDIO_D0,
  CYBSP_WIFI_SDIO_D1,
  CYBSP_WIFI_SDIO_D2,
  CYBSP_WIFI_SDIO_D3,
  CYBSP_SDIO_OOB_IRQ,
  CYBSP_WIFI_MAX,
} wwd_sdio_pin_t;

/* Edit  Pin configuration */
const pinconfig_t PinConfig[]={  

    [CYBSP_WIFI_WL_REG_ON] = WIFI_WL_REG_ON,
#ifdef CYBSP_WIFI_32K_CLK
    [CYBSP_WIFI_32K_CLK]  =  WIFI_32K_CLK,
#endif /* CYBSP_WIFI_32K_CLK */
    [CYBSP_LED1    ]      =  BSP_LED1,
    [CYBSP_LED2    ]      =  BSP_LED2,
    [CYBSP_WIFI_SDIO_CMD] =  WIFI_SDIO_CMD,
    [CYBSP_WIFI_SDIO_CLK] =  WIFI_SDIO_CLK,
    [CYBSP_WIFI_SDIO_D0 ] =  WIFI_SDIO_D0,
    [CYBSP_WIFI_SDIO_D1 ] =  WIFI_SDIO_D1,
    [CYBSP_WIFI_SDIO_D2 ] =  WIFI_SDIO_D2,
    [CYBSP_WIFI_SDIO_D3 ] =  WIFI_SDIO_D3,
    [CYBSP_SDIO_OOB_IRQ ] =  WIFI_SDIO_OOB_IRQ //VIKR
};

whd_sdio_config_t       sdio_config=
{
    /* Bus config */
    .sdio_1bit_mode = false,       /**< Default is false, means SDIO operates under 4 bit mode */
    .high_speed_sdio_clock = false, /**< Default is false, means SDIO operates in normal clock rate */
    .oob_config = {
      /**< Out-of-band interrupt configuration (required when bus can sleep) */
    .host_oob_pin = CYBSP_SDIO_OOB_IRQ,   /**< Host-side GPIO pin selection CYHAL_NC_PIN_VALUE or CYBSP_SDIO_OOB_IRQ */
    .dev_gpio_sel = 0,        /**< WiFi device-side GPIO pin selection (must be zero) */
    .is_falling_edge = true,  /**< Interrupt trigger (polarity) */
    .intr_priority = 0,       /**< OOB interrupt priority */
    } 
};

cy_rslt_t sdio_enumerate( cyhal_sdio_t   *sdhc_obj )
{
    cy_rslt_t result;
    uint32_t     loop_count = 0;
    uint32_t     data = 0;
    do
    {
        /* Send CMD0 to set it to idle state in SDIO_BYTE_MODE, SDIO_1B_BLOCK */
        cyhal_sdio_send_cmd( sdhc_obj, CYHAL_WRITE, CYHAL_SDIO_CMD_GO_IDLE_STATE, 0,NULL );

        /* CMD5. */
        cyhal_sdio_send_cmd( sdhc_obj, CYHAL_READ, CYHAL_SDIO_CMD_IO_SEND_OP_COND, 0,NULL);

        /* Send CMD3 to get RCA. */
        result = cyhal_sdio_send_cmd(sdhc_obj,  CYHAL_READ, CYHAL_SDIO_CMD_SEND_RELATIVE_ADDR,0, &data );
        loop_count++;
        if ( loop_count >= (uint32_t) SDIO_ENUMERATION_TIMEOUT_MS )
        {
            return -1;
        }

    } while ( ( result != CY_RSLT_SUCCESS ) && ( cy_rtos_delay_milliseconds( (uint32_t) 1 ), ( 1 == 1 ) ) );
    /* If you're stuck here, check the platform matches your hardware */

    /* Send CMD7 with the returned RCA to select the card */
    cyhal_sdio_send_cmd(sdhc_obj,  CYHAL_WRITE, CYHAL_SDIO_CMD_SELECT_CARD,data, &data);
    return result;
}

cy_rslt_t whd_init_hardware_sdio(cyhal_sdio_t   *sdhc_obj)
{
   /* WiFi no power */
    cy_rslt_t result = cyhal_gpio_init(CYBSP_WIFI_WL_REG_ON, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_PULLUP,0);
 
 #ifdef CYBSP_WIFI_32K_CLK
   result = cyhal_gpio_init(CYBSP_WIFI_32K_CLK, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP,0);
   cyhal_gpio_write(CYBSP_WIFI_32K_CLK, false);
#endif

    if(result == CY_RSLT_SUCCESS)
    {
        /* Init SDIO Host */
        result = cyhal_sdio_init(sdhc_obj, CYBSP_WIFI_SDIO_CMD, CYBSP_WIFI_SDIO_CLK, CYBSP_WIFI_SDIO_D0, CYBSP_WIFI_SDIO_D1, CYBSP_WIFI_SDIO_D2, CYBSP_WIFI_SDIO_D3);;
        
        if(result == CY_RSLT_SUCCESS)
        {
            /* WiFi put power  */
            cyhal_gpio_write(CYBSP_WIFI_WL_REG_ON, true);
            osDelay(WLAN_POWER_UP_DELAY_MS);
        }
      result=sdio_enumerate(sdhc_obj);

      if(result == CY_RSLT_SUCCESS) sdio_enable_high_speed();

    }
    return result;
}

static cy_rslt_t init_sdio_bus(whd_driver_t* drv, cyhal_sdio_t   *sdhc_obj)
{
    cy_rslt_t res = whd_init_hardware_sdio(sdhc_obj);
    if (res != CY_RSLT_SUCCESS) {
        return res;
    }
    return whd_bus_sdio_attach(*drv, &sdio_config, sdhc_obj);
}

cyhal_sdio_t   sdhc_obj;

cy_rslt_t cybsp_wifi_init_primary(whd_interface_t* interface)
{
    whd_init_config_t whd_init_config;
    whd_init_config.thread_stack_size = (uint32_t)WHD_THREAD_STACK_SIZE;
    whd_init_config.thread_stack_start = NULL;
    whd_init_config.thread_priority =  (uint32_t)WHD_THREAD_PRIORITY;
    whd_init_config.country = WHD_COUNTRY;

    cy_rslt_t result = whd_init(&whd_drv, &whd_init_config, &resource_ops, &buffer_ops, &netif_ops);
    if(result == CY_RSLT_SUCCESS)
    {
        result = init_sdio_bus(&whd_drv, &sdhc_obj);
        if(result == CY_RSLT_SUCCESS)
        {
            result = whd_wifi_on(whd_drv, interface);
        }
    }
    return result;
}

cy_rslt_t cybsp_wifi_init_secondary(whd_interface_t* interface, whd_mac_t* mac_address)
{
    return whd_add_secondary_interface(whd_drv, mac_address, interface);
}

cy_rslt_t cybsp_wifi_deinit(whd_interface_t interface)
{
    cy_rslt_t result = whd_wifi_off(interface);
    if(result == CY_RSLT_SUCCESS)
    {
        result = whd_deinit(interface);
        if(result == CY_RSLT_SUCCESS)
        {
            cyhal_gpio_free(CYBSP_WIFI_WL_REG_ON);
        }
    }
    return result;
}

whd_driver_t cybsp_get_wifi_driver(void)
{
    return whd_drv;
}

#if defined(__cplusplus)
}
#endif