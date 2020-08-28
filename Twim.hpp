#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_delay.h"

#include "Gpio.hpp"
#include "Print.hpp"

#define U32 uint32_t
#define U16 uint16_t
#define I16 int16_t
#define U8 uint8_t
// #define SA [[gnu::always_inline]] static auto
// #define SA [[gnu::noinline]] static auto
#define SA static auto
#define SI static inline
#define SCA static constexpr auto

/*------------------------------------------------------------------------------
    Twim struct (TWI master)
    BaseAddr_ = peripheral base address
    Sda_ = sda pin (from GPIO::PIN)
    Scl_ = scl pin (from GPIO::PIN)
    Pwr_ = pwr pin (from GPIO::PIN) - optional, if power the slave from a pin

    NOTE
    this was created from the nRF52810 product specification (datasheet)
    and the nRF52840 may have some differences, like an additional SHORTS
    option (see enum), the nRF52840 also has a 'simple' Twi chapter (no 
    Easy-DMA) using ENABLE = 5, that the nRF52810 does not show, but they
    most likely both will work with the code below (only tested on a nRF52810)
------------------------------------------------------------------------------*/
template<U32 BaseAddr_, PIN Sda_, PIN Scl_, PIN Pwr_>
struct Twim {

//============
    protected:
//============

//------------
//  registers
//------------

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"

    struct Twim_ {
        struct {
                U32 STARTRX;        //0x00
                U32 unused1;
                U32 STARTTX;        //0x08
                U32 unused2[2];
                U32 STOP;           //0x14
                U32 unused3;
                U32 SUSPEND;        //0x1C
                U32 RESUME;         //0x20
        }   TASKS;

        U32 unused4[(0x104-0x24)/4]; 

        struct {
                U32 STOPPED;            //0x104
                U32 unused5[(0x124-0x108)/4];
                U32 ERROR;              //0x124
                U32 unused6[(0x148-0x128)/4];
                U32 SUSPENDED;          //0x148
                U32 RXSTARTED;          //0x14C
                U32 TXSTARTED;          //0x150
                U32 unused7[2];
                U32 LASTRX;             //0x15C
                U32 LASTTX;             //0x160
        }   EVENTS;

        U32 unused8[(0x200-0x164)/4];

        U32 SHORTS;             //0x200
        U32 unused9[(0x300-0x204)/4];
        U32 INTEN;              //0x300
        U32 INTENSET;           //0x304
        U32 INTENCLR;           //0x308
        U32 unused10[(0x4C4-0x30C)/4];
        U32 ERRORSRC;           //0x4C4
        U32 unused11[(0x500-0x4C8)/4];
        U32 ENABLE;             //0x500
        U32 unused12;
        U32 PSEL_SCL;           //0x508
        U32 PSEL_SDA;           //0x50C
        U32 unused13[(0x524-0x510)/4];
        U32 FREQUENCY;          //0x524

        U32 unused14[(0x534-0x528)/4];

        struct {
                U32 PTR;        //0x534
                U32 MAXCNT;
                U32 AMOUNT;     //RO
                U32 LIST;
        }   RXD;
        struct {
                U32 PTR;        //0x544
                U32 MAXCNT;
                U32 AMOUNT;     //RO
                U32 LIST;
        }   TXD;

        U32 unused15[(0x588-0x554)/4];
        U32 ADDRESS;            //0x588
    };
    #pragma GCC diagnostic pop

//============
    public:
//============

    // enums
                //TODO NOTE the nRF52840 also has LASTRX_SUSPEND (11)
                //these are bitmasks
    enum SHORTS { LASTTX_STARTRX = 1<<7, LASTTX_SUSPEND = 1<<8, LASTTX_STOP = 1<<9,
                  LASTRX_STARTTX = 1<<10, LASTRX_STOP = 1<<12,
                  //added these
                  LASTTX_STARTRX_STOP = LASTTX_STARTRX|LASTRX_STOP,
                  ALL_OFF = 0
                 }; 
    enum INT    { STOPPED, ERROR, SUSPENDED, RXSTARTED, TXSTARTED, LASTRX, LASTTX };
    enum FREQ   { K100 = 0x01980000, K250 = 0x04000000, K400 = 0x06400000 };

