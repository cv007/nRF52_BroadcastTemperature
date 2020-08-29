#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_sdh.h"
#include "nrf_delay.h"

#include "Print.hpp"
#include "Tmp117.hpp"
#include "Si7051.hpp"

#define SA [[gnu::noinline]] static auto
#define SCA static constexpr auto
#define SI static inline

/*------------------------------------------------------------------------------
    Temperature
        base class only
------------------------------------------------------------------------------*/
template<uint8_t HistSiz_>
struct Temperature {

//============
    protected:
//============

    Temperature(){} //cannot use this class directly

    SI int16_t tempHistory_[HistSiz_];
    SCA TEMP_MAX{ 180*10 };
    SCA TEMP_MIN{ -40*10 };

SA  addHistory      (int16_t v) {
                        static bool isInit;
                        static uint8_t idx;

                        if( not isInit ){ //first time, populate all with same value
                            for( auto& i : tempHistory_ ) i = v;
                            isInit = true;
                        }
                        if( v < TEMP_MIN ) v = TEMP_MIN;
                        if( v > TEMP_MAX ) v = TEMP_MAX;
                        tempHistory_[idx++] = v;
                        if( idx >= HistSiz_ ) idx = 0;

                        return v; //the min/max limited value
                    }

//===========
    public:
//===========

SA  average         () {
                        int16_t avg{0};
                        for( auto& i : tempHistory_ ) avg += i;
                        return avg / HistSiz_;
                    }

SA  histSize        () { return HistSiz_; }

};

/*------------------------------------------------------------------------------
    Temperature - internal
    assuming softdevice in use 
    (softdevice takes over temp sensor, so have to use sd)
------------------------------------------------------------------------------*/
template<uint8_t HistSiz_>
struct TemperatureInternal : Temperature<HistSiz_> {

SA  read            () {
                        int16_t f = -999; //-99.9 = failed to get
                        int32_t t;
                        if( sd_temp_get(&t) ) return f;
                        f = (t*10*9/5+320*4)/4; // Fx10
                        f = Temperature<HistSiz_>::addHistory( f );
                        DebugFuncHeader();
                        Debug("{Fwhite}  raw: %d\n    F: %02d.%d\n", t, f/10, f%10);
                        return f;
                    }
};

template<uint8_t HistSiz_>
struct TemperatureTmp117 : Temperature<HistSiz_> {

using twi_ = Twim0< board.sda.pinNumber(),   
                    board.scl.pinNumber(), 
                    board.i2cDevicePwr.pinNumber() >;

static inline Tmp117< twi_ > tmp117;

                    // -999 = failed (and is not added to history)
SA  read            () {
                        int16_t f = -999;
                        int16_t t = -32768;
                        //get temp Fx10 into f

                        //init will power on, w/2ms delay time for startup
                        tmp117.init();
                        //default is continuous conversion, 8 samples, 15.5ms*8 = 124ms
                        nrf_delay_ms(130);
                        //poll for data ready (up to a point)
                        auto i = 10; //500us * 10 = 5ms
                        while( not tmp117.isDataReady() and i ){ nrf_delay_us(500); i--; }
                        bool ok = tmp117.tempRaw(t);
                        tmp117.deinit(); //turn off power to ic

                        DebugFuncHeader();
                        if( i == 0 ){ Debug("  timeout, ready bit not set\n"); return f; }
                        if( not ok ){ Debug("  failed to read temp value\n"); return f; }
                        if( t == -32768 ){ Debug("  returned default temp value\n"); return f; }

                        f = tmp117.x10F( t );
                        f = Temperature<HistSiz_>::addHistory( f );
                        Debug("  tmp117 raw: %d  F: %d\n", t, f);
                        return f;
                    }
};

template<uint8_t HistSiz_>
struct TemperatureSi7051 : Temperature<HistSiz_> {

using twi_ = Twim0< board.sda.pinNumber(),   
                    board.scl.pinNumber(), 
                    board.i2cDevicePwr.pinNumber() >;

static inline Si7051< twi_ > si7051;

SA  read            () {
                        int16_t f = -999; //-99.9 = failed to get
                        uint16_t t;

                        //TODO 
                        //get temp Fx10 into f
                        si7051.init();
                        nrf_delay_ms(80); //max power up time
                        bool ok = si7051.tempWait(t); //10ms
                        si7051.deinit();
                        if( not ok ) return f;
                        f = si7051.x10F(t);
                        
                        f = Temperature<HistSiz_>::addHistory( f );
                        DebugFuncHeader();
                        Debug("\traw: %d\n\tF: %02d.%d\n", t, f/10, f%10);
                        return f;
                    }
};

#undef SA
#undef SCA
#undef SI



