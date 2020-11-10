#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include "nrf_power.h"
#include "nrf_pwr_mgmt.h"

#include "Errors.hpp" //error
#include "Print.hpp"

/*------------------------------------------------------------------------------
    Power
------------------------------------------------------------------------------*/
struct Power {

SA  init        () {
                    Debug( "Power::init...\n" );                    
                    error.check( nrf_pwr_mgmt_init() );
                    //enable REG1 Dc-Dc (instead of LDO, for 1.8v system)
                    nrf_power_dcdcen_set( true );
                }
SA  sleep       () { 
                    DebugFuncHeader();
                    nrf_pwr_mgmt_run(); 
                } 
SA  loop        () {
                    while(true) sleep();
                }

};

//for all who include this file
inline Power power;