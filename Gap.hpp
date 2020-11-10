#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include "nrf_sdh_ble.h"
#include "ble_conn_params.h"

#include "Errors.hpp" //error
#include "Print.hpp"
#include "Flash.hpp"

/*------------------------------------------------------------------------------
    Gap
------------------------------------------------------------------------------*/
struct Gap {

SA  init        () {
                    Debug( "Gap::init...\n" );                    

                    ble_gap_conn_params_t   gap_conn_params;
                    ble_gap_conn_sec_mode_t sec_mode;

                    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

                    error.check( 
                        sd_ble_gap_device_name_set( &sec_mode, (const uint8_t*)flash.readName(), strlen( flash.readName() ) )
                     );

                    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

                    gap_conn_params.min_conn_interval = MSEC_TO_UNITS(100, UNIT_1_25_MS);
                    gap_conn_params.max_conn_interval = MSEC_TO_UNITS(200, UNIT_1_25_MS);
                    gap_conn_params.conn_sup_timeout  = MSEC_TO_UNITS(4000, UNIT_10_MS);

                    error.check( sd_ble_gap_ppcp_set(&gap_conn_params) );
                }

};

//for all who include this file
inline Gap gap;