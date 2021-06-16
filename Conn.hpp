#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include "ble_conn_params.h"

#include "Errors.hpp" //error
#include "Print.hpp"

#define SI static inline

/*------------------------------------------------------------------------------
    Conn
------------------------------------------------------------------------------*/
struct Conn {

SA  init        () {
                    DebugRtt << "Conn::init..." << endl;                  
                    ble_conn_params_init_t cp_init;

                    memset(&cp_init, 0, sizeof(cp_init));

                    cp_init.first_conn_params_update_delay = APP_TIMER_TICKS(20000);
                    cp_init.next_conn_params_update_delay  = APP_TIMER_TICKS(5000);
                    cp_init.max_conn_params_update_count   = 3;
                    cp_init.disconnect_on_fail             = true;

                    error.check( ble_conn_params_init(&cp_init) );
                }

SA  stop        () { error.check( ble_conn_params_stop() ); }

};


#undef SI

//for all who include this file
inline Conn conn;