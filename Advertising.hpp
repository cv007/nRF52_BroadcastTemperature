#pragma once

#include <cstdint>
#include <cstdbool>
#include <cstdio>
#include <cstring>

#include "ble_advdata.h"
#include "nrf_nvmc.h"
#include "nrfx_saadc.h"

#include "Boards.hpp"       //board
#include "Temperature.hpp"
#include "Errors.hpp"       //error
#include "Saadc.hpp"

#define SA [[gnu::noinline]] static auto
#define SCA static constexpr auto
#define SC static constexpr
#define SI static inline

/*------------------------------------------------------------------------------
    read battery voltage
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
    SI int16_t voltage_{ 0 }; 

SA  update          () {
                        static uint8_t count;
                        if( count == 0 ) {
                            vdd_.calibrate();
                            int16_t v = 0;
                            vdd_.read(v, vdd_.RES10, vdd_.OVER8X);
                            voltage_ = (int32_t)v * 3600 / 1024;
                            //make sure we are in some sane range
                            if( voltage_ < 500 ) voltage_ = 0; // <500mv, show 0000
                            if( voltage_ > 3600 ) voltage_ = 9999; //>3600, show 9999
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

/*------------------------------------------------------------------------------
    Flags - 0x01 - 0x02, 0x01, flags
------------------------------------------------------------------------------*/
struct Flags01 {

SA  make            (uint8_t* buf, uint8_t flags) {
                        buf[0] = 2;
                        buf[1] = 1;
                        buf[2] = flags;
                        return 3;
                    }

};

/*------------------------------------------------------------------------------
    Complete Local Name - len, 0x09, string
    name is letter A-Z, then current temp
    T 82.5 T-20.5 T-05.2 T101.1
    -> if temp >99.9 then T100.0
    -> if temp 0.0 to 99.9 then T 55.6 or T  3.4
    -> if temp -40.0 to -0.1 then T-10.5 or T -5.2
------------------------------------------------------------------------------*/
struct CompleteName09 {

SA  make            (uint8_t* buf, const char* str, uint8_t maxlen) {
                        uint8_t slen = strlen( str );
                        if( slen > maxlen ) slen = maxlen;
                        buf[0] = slen + 1;
                        buf[1] = 9;
                        memcpy( &buf[2], str, slen );
                        return slen+2;
                    }

};

/*------------------------------------------------------------------------------
    Complete List of 128bit Service Class UUID's - 17, 7, UUID
    make from 16bit values, reverse to make it read left-to-right on phone
------------------------------------------------------------------------------*/
struct UUID07 {

SA  u16toBcd    (uint16_t v) {
                    return (((v/1000)%10)<<12) bitor
                            (((v/100 )%10)<<8 ) bitor
                            (((v/10  )%10)<<4 ) bitor
                            v%10;
                }

SA  make        (uint8_t *buf, const uint16_t (&vals)[8]) {
                    buf[0] = 17;
                    buf[1] = 7;
                    uint8_t* pd = &buf[2];
                    for( auto i = 0; i < 8; i++ ){
                        uint16_t u = vals[i]; //0875 -> 7508
                        *pd++ = u; //75
                        *pd++ = u>>8; //08
                    } 
                    return 18;
                }

                // [0] will not be converted to bcd
SA  makeBCD     (uint8_t *buf, const uint16_t (&vals)[8]) {
                    buf[0] = 17;
                    buf[1] = 7;
                    uint8_t* pd = &buf[2];
                    for( auto i = 0; i < 8; i++ ){
                        uint16_t u = u16toBcd( vals[i] ); //0875 -> 7508
                        if( i == 7 ) u = vals[i]; // 0xde9f no bcd conversion (degF)
                        *pd++ = u; //75
                        *pd++ = u>>8; //08
                    } 
                    return 18;
                }

};


/*------------------------------------------------------------------------------
    MyTemperatureAD - AD data struct(s) to make up payload of adv pdu
    <HistMinutes_> is how many minute to average for a single historical temp
------------------------------------------------------------------------------*/
template<unsigned HistMinutes_ = 5>
struct MyTemperatureAD {

//============
    private:
//============

    SI Temperature<HistMinutes_> temp_;

    using uuidDataT = union {
        uint16_t all[8];
        struct {
            uint16_t tempHist[5];
            uint16_t battery;
            uint16_t count;
            uint16_t id;
        };
    };
    SI uuidDataT uuidData_{0};

