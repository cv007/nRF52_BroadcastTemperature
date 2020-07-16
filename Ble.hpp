#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include <cstdbool>
#include <cstdint>

#include "nrf_sdh_ble.h"

#include "Boards.hpp" //board
#include "Errors.hpp" //error

/*-----------------------------------------------------------------------------
    misc defines
-----------------------------------------------------------------------------*/
#define SA static auto
#define SCA static constexpr auto
#define SI static inline

/*------------------------------------------------------------------------------
    Ble
------------------------------------------------------------------------------*/
struct Ble {

    private:
                    //unused, but if something shows up blink the red led
SA  eventHandler    (ble_evt_t const * p_ble_evt, void * p_context) {
                        switch (p_ble_evt->header.evt_id){
                            default:
                                board.caution(); //red blink
                                break;
                        }
                    }

//===========
    public:
//===========

SA  init            () {
                        uint32_t ram_start = 0;
                        error.check( nrf_sdh_enable_request() );
                        error.check( nrf_sdh_ble_default_cfg_set(BLE_CONN_CFG_TAG_DEFAULT, &ram_start) );
                        error.check( nrf_sdh_ble_enable(&ram_start) );
                        //_name, _prio, _handler, _context
                        NRF_SDH_BLE_OBSERVER(bleObserver_, 3, eventHandler, NULL);
                    }

};

#undef SA
#undef SCA
#undef SI


//for all who include this file
inline Ble ble;