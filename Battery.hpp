#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include "nrf_sdh_ble.h"

#include "Print.hpp"
#include "Saadc.hpp"

/*------------------------------------------------------------------------------
    Battery - read battery voltage
------------------------------------------------------------------------------*/
struct Battery {

//============
    private:
//============

    SCA updateInterval_{ 60 }; //no need to read all the time
    //CH0, gain 1/6, ref = 0.6v, 10us (all default values)
    SI SaadcChan vdd_{ SaadcChan::VDD };
    // millivolts  (adc*vref*1000*scale/resolution)
    // adc*3600/1024
    SI i16 voltage_{ 0 }; 

SA  update          () {
                        static u8 count;
                        if( count == 0 ) {
                            vdd_.calibrate();
                            i16 v = 0;
                            vdd_.read(v, vdd_.RES10, vdd_.OVER8X);
                            voltage_ = (i32)v * 3600 / 1024;
                            //make sure we are in some sane range
                            if( voltage_ < 500 ) voltage_ = 0; // <500mv, show 0000
                            if( voltage_ > 3600 ) voltage_ = 9999; //>3600, show 9999
                            DebugRtt << "Battery::update  " << (i16)(voltage_/1000) << '.' 
                                    << setwf(3,'0') << (i16)(voltage_%1000) << "uV" << endlr; 
                        }
                        if( ++count >= updateInterval_ ) count = 0;  
                        return voltage_;
                    }
//============
    public:
//============

SA  read            () { return update(); }

SA  isOk            () { return voltage_ > 2100 ; }

};

//for all who include this file
inline Battery battery;