#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include "nrf_sdh_ble.h"

#include "Boards.hpp" //board
#include "Errors.hpp" //error
#include "Print.hpp"
#include "Advertising.hpp"
#include "Conn.hpp"

/*------------------------------------------------------------------------------
    Ble
------------------------------------------------------------------------------*/
struct Ble {

    private:

SA  eventHandler    (ble_evt_t const * p_ble_evt, void * p_context) {
                        DebugFuncHeader();
                        Debug( ANSI_NORMAL "header.event_id: %d\n", p_ble_evt->header.evt_id );
                        switch (p_ble_evt->header.evt_id){

                            case BLE_GATTS_EVT_WRITE:
                                Debug( "BLE_GATTS_EVT_WRITE: \n" );
                                //if write device name, we are interested
                                if( p_ble_evt->evt.gatts_evt.params.write.uuid.uuid == BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME ){
                                    uint16_t len = 0;
                                    sd_ble_gap_device_name_get( NULL, &len );
                                    uint8_t buf[len+1];
                                    sd_ble_gap_device_name_get( buf, &len );
                                    buf[len] = 0; //0 terminate string
                                    flash.updateName( (const char*)buf );
                                    Debug("%s\n", buf);
                                }
                                break;

                            case BLE_GAP_EVT_CONNECTED:
                                Debug( "connected\n" );
                                adv.timerOff(); //stop the adv update timer
                                adv.isStopped(); //and let adv know it is stopped
                                break;

                            case BLE_GAP_EVT_DISCONNECTED:
                                Debug( "disconnected\n" );
                                conn.stop(); //no longer need, so stop (?)
                                adv.connectable( false ); //no longer need to be connectable
                                adv.update(); //restart advertising
                                adv.timerOn(); //restart adv update timer
                                break;

                            case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
                                {
                                Debug( "BLE_GAP_EVT_PHY_UPDATE_REQUEST\n" );
                                ble_gap_phys_t const phys{ BLE_GAP_PHY_AUTO, BLE_GAP_PHY_AUTO, };
                                error.check( sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys) );
                                } 
                                break;

                            default:
                                Debug( FG RED "unhandled event\n" ANSI_NORMAL );
                                board.caution(); //red blink
                                break;
                        }
                    }

//===========
    public:
//===========

SA  init            () {
                        Debug( "Ble::init...\n" );
                        uint32_t ram_start = 0;
                        error.check( nrf_sdh_enable_request() );
                        error.check( nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG_DEFAULT, &ram_start) );
                        error.check( nrf_sdh_ble_enable(&ram_start) );
                        Debug( "    ram start: 0x%08X\n", ram_start );
                        //_name, _prio, _handler, _context
                        NRF_SDH_BLE_OBSERVER(bleObserver_, 3, eventHandler, NULL);
                    }

};

//for all who include this file
inline Ble ble;


