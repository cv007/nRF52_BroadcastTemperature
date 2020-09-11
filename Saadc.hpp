#pragma once

#include "nRFconfig.hpp"

#undef SA
//#define SA [[gnu::always_inline]] static auto
#define SA [[gnu::noinline]] static auto
#define SI static inline

/*------------------------------------------------------------------------------
    Saadc struct
------------------------------------------------------------------------------*/
struct Saadc {

//============
    protected:
//============

    SCA         base_   { 0x40007000 };
    SI uint8_t  inuse_  { 0 }; //channels in use 0b00000000

//------------
//  registers
//------------

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wpedantic"

    using cfgT = struct {
                u32 PSELP;
                u32 PSELN;
        union { u32 CONFIG;
        struct{     u32 RESP    : 2;
                    u32         : 2;
                    u32 RESN    : 2;
                    u32         : 2;
                    u32 GAIN    : 3;
                    u32         : 1;
                    u32 REFSEL  : 1;
                    u32         : 3;
                    u32 TACQ    : 3;
                    u32         : 1;
                    u32 MODE    : 1;
                    u32         : 3;
                    u32 BURST   : 1;
        };};
                i16 LIMITL;
                i16 LIMITH;
   };

    struct Saadc_ {
        struct {
                u32 START;        //0x00
                u32 SAMPLE;
                u32 STOP;  
                u32 CALIBRATE;
        } TASKS;

                u32 unused1[(0x100-0x10)/4]; 

        struct {
                u32 STARTED;            //0x100
                u32 END;
                u32 DONE;
                u32 RESULTDONE;
                u32 CALIBRATEDONE;
                u32 STOPPED;
        struct {    u32 H;
                    u32 L;
            }   LIMIT[8];               //0x118-0x154
        } EVENTS;

                u32 unused2[(0x300-0x158)/4];

                u32 INTEN;              //0x300
                u32 INTENSET;
                u32 INTENCLR;
                u32 unused3[(0x400-0x30C)/4];
                u32 STATUS;             //0x400
                u32 unused4[(0x500-0x404)/4];
                u32 ENABLE;             //0x500
                u32 unused5[3];

                cfgT CHCONFIG[8];       //0x510-0x58C

                u32 unused6[(0x5F0-0x590)/4];
                u32 RESOLUTION;         //0x5F0
                u32 OVERSAMPLE;
                u32 SAMPLERATE;
                u32 unused7[(0x62C-0x5FC)/4];
                u32 RESULTPTR;          //0x62C
                u32 RESULTMAXCNT;
                u32 RESULTAMOUNT;
    };
    #pragma GCC diagnostic pop

//============
    public:
//============

    // enums
    enum PSEL     { NC, AIN0, AIN1, AIN2, AIN3, AIN4, AIN5, AIN6, AIN7, 
                    VDD, VDDHDIV5 = 0x0D };
    enum CH       { CH0, CH1, CH2, CH3, CH4, CH5, CH6, CH7 };
    enum GAIN     { DIV6, DIV5, DIV4, DIV3, DIV2, DIV1, MUL2, MUL4 };
    enum REFSEL   { INT0V6, VDD_DIV4 };
    enum TACQ     { T3US, T5US, T10US, T15US, T20US, T40US };
    enum MODE     { SE, DIFF };
    enum RESISTOR { BYPASS, PULLGND, PULLVDD, PULLVDD_DIV2 };
    enum BURST    { BURSTOFF, BURSTON };
    enum RES      { RES8, RES10, RES12, RES14 };
    enum INT      { STARTED, END, DONE, RESULT, CALIBRATE, STOPPED,
                    CH0H, CH0L, CH1H, CH1L, CH2H, CH2L, CH3H, CH3L,
                    CH4H, CH4L, CH5H, CH5L, CH6H, CH6L, CH7H, CH7L };
    enum OVERSAMP { OVEROFF, OVER2X, OVER4X, OVER8X, OVER16X, OVER32X, 
                    OVER64X, OVER128X, OVER256X  };


