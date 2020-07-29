#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_sdh.h"

#define SA [[gnu::noinline]] static auto
#define SCA static constexpr auto
#define SI static inline

/*------------------------------------------------------------------------------
    Temperature
    assuming softdevice in use 
    (softdevice takes over temp sensor, so have to use sd)
------------------------------------------------------------------------------*/
template<uint8_t HistSiz_>
struct Temperature {

//============
    private:
//============

    using tempT = int16_t;

    SI tempT tempHistory_[HistSiz_];

SA  addHistory      (tempT v) {
                        static bool isInit;
                        static uint8_t idx;

                        if( not isInit ){ //first time, populate all with same value
                            for( auto& i : tempHistory_ ) i = v;
                            isInit = true;
                            return;
                        }
                        tempHistory_[idx++] = v;
                        if( idx >= HistSiz_ ) idx = 0;
                    }

//===========
    public:
//===========

SA  read            () {
                        tempT f = -999; //-99.9 = failed to get
                        int32_t t;
                        if( sd_temp_get(&t) ) return f;
                        f = (t*10*9/5+320*4)/4; // Fx10
                        if( f < -400 ) f = -400; // -40.0 min
                        if( f > 1800 ) f = 1800; //180.0 max
                        addHistory( f );
                        return f;
                    }

SA  average         () {
                        tempT avg{0};
                        for( auto& i : tempHistory_ ) avg += i;
                        return avg / HistSiz_;
                    }

SA  histSize        () { return HistSiz_; }

};

#undef SA
#undef SCA
#undef SI