    //give public access to registers
    static inline volatile Twim_& reg { *(reinterpret_cast<Twim_*>(BaseAddr_)) };

//--------------------
//  control
//--------------------
SA  enable          ()          { reg.ENABLE = 6; }
SA  disable         ()          { reg.ENABLE = 0; }
SA  isEnabled       ()          { return reg.ENABLE; }
SA  frequency       (FREQ e)    { reg.FREQUENCY = e; }
SA  address         (U8 v)      { reg.ADDRESS = v; } //0-127

//--------------------
//  Easy-DMA buffers
//--------------------
                    //nRF52840 max len = 0xFFFF, nRF52810 = 0x3FFF
                    //caller will have to know what is max, and len
                    //is used as-is
SA  txBufferSet     (U32 addr, U16 len) {
                        reg.TXD.MAXCNT = len;
                        reg.TXD.PTR = addr;
                    }

                    template<unsigned N>
SA  txBufferSet     (U8 (&addr)[N]) {
                        txBufferSet( (U32)addr, N );
                    }

SA  rxBufferSet     (U32 addr, U16 len) {
                        reg.RXD.MAXCNT = len;
                        reg.RXD.PTR = addr;
                    }

                    template<unsigned N>
SA  rxBufferSet     (U8 (&addr)[N]) {
                        rxBufferSet( (U32)addr, N );
                    }

SA  txAmount        ()          { return reg.TXD.AMOUNT; }
SA  rxAmount        ()          { return reg.RXD.AMOUNT; }

//--------------------
//  pins (uses GPIO::PIN)
//--------------------
SA  pinSclConnect   ()          { reg.PSEL_SCL and_eq compl (1<<31); }
SA  pinSdaConnect   ()          { reg.PSEL_SDA and_eq compl (1<<31); } 
SA  pinSclDisconnect()          { reg.PSEL_SCL or_eq (1<<31); } 
SA  pinSdaDisconnect()          { reg.PSEL_SDA or_eq (1<<31); }


                    //set pins only when disabled
SA  pinScl          (PIN e, bool on = true) { 
                        reg.PSEL_SCL = e;
                        if( not on ) pinSclDisconnect();
                    } 

SA  pinSda          (PIN e, bool on = true) {
                        reg.PSEL_SDA = e;
                        if( not on ) pinSdaDisconnect();
                    }


//--------------------
//  events
//--------------------
SA  clearStopped    ()          { reg.EVENTS.STOPPED = 0; }
SA  clearError      ()          { reg.ERRORSRC = 7; reg.EVENTS.ERROR = 0; }
SA  clearSuspended  ()          { reg.EVENTS.SUSPENDED = 0; }
SA  clearRxStarted  ()          { reg.EVENTS.RXSTARTED = 0; }
SA  clearTxStarted  ()          { reg.EVENTS.TXSTARTED = 0; }
SA  clearLastRx     ()          { reg.EVENTS.LASTRX = 0; }
SA  clearLastTx     ()          { reg.EVENTS.LASTTX = 0; }
SA  clearEvents     ()          { clearError();
                                  clearStopped();                                
                                  clearSuspended();
                                  clearRxStarted();
                                  clearTxStarted();
                                  clearLastRx();
                                  clearLastTx();                                  
                                }

SA  isError         ()          { return reg.EVENTS.ERROR; }
SA  isStopped       ()          { return reg.EVENTS.STOPPED; }
SA  isSuspended     ()          { return reg.EVENTS.SUSPENDED; }
SA  isRxStarted     ()          { return reg.EVENTS.RXSTARTED; }
SA  isTxStarted     ()          { return reg.EVENTS.TXSTARTED; }
SA  isLastRx        ()          { return reg.EVENTS.LASTRX; }
SA  isLastTx        ()          { return reg.EVENTS.LASTTX; }

//--------------------
//  tasks
//--------------------
SA  startRx         ()          { reg.TASKS.STARTRX = 1; } 
SA  startTx         ()          { reg.TASKS.STARTTX = 1; } 
SA  stop            ()          { reg.TASKS.STOP = 1; } //cannot stop while suspended
SA  suspend         ()          { reg.TASKS.SUSPEND = 1; }
SA  resume          ()          { reg.TASKS.RESUME = 1; }
SA  startTxRxStop   ()          { clearEvents(); shortsSetup(LASTTX_STARTRX_STOP); startTx(); }
SA  startTxStop     ()          { clearEvents(); shortsSetup(LASTTX_STOP); startTx(); }
SA  startRxStop     ()          { clearEvents(); shortsSetup(LASTRX_STOP); startRx(); }

//--------------------
//  shorts
//--------------------
SA  shortsSetup     (SHORTS e)  { reg.SHORTS = e; }

//--------------------
//  interrupts
//--------------------
SA  irqOn           (INT e)     { reg.INTENSET = (1<<e); }
SA  irqOff          (INT e)     { reg.INTENCLR = (1<<e); }
SA  irqAllOff       ()          { reg.INTENCLR = 0xFFFFFFFF; }
SA  isIrqOn         (INT e)     { return reg.INTEN bitand (1<<e); }

//--------------------
//  errors
//--------------------
SA  isOverrun       ()          { return reg.ERRORSRC bitand 1; }
SA  isAddrNack      ()          { return reg.ERRORSRC bitand 2; }
SA  isDataNack      ()          { return reg.ERRORSRC bitand 4; }

//--------------------
//  init/constructors
//--------------------
SA  init            (U8 addr, FREQ f = K400) { 
                        address( addr );
                        frequency( f );  
                        //when twi not enabled, set pins gpio state like twi
                        Gpio<Sda_>::init( INPUT, S0D1, PULLUP );
                        Gpio<Scl_>::init( INPUT, S0D1, PULLUP );
                        //set twi pins, (default is connected when enabled)
                        pinSda( Sda_ );
                        pinScl( Scl_ ); 
                        //if a power pin specified, turn on to power twi slave
                        if constexpr( Pwr_ != PIN(-1) ){
                            Gpio<Pwr_>::init( OUTPUT, S0H1 );
                            Gpio<Pwr_>::on();
                        }
                        enable(); 
                    }

SA  deinit          () { 
                        disable();
                        //if a power pin specified, turn off power to twi slave
                        if constexpr( Pwr_ != -1 ){
                            Gpio<Pwr_>::off();
                            //and twi pins back to default so no pullups driving anything
                            Gpio<Sda_>::init();
                            Gpio<Scl_>::init();
                        }
                    }