    SI char fullnameLetter_; //ram copy, in case want to temporarily change
    //bootloader is at 0xE0000, so use page before (0xE0000-0x1000 = 0xDF000)
    //for flash stored letter - 'A' - 'Z'
    SI volatile char& fullnameLetterFlash_{ *(reinterpret_cast<char*>(LAST_PAGE)) };
    SCA lastPageFlash_{LAST_PAGE};

SA  makeValidLetter (const char c)  { return (c >= 'A' and c <= 'Z') ? c : 'T'; }
SA  uuidCountInc    ()              { if(++uuidData_.count > 9999) uuidData_.count = 0; return uuidData_.count; }
SA  uuidId          (uint16_t v)    { uuidData_.id = v; }
SA  uuidTempShift   ()              { for(int i = 0; i < 4; uuidData_.tempHist[i] = uuidData_.tempHist[i+1], i++ ); }
SA  uuidTempLatest  (uint16_t v)    { uuidData_.tempHist[4] = v; }

                    //will just delay here plenty of time
                    //so can skip checking for events from sd
                    //check if erased and if value was written, and if not
                    //blink the error from error.check
SA  sdFlashWrite    (const char ltr) {
                        error.check( sd_flash_page_erase(lastPageFlash_/4096) );
                        nrf_delay_ms( 1000 );
                        if( fullnameLetterFlash_ != 0xFF ) error.check( NRF_EVT_FLASH_OPERATION_ERROR );
                        uint32_t v = ltr;
                        error.check( sd_flash_write((uint32_t*)lastPageFlash_, &v, 1 ) );
                        nrf_delay_ms( 1000 );
                        if( fullnameLetterFlash_ != ltr ) error.check( NRF_EVT_FLASH_OPERATION_ERROR );
                    }

                    //if sd is enabled (from ble.init), then use the sd version
                    //(cannot use nrf version if sd is enabled)
SA  flashWrite      (const char ltr) {
                        if( nrf_sdh_is_enabled() ) return sdFlashWrite( ltr );
                        nrf_nvmc_page_erase(lastPageFlash_);
                        nrf_nvmc_write_byte(lastPageFlash_, ltr);
                    }

SA  getName         () { return fullnameLetter_; }

SA  setName         (const char c) { fullnameLetter_ = makeValidLetter(c); }

SA  getNameFlash    () { return fullnameLetterFlash_; }

SA  setNameFlash    (const char ltr) {
                        if( ltr != makeValidLetter( ltr ) ) return; //invalid
                        if( ltr != getNameFlash() ){ //not already stored
                            flashWrite( ltr );
                        }
                    }
//===========
    public:
//===========
                    /*  type 0x07, 128bit UUID, 4-2-2-2-6
                        -id-cccc tttt tttt tttt tttttttttttt
                        de9f0001-0872-0877-0878-087908820883 - displayed as little endian
                        830882087908 7808 7708 7208 01009fde - raw

                        id = fixed id
                        cccc = counter (0000-9999, +1 every new temp)
                        tttt = 87.4 = 0874, 100.2 = 1002, 0.0 = 0000, 1.5 = 0015, -1.5 = E015
                        called by timer->adv.update()->here
                    */

SA  update          ( uint8_t (&buf)[31] ) -> void {
                        uuidId( 0xde9f );

                        //every n minutes, shift average temps [1-5] -> [0-4]
                        if( uuidCountInc() % temp_.histSize() == 0 ){
                            uuidTempShift();
                        }

                        //new battery reading
                        uuidData_.battery = Battery::read();

                        //new temp reading
                        int16_t f = temp_.read(); //~50us

                        //put current average in first (latest) position of history
                        int16_t avgF = temp_.average();
                        uuidTempLatest( (avgF >= 0) ? (uint16_t)avgF : 0xE000 bitor (uint16_t)-avgF );  //E=negative 

                        //show current temp in name
                        // T 82.5 T-20.5 T-05.2 T101.1
                        char nambuf[7];
                        //making our own decimal point, so %10 needs to be positive
                        uint8_t f10 = (f < 0) ? -f%10 : f%10;
                        snprintf( nambuf, 7, "%c%3d.%u", getName(), f/10, f10 );

                        uint8_t idx = Flags01::make( buf, BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED ); //3 fixed
                        idx += CompleteName09::make( &buf[idx], nambuf, 8 ); //max 10 (str max 8)
                        idx += UUID07::makeBCD( &buf[idx], uuidData_.all ); //18 fixed
                        buf[idx] = 0;
                    }

                    //set name A-Z

