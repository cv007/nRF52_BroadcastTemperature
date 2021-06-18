#pragma once

#include "nRFconfig.hpp"

#include <cstdio>
#include <cstring>

#include "ble_advdata.h"
#include "nrf_nvmc.h"
#include "nrfx_saadc.h"

#include "Boards.hpp"       //board
#include "Temperature.hpp"
#include "Errors.hpp"       //error
// #include "Saadc.hpp"
#include "Print.hpp"
#include "Timer.hpp"
#include "Battery.hpp"
#include "Flash.hpp"

#undef SA
#define SA [[gnu::noinline]] static auto
#define SC static constexpr

/*------------------------------------------------------------------------------
    Flags - len, 0x01, flags
    size [3]
------------------------------------------------------------------------------*/
struct Flags01 {

SA  make            (u8* buf, u8 flags) {
                        buf[0] = 2;
                        buf[1] = 1;
                        buf[2] = flags;
                        return 3;
                    }

};

/*------------------------------------------------------------------------------
    Complete Local Name - len, 0x09, string
    size [2+strlen]

    if using flags,battery service
    flags=3, battery=4, 31-7-2= 22 chars max for name 
------------------------------------------------------------------------------*/
struct CompleteName09 {

SA  make            (u8* buf, const char* str, u8 maxlen) {
                        u8 slen = strlen( str );
                        if( slen > maxlen ) slen = maxlen;
                        buf[0] = slen + 1;
                        buf[1] = 9;
                        memcpy( &buf[2], str, slen );
                        return slen+2;
                    }

};

/*------------------------------------------------------------------------------
    Appearance - len, 0x19, value (2bytes, little endian)
    size [4]
------------------------------------------------------------------------------*/
struct Appearance19 {

SA  make            (u8* buf, u16 v) {
                        buf[0] = 3;
                        buf[1] = 0x19;
                        buf[2] = v;
                        buf[3] = v<<8;
                        return 4;
                    }

};

/*------------------------------------------------------------------------------
    Service Data - len, 0x16, 16bit UUID, data
    size [4+datlen]
------------------------------------------------------------------------------*/
struct ServiceData16 {

SA  make            (u8* buf, u16 uuid, u8* dat, u8 datlen) {
                        buf[0] = 3 + datlen;
                        buf[1] = 0x16;
                        buf[2] = uuid;
                        buf[3] = uuid>>8;
                        for( auto i = 4; i < datlen+4; i++ ) buf[i] = *dat++;
                        return buf[0] + 1;
                    }

};

/*------------------------------------------------------------------------------
    Battery Service - data 1 byte 0-100%
        uses ServiceData16
    size [4]
------------------------------------------------------------------------------*/
struct BatteryService180F {

SA  make            (u8* buf) {
                        //2.00v = 0%, 3.00v = 100%
                        //percentage will be mV/10 from 2-3v (77% = 2.77v)
                        u16 bv = battery.read();
                        DebugFuncHeader();
                        DebugRtt << "  battery: " << bv << "mV" << endl;
                        u8 dat = bv > 3000 ? 100 :
                                 bv < 2000 ? 0 :
                                 (bv - 2000)/10;
                        return ServiceData16::make( buf, 0x180F, &dat, 1 );
                    }

};


/*------------------------------------------------------------------------------
    MyTemperatureAD - AD data struct(s) to make up payload of adv pdu
------------------------------------------------------------------------------*/
template<typename TempDriver_>
struct MyTemperatureAD {

//============
    private:
//============

    SI TempDriver_ temp_;

//===========
    public:
//===========

SA  update          ( u8 (&buf)[31] ) -> void {
                        // new temp reading
                        i16 f = temp_.read(); //~50us
                        //making our own decimal point, so %10 needs to be positive
                        u8 f10 = (f < 0) ? -f%10 : f%10;
                        f = f/10;
                        BufoStreamer<22+1> nambuf; //add 1 for 0 terminator
                        nambuf << f << "." << f10 << "F " << flash.readName();
                        u8 idx = Flags01::make( buf, BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED ); //3
                        idx += BatteryService180F::make( &buf[idx] ); //4
                        idx += CompleteName09::make( &buf[idx], nambuf.buf(), 31-7-2 ); // up to 22 chars
                        if( idx < 31 ) buf[idx] = 0;
                    }

};


/*------------------------------------------------------------------------------
    Advertising
    AdT_ = some struct that takes care of the adv data, which has init/update
    IntervalMS_ = advertising interval in ms
    InitCB_ = function pointer for init() to call if wanted (start a timer)
------------------------------------------------------------------------------*/
template<typename AdT_, u16 IntervalMS_, u32 UpdateInterval_>
struct Advertising {

//============
    private:
//============

    SI AdT_ ADdata_;

    //nRF528xx.hpp will create the SD_TX_LEVELS array for each device
    //(1-14 for S140/52840, 1-9 for S112/52810)
    SI i8 txPower_{0};

    SI ble_gap_adv_params_t params_;
    SI u8 handle_{BLE_GAP_ADV_SET_HANDLE_NOT_SET};
    SI u8 buffer_[31];

