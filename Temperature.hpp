#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_sdh.h"
#include "nrf_delay.h"

#include "Print.hpp"
#include "Tmp117.hpp"

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

SA  read            () {
                        DebugFuncHeader();
                        int16_t f = -999; //-99.9 = failed to get
                        int16_t t = -32768;
                        //get temp Fx10 into f
 
                        if( tmp117.oneShot() ){
                            nrf_delay_ms( 125 ); //15.5ms x8
                            auto i = 50; //give time to get a read
                            for( ; i; i-- ){
                                if( tmp117.tempRaw(t) ) break;
                                 nrf_delay_ms(1);
                            }
                            tmp117.deinit();
                            if( i ){
                                f = tmp117.x10F( t );
                                 Debug("  tmp117 raw: %d  F: %d  i: %d\n", t, f, i); 
                            } else Debug("  tmp117 timeout\n");
                            if( t == -32768 ) return f;
                        } else {
                            tmp117.deinit();
                            Debug("  tmp117 failed\n");
                            return f;
                        }
                        
                        f = Temperature<HistSiz_>::addHistory( f );
                        return f;
                    }
};

template<uint8_t HistSiz_>
struct TemperatureSi7051 : Temperature<HistSiz_> {

SA  read            () {
                        int16_t f = -999; //-99.9 = failed to get
                        int32_t t;

                        //get temp Fx10 into f

                        f = Temperature<HistSiz_>::addHistory( f );
                        DebugFuncHeader();
                        Debug("\traw: %d\n\tF: %02d.%d\n", t, f/10, f%10);
                        return f;
                    }
};

#undef SA
#undef SCA
#undef SI