    Twim            (U8 addr, FREQ f = K100) { init( addr, f ); }
    Twim            (){} //call init manually


//--------------------
//  tx/rx functions
//--------------------

//TODO - add ability to specify if STOP is wanted (and have default as true)
//so can do repeated starts
//just do STOP manually instead of using shorts, since these
//are blocking functions anyway
//should also add timeouts so if something wrong we don't block forever

SA  waitForStop     () {
                        bool err;
                        while( err = isError(), not err and not isStopped() ){}
                        if( err ){
                            stop();
                            // DebugFuncHeader();
                            // Debug("  {Forange}twim xfer ERRORSRC:{Fwhite} 0x%08X\n", reg.ERRORSRC);
                        }
                        return not err;
                    }

                    //write,read
                    template<unsigned NT, unsigned NR>
SA  writeRead       (U8 (&txbuf)[NT], U8 (&rxbuf)[NR]) {
                        txBufferSet( txbuf );
                        rxBufferSet( rxbuf );
                        startTxRxStop();  
asm("nop");
                        return waitForStop() and (txAmount() == NT) and (rxAmount() == NR);

                    }

                    //write only
                    template<unsigned N>
SA  write           (U8 (&txbuf)[N]) {
                        txBufferSet( txbuf );
                        startTxStop();
                        return waitForStop() and (txAmount() == N);
                    }

                    //read only
                    template<unsigned N>
SA  read            (U8 (&rxbuf)[N]) {
                        rxBufferSet( rxbuf );
                        startRxStop();
                        return waitForStop() and (rxAmount() == N);
                    }
};

#undef U32
#undef U16
#undef I16
#undef U8
#undef SA 
#undef SI
#undef SCA

template<PIN Sda_, PIN Scl_, PIN Pwr_ = PIN(-1)>
using Twim0 = Twim<0x40003000, Sda_, Scl_, Pwr_>; //all

#ifdef  NRF52840 
template<PIN Sda_, PIN Scl_, PIN Pwr_ = PIN(-1)>
using Twim1 = Twim<0x40004000, Sda_, Scl_, Pwr_>; //nRF52840 has 2 twi instances
#endif
