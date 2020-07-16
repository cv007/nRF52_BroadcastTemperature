#pragma once

#include <cstdint>
#include <cstdbool>

#include "app_timer.h"

#include "Errors.hpp"   //error

#define SA static auto
#define SCA static constexpr auto
#define SI static inline

/*------------------------------------------------------------------------------
    user defined literals use for ms, sec
------------------------------------------------------------------------------*/
#define CUO constexpr uint32_t operator ""

CUO     _ms     (unsigned long long int ms){ return ms; }
CUO     _sec    (unsigned long long int sec){ return sec * 1000_ms; }

#undef CUO

/*------------------------------------------------------------------------------
    Timer
------------------------------------------------------------------------------*/
struct Timer {

//============
    private:
//============

    //for each instance
    app_timer_t timerIdData_;
    const app_timer_id_t ptimerId_{&timerIdData_};

    //for all instances
    SI bool isTimerModuleInit_{false};
    SCA ONEDAY_MS{ 1000*60*60*24 }; //maximum ms for app timer
    SCA RTC_HZ{ 32768 / (APP_TIMER_CONFIG_RTC_FREQUENCY+1) }; // from sdk_config.h, 0=div1, 1=div2, etc.

                    //ms to timer ticks - timer uses rtc
SCA appTimerTicks   (uint32_t ms){
                        if( ms > ONEDAY_MS ) ms = ONEDAY_MS;
                        return __builtin_ceil( ms*(RTC_HZ/1000.0) );
                    }

                    //init timer module on first use, applies to all instances
SA init             () {
                        if( isTimerModuleInit_ ) return;
                        app_timer_init(); //always success
                        isTimerModuleInit_ = true;
                    }

//============
    public:
//============
                    
                    //for each instance

auto initSingle     ( uint32_t ms, void(*cb)(void*) ) {
                        init();
                        error.check( app_timer_create(&ptimerId_, APP_TIMER_MODE_SINGLE_SHOT, cb) );
                        error.check( app_timer_start(ptimerId_, appTimerTicks(ms), NULL) );
                    }
auto initRepeated   ( uint32_t ms, void(*cb)(void*) ) {
                        init();
                        error.check( app_timer_create(&ptimerId_, APP_TIMER_MODE_REPEATED, cb) );
                        error.check( app_timer_start(ptimerId_, appTimerTicks(ms), NULL) );
                    }

};

#undef SA
#undef SCA
#undef SI