    //give public access to registers
    static inline volatile Saadc_&
    reg { *(reinterpret_cast<Saadc_*>(base_)) };

//--------------------
//  control, status
//--------------------
SA  isBusy          ()          { return reg.STATUS; } //conversion in progress?
SA  enable          ()          { reg.ENABLE = 1; }
SA  disable         ()          { reg.ENABLE = 0; }
SA  isEnabled       ()          { return reg.ENABLE; }

//--------------------
//  events
//--------------------
SA  clearStarted    ()          { reg.EVENTS.STARTED = 0; }
SA  clearBufferFull ()          { reg.EVENTS.END = 0; }
SA  clearConversion ()          { reg.EVENTS.DONE = 0; }
SA  clearResult     ()          { reg.EVENTS.RESULTDONE = 0; }
SA  clearCalibrated ()          { reg.EVENTS.CALIBRATEDONE = 0; }
SA  clearStopped    ()          { reg.EVENTS.STOPPED = 0; }
SA  clearLimitHigh  (CH e)      { reg.EVENTS.LIMIT[e].H = 0; }
SA  clearLimitLow   (CH e)      { reg.EVENTS.LIMIT[e].L = 0; }
SA  clearEvents     ()          { 
                                    clearStarted();
                                    clearBufferFull();
                                    clearConversion();
                                    clearResult();
                                    clearStopped();
                                }

SA  isStarted       ()          { return reg.EVENTS.STARTED; }
SA  isBufferFull    ()          { return reg.EVENTS.END; } //ram buffer is filled
SA  isConversion    ()          { return reg.EVENTS.DONE; } //a conversion was done
SA  isResult        ()          { return reg.EVENTS.RESULTDONE; } //when conversion(s) stored to ram
SA  isCalibrated    ()          { return reg.EVENTS.CALIBRATEDONE; }
SA  isStopped       ()          { return reg.EVENTS.STOPPED; }
SA  isLimitHigh     (CH e)      { return reg.EVENTS.LIMIT[e].H; }
SA  isLimitLow      (CH e)      { return reg.EVENTS.LIMIT[e].L; }

//--------------------
//  tasks
//--------------------
SA  start           ()          { enable(); reg.TASKS.START = 1; } 
SA  sample          ()          { reg.TASKS.SAMPLE = 1; } 
SA  stop            ()          { reg.TASKS.STOP = 1; } 
SA  calibrate       ()          {   
                                    enable();
                                    reg.TASKS.CALIBRATE = 1;
                                    while( not isCalibrated() );
                                    clearCalibrated();
                                    //leave enabled
                                }
//--------------------
//  interrupts
//--------------------
SCA ch2int          (CH e, bool L)  { return INT(CH0H + e*2 +L); } //convert CH to INT CH0H-CH7L

SA  irqOn           (INT e)         { reg.INTENSET = 1<<e; }
SA  irqOff          (INT e)         { reg.INTENCLR = 1<<e; }
SA  irqAllOff       ()              { reg.INTENCLR = 0xFFFFFFFF; }
SA  isIrqOn         (INT e)         { return reg.INTEN bitand (1<<e); }

SA  irqOnLimitH     (CH e)          { irqOn( ch2int(e,0) ); } //H=0,L=1
SA  irqOnLimitL     (CH e)          { irqOn( ch2int(e,1) ); }
SA  irqOffLimitH    (CH e)          { irqOff( ch2int(e,0) ); }
SA  irqOffLimitL    (CH e)          { irqOff( ch2int(e,1) ); }

//--------------------
//  channel config
//--------------------
SA  isChannelUsed   (CH e)      { return inuse_ bitand (1<<e); }
SA  channelSetup    (CH e, u32 cfg, PSEL p, PSEL n = NC) { 
                                    if( p or n ) inuse_ or_eq (1<<e);
                                    reg.CHCONFIG[e].PSELP = p;
                                    reg.CHCONFIG[e].PSELN = n; 
                                    reg.CHCONFIG[e].CONFIG = cfg;                      
                                }
SA  channelRelease  (CH e)      {
                                    inuse_ and_eq compl (1<<e); 
                                    reg.CHCONFIG[e].PSELP = NC;
                                    reg.CHCONFIG[e].PSELN = NC;
                                }
SA  channelOnly     (CH e)      {
                                    for( int i = CH0; i <= CH7; i++ ){
                                        if( e != i ) channelRelease( (CH)i );
                                    }
                                }

SA  limitH          (CH e, i16 v)   { reg.CHCONFIG[e].LIMITH = v; } 
SA  limitH          (CH e)          { return reg.CHCONFIG[e].LIMITH; } 
SA  limitL          (CH e, i16 v)   { reg.CHCONFIG[e].LIMITL = v; }
SA  limitL          (CH e)          { return reg.CHCONFIG[e].LIMITL; } 
SA  limitHL         (CH e, i16 H, i16 L) { limitH(e,H); limitL(e,L); }

//--------------------
//  config
//--------------------
SA  resolution      (RES e)         { reg.RESOLUTION = e; }
SA  resolution      ()              { return RES(reg.RESOLUTION); }

SA  overSample      (OVERSAMP e)    { reg.OVERSAMPLE = e; }
SA  overSample      ()              { return OVERSAMP(reg.OVERSAMPLE); }

SA  sampleRate      (u16 v)         {   
                                        if( v < 80 ) v = 80; 
                                        if( v > 2047 ) v =2047;
                                        reg.SAMPLERATE = v bitor (1<<12);
                                    }
SA  sampleRateTask  ()              { reg.SAMPLERATE = 0; }

SA  bufferAddr      (u32 v)         { reg.RESULTPTR = v; }
SA  bufferAddr      ()              { return reg.RESULTPTR; }
SA  bufferSize      (u16 v)         { reg.RESULTMAXCNT = v; } //15bits (max 32767)
SA  bufferSize      ()              { return reg.RESULTMAXCNT; }
SA  bufferSet       (u32 v, u16 n)  { bufferAddr(v); bufferSize(n); }
SA  bufferUsed      ()              { return reg.RESULTAMOUNT; }


SA  deinit          (CH e) {
                        irqOff( INT(CH0H + e*2) ); //convert CH to INT CH0H-CH7L
                        irqOff( INT(CH0L + e*2) );
                        reg.CHCONFIG[e].CONFIG = 0x20000; //default values
                        reg.CHCONFIG[e].LIMITL = -32768;
                        reg.CHCONFIG[e].LIMITH = 32767;
                        clearLimitLow( e );
                        clearLimitHigh( e );
                        channelRelease( e );
                    }

};



