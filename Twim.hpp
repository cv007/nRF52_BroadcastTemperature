#pragma once

#include <cstdint>
#include <cstdbool>

#include "nrf_delay.h"

#include "Gpio.hpp"
#include "Print.hpp"

#define U32 uint32_t
#define U16 uint16_t
#define I16 int16_t
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
------------------------------------------------------------------------------*/
template<uint32_t BaseAddr_, PIN Sda_, PIN Scl_, PIN Pwr_>
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
                U32 AMOUNT;
                U32 LIST;
        }   RXD;
        struct {
                U32 PTR;        //0x544
                U32 MAXCNT;
                U32 AMOUNT;
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
    enum SHORTS { LASTTX_STARTRX = 7, LASTTX_SUSPEND, LASTTX_STOP, LASTRX_STARTTX,
                  LASTRX_STOP = 12 };
    enum INT    { STOPPED, ERROR, SUSPENDED, RXSTARTED, TXSTARTED, LASTRX, LASTTX };
    enum ERRORS { OVERRUN, ADDR_NACK, DATA_NACK };
    enum FREQ   { K100 = 0x01980000, K250 = 0x04000000, K400 = 0x06400000 };

    //give public access to registers
    static inline volatile Twim_&
    reg { *(reinterpret_cast<Twim_*>(BaseAddr_)) };

//--------------------
//  control
//--------------------
SA  enable          ()          { reg.ENABLE = 6; }
SA  disable         ()          { reg.ENABLE = 0; }
SA  isEnabled       ()          { return reg.ENABLE; }
SA  frequency       (FREQ e)    { reg.FREQUENCY = e; }
SA  address         (uint8_t v) { reg.ADDRESS = v; } //0-127

//--------------------
//  Easy-DMA buffers
//--------------------
                    //nRF52840 max len = 0xFFFF, nRF52810 = 0x3FFF
                    //caller will have to know what is max, and len
                    //is used as-is
SA  txBufferSet     (uint32_t addr, uint16_t len) {
                        reg.TXD.PTR = addr;
                        reg.TXD.MAXCNT = len;
                    }
                    template<unsigned N>
SA  txBufferSet     (uint8_t (&addr)[N]) {
                        txBufferSet( (uint32_t)addr, N );
                    }
SA  rxBufferSet     (uint32_t addr, uint16_t len) {
                        reg.RXD.PTR = addr;
                        reg.RXD.MAXCNT = len;
                    }
                    template<unsigned N>
SA  rxBufferSet     (uint8_t (&addr)[N]) {
                        rxBufferSet( (uint32_t)addr, N );
                    }

SA  txSentCount     () { return reg.TXD.AMOUNT; }
SA  rxReceivedCount () { return reg.RXD.AMOUNT; }

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
SA  clearError      ()          { reg.EVENTS.ERROR = 0; }
SA  clearSuspended  ()          { reg.EVENTS.SUSPENDED = 0; }
SA  clearRxStarted  ()          { reg.EVENTS.RXSTARTED = 0; }
SA  clearTxStarted  ()          { reg.EVENTS.TXSTARTED = 0; }
SA  clearLastRx     ()          { reg.EVENTS.LASTRX = 0; }
SA  clearLastTx     ()          { reg.EVENTS.LASTTX = 0; }
SA  clearEvents     ()          { clearStopped();
                                  clearError();
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

//--------------------
//  shorts
//--------------------
SA  shortsEnable    (SHORTS e)  { reg.SHORTS or_eq (1<<e); }
SA  shortsDisableAll()          { reg.SHORTS = 0; }

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
SA  clearAddrNack   ()          { reg.ERRORSRC = 1; }
SA  clearDataNack   ()          { reg.ERRORSRC = 2; }

//--------------------
//  init/constructors
//--------------------
SA  init            (uint8_t addr, FREQ f = K400) { 
                        address( addr );
                        frequency( f );  
                        //when twi not enabled, puts pins in a twi like state
                        Gpio<Sda_>::init( INPUT, S0D1, PULLUP );
                        Gpio<Scl_>::init( INPUT, S0D1, PULLUP );
                        //set twi pins, connected (when enabled)
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
                        //twi pins back to default so pullups not driving anything external
                        Gpio<Sda_>::init();
                        Gpio<Scl_>::init();
                        //if a power pin specified, turn off power to twi slave
                        if constexpr( Pwr_ != -1 ){
                            Gpio<Pwr_>::off();
                        }
                    }

    Twim            (uint8_t addr, FREQ f = K100) { init( addr, f ); }
    Twim            (){} //call init manually


//--------------------
//  tx/rx functions
//--------------------
                    template<unsigned TN, unsigned RN>
SA  xfer            (uint8_t (&txbuf)[TN], uint8_t (&rxbuf)[RN]) {
                        txBufferSet( txbuf );
                        rxBufferSet( rxbuf );
                        clearEvents();
                        shortsDisableAll();
                        shortsEnable( LASTTX_STARTRX ); //tx -> rx    
                        shortsEnable( LASTRX_STOP ); //rx -> stop
                        startTx();
                        while( not isError() and not isStopped() );                       
                        // DebugFuncHeader();
                        // Debug("  {Forange}twim xfer{Fwhite} 0x%08X\n", reg.ERRORSRC);
                        return txSentCount() == TN and rxReceivedCount() == RN;
                    }

                    template<unsigned N>
SA  write           (uint8_t (&txbuf)[N]) {
                        txBufferSet( txbuf );
                        clearEvents();
                        shortsDisableAll();
                        shortsEnable( LASTTX_STOP ); //tx -> stop 
                        startTx(); //also enables if not already
                        while( not isError() and not isStopped() );
                        // DebugFuncHeader();
                        // Debug("  {Forange}twim write{Fwhite} 0x%08X\n", reg.ERRORSRC );
                        return txSentCount() == N;
                    }
};

#undef U32
#undef U16
#undef I16
#undef SA 
#undef SI
#undef SCA

template<PIN Sda_, PIN Scl_, PIN Pwr_ = PIN(-1)>
using Twim0 = Twim<0x40003000, Sda_, Scl_, Pwr_>; //all

#ifdef  NRF52840 
template<PIN Sda_, PIN Scl_, PIN Pwr_ = PIN(-1)>
using Twim1 = Twim<0x40004000, Sda_, Scl_, Pwr_>; //nRF52840
#endif