    // Struct that contains pointers to the encoded advertising data
    SI ble_gap_adv_data_t pdata_{
        .adv_data = { .p_data = buffer_, .len = 31 },
        .scan_rsp_data = { .p_data = NULL, .len = 0 }
    };

    //advertsing interval
    SI auto paramInterval_{IntervalMS_*1.6};// 1600 = 1 sec

    SI bool isActive_{false};
    SI bool isConnectable_{true}; //start out connectable so can change name
    SI u8   connectableTimeout_{20}; //disable connectable after some number of updates

    //update data interval
    SI Timer timerAdvUpdate_;
    SI u32   timerInterval_{UpdateInterval_};

//===========
    public:
//===========

                    //call from  ble connected event handler to update the state
                    //of advertising since the soft device stopped it
SA  isStopped       () { 
                        isActive_ = false; 
                    }

                    //turn on/off connectable, so can make connectable intially 
                    //to change name, turn off when no longer wanted
SA  connectable     (bool tf) { 
                        isConnectable_ = tf; 
                    }

                    //called by timer
SA  update          (void* pcontext = nullptr) -> void {
                        stop();
                        ADdata_.update(buffer_);

                        //=== Debug ===
                        DebugFuncHeader();
                        DebugRtt << FG CYAN "  -advertising packet-" << endl << FG WHITE;
                        auto i = 0;
                        while( buffer_[i] ){
                            u32 len = buffer_[i++];
                            auto typ = buffer_[i++];
                            DebugRtt
                                << clear
                                << "  len: " << setwf(2,' ') << len-- 
                                << "  type: " << Hex << setwf(2,'0') << typ
                                << "  data: ";
                            //name
                            if( typ == 9 ){ 
                                DebugRtt << setwmax(len) << (char*)&buffer_[i] << ' ' << setwmax(0);
                                }
                            else {
                                for( u32 j = 0; j < len; j++ ){ 
                                    DebugRtt << setwf(2,'0') << Hex << buffer_[i+j] << ' ';
                                    }
                            }
                            i += len;
                            DebugRtt << endl;
                        }
                        DebugRtt << endlc;
                        //=== Debug ===

                        start();
                    }

SA  timerOn         () {
                        timerAdvUpdate_.init( timerInterval_, update, timerAdvUpdate_.REPEATED );
                    }

SA  timerOff        () {
                        timerAdvUpdate_.stop();
                    }
SA  timerInterval   (u32 ms) {
                        timerInterval_ = ms;
                    }

SA  init            () {
                        DebugRtt << "Advertising::init..." << endl;
                        params_.interval = paramInterval_;
                        update();
                        timerOn();
                    }

                    //1-14 = -40 to +8 dBm, or 1-9 -40 to +4 dBm
SA  power           (u8 v) {
                        if( v >= sizeof(SD_TX_LEVELS) ) v = sizeof(SD_TX_LEVELS)-1;
                        error.check( sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, handle_, SD_TX_LEVELS[v] ) );
                    }

SA  start           () -> void {
                        if( isActive_ ) return;
                        //turn off connectable after allowing some time to change name
                        if( connectableTimeout_ and not --connectableTimeout_ ) connectable( false );
                        //set params if have not started before, else use NULL so will just update data
                        //(have to use this method if advertising is still active, but we are stopped here)
                        // ble_gap_adv_params_t const *pp = (handle_ == BLE_GAP_ADV_SET_HANDLE_NOT_SET) ? &params_ : NULL;
                        // error.check( sd_ble_gap_adv_set_configure(&handle_, &pdata_, pp) );
                       
                        // OR just set parameters also (if we change parameters over time)
                        // which will work because we are stopped
                        params_.properties.type = isConnectable_ ?
                                BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED :
                                BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED;
                        error.check( sd_ble_gap_adv_set_configure(&handle_, &pdata_, &params_) );
                        error.check( sd_ble_gap_adv_start(handle_, BLE_CONN_CFG_TAG_DEFAULT) );
                        isActive_ = true;
                        power( txPower_ );
                        if( battery.isOk() ) board.ok(); else board.caution();

                    }

SA  stop            () -> void {
                        if( not isActive_ ) return;
                        error.check( sd_ble_gap_adv_stop(handle_) );
                        isActive_ = false;
                    }

};

#undef SA
#define SA static auto
#undef SC

/*
    for all who include this file
    someone(main) will need to run adv.init()
    5 minutes average per history value
    3000ms advertising interval
    20 second temp update interval
*/
// void advInitCB(); //called from adv.init()
#ifdef TEMPERATURE_INTERNAL
    inline Advertising< MyTemperatureAD<TemperatureInternal<5> >, 3000, 20_sec > adv; 
#elif defined TEMPERATURE_TMP117
    inline Advertising< MyTemperatureAD<TemperatureTmp117<5> >, 3000, 20_sec > adv; 
#elif defined TEMPERATURE_SI7051
    inline Advertising< MyTemperatureAD<TemperatureSi7051<5> >, 3000, 20_sec > adv;
#else
    #error "Temperature source not defined in nRFconfig.hpp" 
#endif

