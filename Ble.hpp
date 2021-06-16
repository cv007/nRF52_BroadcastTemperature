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
                        DebugRtt << ANSI_NORMAL "header.event_id: " << p_ble_evt->header.evt_id << endl;
                        switch (p_ble_evt->header.evt_id){

                            case BLE_GATTS_EVT_WRITE:
                                DebugRtt << "BLE_GATTS_EVT_WRITE:" << endl;
                                //if write device name, we are interested
                                if( p_ble_evt->evt.gatts_evt.params.write.uuid.uuid == BLE_UUID_GAP_CHARACTERISTIC_DEVICE_NAME ){
                                    uint16_t len = 0;
                                    sd_ble_gap_device_name_get( NULL, &len );
                                    uint8_t buf[len+1];
                                    sd_ble_gap_device_name_get( buf, &len );
                                    buf[len] = 0; //0 terminate string
                                    flash.updateName( (const char*)buf );
                                    DebugRtt << buf << endl;
                                }
                                break;

                            case BLE_GAP_EVT_CONNECTED:
                                DebugRtt << "connected" << endl;
                                adv.timerOff(); //stop the adv update timer
                                adv.isStopped(); //and let adv know it is stopped
                                break;

                            case BLE_GAP_EVT_DISCONNECTED:
                                DebugRtt << "disconnected" << endl;
                                conn.stop(); //no longer need, so stop (?)
                                adv.connectable( false ); //no longer need to be connectable
                                adv.update(); //restart advertising
                                adv.timerOn(); //restart adv update timer
                                break;

                            case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
                                {
                                DebugRtt << "BLE_GAP_EVT_PHY_UPDATE_REQUEST" << endl;
                                ble_gap_phys_t const phys{ BLE_GAP_PHY_AUTO, BLE_GAP_PHY_AUTO, };
                                error.check( sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys) );
                                } 
                                break;

                            default:
                                DebugRtt << FG RED "unhandled event" << ANSI_NORMAL << endl;
                                board.caution(); //red blink
                                break;
                        }
                    }

//===========
    public:
//===========

SA  init            () {
                        DebugRtt << "Ble::init..." << endl;
                        uint32_t ram_start = 0;
                        error.check( nrf_sdh_enable_request() );
                        error.check( nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG_DEFAULT, &ram_start) );
                        error.check( nrf_sdh_ble_enable(&ram_start) );
                        DebugRtt << "    ram start: " << showbase << setfill('0') << setw(8) << ram_start << endl << clear;
                        //_name, _prio, _handler, _context
                        NRF_SDH_BLE_OBSERVER(bleObserver_, 3, eventHandler, NULL);
                    }

};

//for all who include this file
inline Ble ble;


