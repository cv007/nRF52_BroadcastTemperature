#pragma once

//TODO - clearing events and clearing interrupts can take up to 4 cycles
//so I guess can get into situation where exit isr before event is cleared
//or irq is cleared, although seems unlikely
//


#include "nRFconfig.hpp"

#include "nrf_delay.h"

#include "Gpio.hpp"
#include "Print.hpp"

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
template<u32 BaseAddr_, PIN Sda_, PIN Scl_, PIN Pwr_>
struct Twim {

//============
    protected:
//============

    struct Twim_; //forward declare register struct, at end

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
private: //have to go through init (or throught init via the constructor)
         //so will get the address, frequency, pins setup
SA  enable          ()          { reg.ENABLE = 6; }
SA  reEnable        ()          { disable(); clearError(); shortsSetup(ALL_OFF); enable(); }

public:
SA  disable         ()          { reg.ENABLE = 0; }
SA  isEnabled       ()          { return reg.ENABLE; }
SA  frequency       (FREQ e)    { reg.FREQUENCY = e; }
SA  address         (u8 v)      { reg.ADDRESS = v; } //0-127

//--------------------
//  Easy-DMA buffers
//--------------------
                    //nRF52840 max len = 0xFFFF, nRF52810 = 0x3FFF
                    //caller will have to know what is max, and len
                    //is used as-is
SA  txBufferSet     (u32 addr, u16 len) {
                        reg.TXD.MAXCNT = len;
                        reg.TXD.PTR = addr;
                    }

                    template<unsigned N>
SA  txBufferSet     (const u8 (&addr)[N]) {
                        txBufferSet( (u32)addr, N );
                    }

SA  rxBufferSet     (u32 addr, u16 len) {
                        reg.RXD.MAXCNT = len;
                        reg.RXD.PTR = addr;
                    }

                    //T so can be volatile or not volatile
                    template<typename T, unsigned N>
SA  rxBufferSet     (T (&addr)[N]) {
                        static_assert(sizeof(T) == 1, "Twi::rxBufferSet needs a byte array");
                        rxBufferSet( (u32)addr, N );
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
SA  init            (u8 addr, FREQ f = K400) { 
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

    Twim            (u8 addr, FREQ f = K100) { init( addr, f ); }
    Twim            (){} //call init manually


//--------------------
//  tx/rx functions
//--------------------

//TODO - add timeouts so if something wrong we don't block forever

SA  waitForStop     () {
                        while( not isStopped() ){
                            if( isError() ){
                                stop();
                                while( not isStopped() ){}
                                //DebugFuncHeader();
                                //Debug(FG ORANGE "  twim xfer ERRORSRC:" WHITE " 0x%08X\n", reg.ERRORSRC);
                                return false;
                            }
                        }
                        return true;
                    }

                    //write,read
                    template<typename T, unsigned NT, unsigned NR>
                    // [[ gnu::noinline ]]
SA  writeRead       (const u8 (&txbuf)[NT], T (&rxbuf)[NR]) {  
                        txBufferSet( txbuf );
                        rxBufferSet( rxbuf );
                        startTxRxStop(); 
                        if( not waitForStop() ) return false;
                        if( (txAmount() == NT) and (rxAmount() == NR) ) return true;
                        //unknown error, reset twi
                        reEnable();
                        return false;
                    }

                    //write only
                    template<unsigned N>
SA  write           (const u8 (&txbuf)[N]) {
                        txBufferSet( txbuf );
                        startTxStop();
                        if( not waitForStop() ) return false;
                        if( txAmount() == N ) return true;
                        //unknown error, reset twi
                        reEnable();
                        return false;
                    }

                    //read only
                    template<typename T, unsigned N>
SA  read            (T (&rxbuf)[N]) {
                        rxBufferSet( rxbuf );
                        startRxStop();
                        if( not waitForStop() ) return false;
                        if( rxAmount() == N ) return true;
                        //unknown error, reset twi
                        reEnable();
                        return false;
                    }

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
                u32 STARTRX;        //0x00
                u32 unused1;
                u32 STARTTX;        //0x08
                u32 unused2[2];
                u32 STOP;           //0x14
                u32 unused3;
                u32 SUSPEND;        //0x1C
                u32 RESUME;         //0x20
        }   TASKS;

        u32 unused4[(0x104-0x24)/4]; 

        struct {
                u32 STOPPED;            //0x104
                u32 unused5[(0x124-0x108)/4];
                u32 ERROR;              //0x124
                u32 unused6[(0x148-0x128)/4];
                u32 SUSPENDED;          //0x148
                u32 RXSTARTED;          //0x14C
                u32 TXSTARTED;          //0x150
                u32 unused7[2];
                u32 LASTRX;             //0x15C
                u32 LASTTX;             //0x160
        }   EVENTS;

        u32 unused8[(0x200-0x164)/4];

        u32 SHORTS;             //0x200
        u32 unused9[(0x300-0x204)/4];
        u32 INTEN;              //0x300
        u32 INTENSET;           //0x304
        u32 INTENCLR;           //0x308
        u32 unused10[(0x4C4-0x30C)/4];
        u32 ERRORSRC;           //0x4C4
        u32 unused11[(0x500-0x4C8)/4];
        u32 ENABLE;             //0x500
        u32 unused12;
        u32 PSEL_SCL;           //0x508
        u32 PSEL_SDA;           //0x50C
        u32 unused13[(0x524-0x510)/4];
        u32 FREQUENCY;          //0x524

        u32 unused14[(0x534-0x528)/4];

        struct {
                u32 PTR;        //0x534
                u32 MAXCNT;
                u32 AMOUNT;     //RO
                u32 LIST;
        }   RXD;
        struct {
                u32 PTR;        //0x544
                u32 MAXCNT;
                u32 AMOUNT;     //RO
                u32 LIST;
        }   TXD;

        u32 unused15[(0x588-0x554)/4];
        u32 ADDRESS;            //0x588
    };
    #pragma GCC diagnostic pop

};

template<PIN Sda_, PIN Scl_, PIN Pwr_ = PIN(-1)>
using Twim0 = Twim<0x40003000, Sda_, Scl_, Pwr_>; //all

#ifdef  NRF52840 
template<PIN Sda_, PIN Scl_, PIN Pwr_ = PIN(-1)>
using Twim1 = Twim<0x40004000, Sda_, Scl_, Pwr_>; //nRF52840 has 2 twi instances
#endif
