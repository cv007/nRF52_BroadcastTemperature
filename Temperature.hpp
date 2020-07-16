#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_sdh.h"

#define SA [[gnu::noinline]] static auto
#define SCA static constexpr auto
#define SI static inline
#define I16 int16_t

/*------------------------------------------------------------------------------
    Temperature
------------------------------------------------------------------------------*/
template<uint8_t HistSiz_>
struct Temperature {

//============
    private:
//============

    SI I16 tempHistory_[HistSiz_];

SA  addHistory      (I16 v) {
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
                        I16 f = -999; //-99.9 = failed to get
                        int32_t t;
                        if( sd_temp_get(&t) ) return f;
                        f = (t*10*9/5+320*4)/4; // Fx10
                        if( f < -400 ) f = -400; // -40.0 min
                        if( f > 1800 ) f = 1800; //180.0 max
                        addHistory( f );
                        return f;
                    }

SA  average         () {
                        I16 avg{0};
                        for( auto& i : tempHistory_ ) avg += i;
                        return avg / HistSiz_;
                    }

SA  histSize        () { return HistSiz_; }

};

#undef SA
#undef SCA
#undef SI
#undef I16