/*------------------------------------------------------------------------------
    SaadcChan struct
------------------------------------------------------------------------------*/
struct SaadcChan : Saadc {

//============
    private:
//============

    SI CH   ch_     { CH0 };
    SI PSEL pselP_  { NC };
    SI PSEL pselN_  { NC };
    SI u32  config_ { 0 };

                    //positive already set in init values for cfgT
                    //so only need to do PSELN
                    template<typename ...Ts>
SCA init            (cfgT& it, PSEL e, Ts... ts) {
                        it.PSELN = e;
                        init(it, ts...); 
                    }
                    //need both as a pair, resp first
                    template<typename ...Ts>
SCA init            (cfgT& it, RESISTOR p, RESISTOR n, Ts... ts) {
                        it.RESP = p; 
                        it.RESN = n;
                        init(it, ts...); 
                    }
                    template<typename ...Ts>
SCA init            (cfgT& it, GAIN e, Ts... ts) {
                        it.GAIN = e;
                        init(it, ts...); 
                    }
                    template<typename ...Ts>
SCA init            (cfgT& it, REFSEL e, Ts... ts) {
                        it.REFSEL = e;
                        init(it, ts...); 
                    }
                    template<typename ...Ts>
SCA init            (cfgT& it, BURST e, Ts... ts) {
                        it.BURST = e;
                        init(it, ts...); 
                    }
                    template<typename ...Ts>
SCA init            (cfgT& it, TACQ e, Ts... ts) {
                        it.TACQ = e;
                        init(it, ts...); 
                    }
                    template<typename ...Ts>
SCA init            (cfgT& it, MODE e, Ts... ts) {
                        it.MODE = e;
                        init(it, ts...); 
                    }
SCA init            (cfgT& it) { //no more arguments
                        pselP_ = (PSEL)it.PSELP;
                        pselN_ = (PSEL)it.PSELN;
                        config_ = it.CONFIG;
                    }

//============
    public:
//============

    SaadcChan       () {} //no init wanted (manually init later)

                    //init from constructor (CH specified)
                    template<typename ...Ts>
    SaadcChan       (CH ch, PSEL p, Ts... ts) {
                        init(ch, p, ts...);
                    }

                    //init from constructor (default CH0)
                    template<typename ...Ts>
    SaadcChan       (PSEL p, Ts... ts) {
                        init(CH0, p, ts...);
                    }

                    //manual init
                    template<typename ...Ts>
SCA init            (CH ch, PSEL p, Ts... ts) { 
                        ch_ = ch;
                        cfgT it{p, NC, 0x20000}; 
                        init(it, ts...); 
                    }

//============
    private:
//============

                    //setup our channel config and buffer in Saadc
                    //take exclusive use of Saadc
SA setConfig        (i16& v) {
                        if( isBusy() ) return false;        //is in use
                        if( pselP_ == NC and pselN_ == NC ) return false; //or we are not init
                        channelSetup( ch_, config_, pselP_, pselN_ );  //set config and inputs
                        bufferSet( (u32)&v, 1 );
                        channelOnly( ch_ );                 //disable all other channels
                        return true;
                    }

                    //get a single sample (blocking) - 
                    //TODO, should have timeout here 
SA  sample1         () {
                        clearConversion();
                        sample();
                        while( not isConversion() );     
                    }

                    //get a single result (blocking) - 
                    //could be >1 sample if oversample is on
                    //TODO, should have timeout here  
SA  result1         () {
                        clearResult();
                        for( ; not isResult(); sample() );  
                    }

//============
    public:
//============

                    //get with a specific resolution, and number of samples
SA  read            (i16& v, RES r, OVERSAMP s = OVEROFF) {
                        if( not setConfig( v ) ) return false;
                        RES rr = resolution();          //save old
                        OVERSAMP ss = overSample();
                        resolution( r );                //set new
                        overSample( s );
                        start();                        //start will also enable
                        result1();
                        resolution( rr );               //restore old
                        overSample( ss );
                        disable();
                        clearEvents();
                        channelRelease( ch_ );
                        return true;
                    }

};

#undef SA
#define SA static auto
#undef SI
