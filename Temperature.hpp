#pragma once

#include "nRFconfig.hpp"

#include "nrf_sdh.h"
#include "nrf_delay.h"

#include "Print.hpp"
#include "Tmp117.hpp"
#include "Si7051.hpp"

/*------------------------------------------------------------------------------
    Temperature
    (not static, each use is seperate)
------------------------------------------------------------------------------*/
template<u8 HistSiz_>
struct Temperature {

//============
    private:
//============

    i16 tempHistory_[HistSiz_ == 0 ? 1 :HistSiz_]{0};

    SCA TEMP_MAX{ 180*10 };
    SCA TEMP_MIN{ -40*10 };

//===========
    public:
//===========

auto    addHistory  (i16 v) {
                        static bool isInit;
                        static u8 idx;

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

auto    average     () {
                        i16 avg{0};
                        for( auto& i : tempHistory_ ) avg += i;
                        return avg / HistSiz_;
                    }

};

/*------------------------------------------------------------------------------
    Temperature - internal
    assuming softdevice in use 
    (softdevice takes over temp sensor, so have to use sd)
------------------------------------------------------------------------------*/
template<u8 HistSiz_>
struct TemperatureInternal {

    private:

    inline static Temperature<HistSiz_> tempH;

    public:

SA  average         () { return tempH.average(); }
SA  histSize        () { return HistSiz_; }

SA  read            () {
                        i16 f = -999; //-99.9 = failed to get
                        int32_t t;
                        if( sd_temp_get(&t) ) return f;
                        f = (t*10*9/5+320*4)/4; // Fx10
                        f = tempH.addHistory( f );
                        DebugFuncHeader();
                        Debug("  internal raw: %d  F: %02d.%d\n", t, f/10, f%10);
                        return f;
                    }
};

#ifdef NRF52810_BL651_TEMP
template<u8 HistSiz_>
struct TemperatureTmp117 {

    private:

    inline static Temperature<HistSiz_> tempH;

    using twi_ = Twim0< board.sda.pinNumber(),   
                        board.scl.pinNumber(), 
                        board.i2cDevicePwr.pinNumber() >;

    static inline Tmp117< twi_ > tmp117;

    public:

SA  average         () { return tempH.average(); }
SA  histSize        () { return HistSiz_; }

                    // -999 = failed (and is not added to history)
SA  read            () {
                        i16 f = -999;
                        i16 t = -32768;
                        //get temp Fx10 into f

                        //init will power on, w/2ms delay time for startup
                        tmp117.init();
                        //start conversion, one shot, 1 sample
                        auto i = 10; //10ms
                        // while( i and not tmp117.oneShot1() ){ nrf_delay_ms(1); i--; }
                        // nrf_delay_ms(16); //~15.5ms for 1 conversion 
nrf_delay_ms(125);
                        //poll for data ready (up to 20ms)
                        i = 20; //20ms
                        while( i and not tmp117.isDataReady() ){ nrf_delay_ms(1); i--; }
                        bool ok = i and tmp117.tempRaw(t);
                        tmp117.deinit(); //turn off power to ic

                        DebugFuncHeader();
                        if( not i )      { Debug(FG RED "  timeout, ready bit not set\n" FG WHITE); return f; }
                        if( not ok )     { Debug(FG RED "  failed to read temp value\n" FG WHITE); return f; }
                        if( t == -32768 ){ Debug("  returned default temp value\n"); return f; }

                        f = tmp117.x10F( t );
                        f = tempH.addHistory( f );
                        Debug("  Tmp117 raw: %d  F: %02d.%d\n", t, f/10, f%10);
                        return f;
                    }
};

template<u8 HistSiz_>
struct TemperatureSi7051 {

    private:

    inline static Temperature<HistSiz_> tempH;

    using twi_ = Twim0< board.sda.pinNumber(),   
                        board.scl.pinNumber(), 
                        board.i2cDevicePwr.pinNumber() >;

    static inline Si7051< twi_ > si7051;

    public:

SA  average         () { return tempH.average(); }
SA  histSize        () { return HistSiz_; }

SA  read            () {
                        i16 f = -999; //-99.9 = failed to get
                        u16 t;

                        //get temp Fx10 into f
                        si7051.init();
                        //we get 2ms delay in .init for power up
                        //can take up to 80ms for ic to startup at extreme temp
                        //will just call tempWait until ack'd (true), then is clock 
                        //stretched and will return a temp value in t,
                        //conversion time for 14bits is ~10ms, so use 95ms as max time to wait
                        SCA TIMEOUT_MS{95};
                        auto ok = false;
                        for( auto ms = 0; ms < TIMEOUT_MS; ms++, nrf_delay_ms(1) ){ 
                            if( not si7051.tempWait(t) ) continue;
                            ok = true;
                            break;
                        };
                        si7051.deinit();
                        if( not ok ) return f; //timeout, return f (-999)

                        f = si7051.x10F(t);                        
                        f = tempH.addHistory( f );
                        DebugFuncHeader();
                        Debug("  Si7051 raw: %d  F: %02d.%d\n", t, f/10, f%10);
                        return f;
                    }
};
#endif //#ifdef NRF52810_BL651_TEMP