                    // 5 green blinks = ready to press sw1
                    // press sw1 = red blink, letter++ (A-Z, A,B,C,D,...,Z,A,...)
                    // timeout 10 seconds (then 5 green blinks)
SA  init            () {
                        char ltr = 'A'-1;
                        auto inactiveCount = 0;
                        board.ledGreen2.blinkN( 5, 50 );
                        for(; inactiveCount < (10000/50); ){ //10 seconds timeout
                            if( board.sw1.isOff() ){
                                inactiveCount++;
                                nrf_delay_ms(50);
                                continue;
                            }
                            //sw1 pressed
                            board.ledRed1.blinkN( 1, 5 );
                            inactiveCount = 0;
                            if( ++ltr > 'Z' ) ltr = 'A';
                            board.sw1.debounce( 100 );
                        }
                        if( ltr != ('A'-1) ){
                            setNameFlash( ltr );
                            board.ledGreen2.blinkN( 2, 250, 250, 1000 );
                        } else {
                            board.ledGreen2.blinkN( 5, 50, 50, 1000 );
                        }
                        //set ram version from flash
                        setName( getNameFlash() );
                    }

};


/*------------------------------------------------------------------------------
    Advertising
    AdT_ = some struct that takes care of the adv data, which has init/update
    IntervalMS_ = advertising interval in ms
    InitCB_ = function pointer for init() to call if wanted (start a timer)
------------------------------------------------------------------------------*/
template<typename AdT_, uint16_t IntervalMS_, void(*InitCB_)() = nullptr>
struct Advertising {

//============
    private:
//============

    SI AdT_ ADdata_;

    //nRF528xx.hpp will create the SD_TX_LEVELS array for each device
    //(1-14 for S140/52840, 1-9 for S112/52810)
    SI int8_t txPower_{0};

    SI ble_gap_adv_params_t params_;
    SI uint8_t handle_{BLE_GAP_ADV_SET_HANDLE_NOT_SET};
    SI uint8_t buffer_[31];

    // Struct that contains pointers to the encoded advertising data
    SI ble_gap_adv_data_t pdata_{
        .adv_data = { .p_data = buffer_, .len = 31 },
        .scan_rsp_data = { .p_data = NULL, .len = 0 }
    };

    //advertsing interval
    SI auto paramInterval_{IntervalMS_*1.6};// 1600 = 1 sec

    SI bool isActive_{false};

//===========
    public:
//===========

                    //called by timer
SA  update          (void* pcontext = nullptr) -> void {
                        stop();
                        ADdata_.update(buffer_);
                        start();
                    }

SA  init            () {
                        ADdata_.init();
                        params_.properties.type = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
                        params_.interval = paramInterval_;
                        update();
                        if( InitCB_ ) InitCB_();
                    }

                    //1-14 = -40 to +8 dBm, or 1-9 -40 to +4 dBm
SA  power           (uint8_t v) {
                        if( v >= sizeof(SD_TX_LEVELS) ) v = sizeof(SD_TX_LEVELS)-1;
                        error.check( sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, handle_, SD_TX_LEVELS[v] ) );
                    }

SA  start           () -> void {
                        if( isActive_ ) return;
                        //set params if have not started before, else use NULL so will just update data
                        //(have to use this method if advertising is still active, but we are stopped here)
                        // ble_gap_adv_params_t const *pp = (handle_ == BLE_GAP_ADV_SET_HANDLE_NOT_SET) ? &params_ : NULL;
                        // error.check( sd_ble_gap_adv_set_configure(&handle_, &pdata_, pp) );
                       
                        // OR just set parameters also (if we change parameters over time)
                        // which will work because we are stopped
                        error.check( sd_ble_gap_adv_set_configure(&handle_, &pdata_, &params_) );
                        error.check( sd_ble_gap_adv_start(handle_, BLE_CONN_CFG_TAG_DEFAULT) );
                        isActive_ = true;
                        power( txPower_ );
                        if( Battery::isOk() ) board.ok(); else board.caution();

                    }

SA  stop            () -> void {
                        if( not isActive_ ) return;
                        error.check( sd_ble_gap_adv_stop(handle_) );
                        isActive_ = false;
                    }

};

#undef SA
#undef SCA
#undef SI
#undef SC

/*
    for all who include this file
    someone(main) will need to run adv.init()
    5 minutes average per history value
    3000ms advertising interval
    call below timer init function to start a 60s timer to run update
*/
void advInitCB(); //called from adv.init()
inline Advertising< MyTemperatureAD<5>, 3000, advInitCB > adv; 

#include "Timer.hpp"
inline Timer timerAdvUpdate;
inline void advInitCB(){
    timerAdvUpdate.initRepeated( 60_sec, adv.update ); //init timer that calls avd.update
}