#pragma once

#include <cstdint>
#include <cstdbool>

#define U32 uint32_t
#define U16 uint16_t
#define I16 int16_t
//#define SA [[gnu::always_inline]] static auto
#define SA [[gnu::noinline]] static auto
#define SI static inline
#define SCA static constexpr auto

/*------------------------------------------------------------------------------
    enums - in structs so do not pollute the global namespace
------------------------------------------------------------------------------*/
struct SAADC_PSEL       { enum PSEL     { NC, AIN0, AIN1, AIN2, AIN3, AIN4, AIN5, 
                                          AIN6, AIN7, VDD, VDDHDIV5 = 0x0D }; };
struct SAADC_CH         { enum CH       { CH0, CH1, CH2, CH3, CH4, CH5, CH6, CH7 }; };
struct SAADC_GAIN       { enum GAIN     { DIV6, DIV5, DIV4, DIV3, DIV2, DIV1, 
                                          MUL2, MUL4 }; };
struct SAADC_REFSEL     { enum REFSEL   { INT0V6, VDD_DIV4 }; };
struct SAADC_TACQ       { enum TACQ     { T3US, T5US, T10US, T15US, T20US, T40US }; };
struct SAADC_MODE       { enum MODE     { SE, DIFF }; };
struct SAADC_RESISTOR   { enum RESISTOR { BYPASS, PULLGND, PULLVDD, PULLVDD_DIV2 }; };
struct SAADC_BURST      { enum BURST    { BURSTOFF, BURSTON }; };
struct SAADC_RES        { enum RES      { RES8, RES10, RES12, RES14 }; };
struct SAADC_INT        { enum INT      { STARTED, END, DONE, RESULT, CALIBRATE, STOPPED,
                                          CH0H, CH0L, CH1H, CH1L, CH2H, CH2L, CH3H, CH3L,
                                          CH4H, CH4L, CH5H, CH5L, CH6H, CH6L, CH7H, CH7L }; };
struct SAADC_OVERSAMP   { enum OVERSAMP { OVEROFF, OVER2X, OVER4X, OVER8X, OVER16X, OVER32X, 
                                          OVER64X, OVER128X, OVER256X  }; };

/*------------------------------------------------------------------------------
    Saadc struct
------------------------------------------------------------------------------*/
struct Saadc : SAADC_CH, SAADC_PSEL, SAADC_RES, SAADC_INT,SAADC_OVERSAMP {

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
                U32 PSELP;
                U32 PSELN;
        union { U32 CONFIG;
        struct{     U32 RESP    : 2;
                    U32         : 2;
                    U32 RESN    : 2;
                    U32         : 2;
                    U32 GAIN    : 3;
                    U32         : 1;
                    U32 REFSEL  : 1;
                    U32         : 3;
                    U32 TACQ    : 3;
                    U32         : 1;
                    U32 MODE    : 1;
                    U32         : 3;
                    U32 BURST   : 1;
        };};
                I16 LIMITL;
                I16 LIMITH;
   };

    struct Saadc_ {
        struct {
                U32 START;        //0x00
                U32 SAMPLE;
                U32 STOP;  
                U32 CALIBRATE;
        } TASKS;

                U32 unused1[(0x100-0x10)/4]; 

        struct {
                U32 STARTED;            //0x100
                U32 END;
                U32 DONE;
                U32 RESULTDONE;
                U32 CALIBRATEDONE;
                U32 STOPPED;
        struct {    U32 H;
                    U32 L;
            }   LIMIT[8];               //0x118-0x154
        } EVENTS;

                U32 unused2[(0x300-0x158)/4];

                U32 INTEN;              //0x300
                U32 INTENSET;
                U32 INTENCLR;
                U32 unused3[(0x400-0x30C)/4];
                U32 STATUS;             //0x400
                U32 unused4[(0x500-0x404)/4];
                U32 ENABLE;             //0x500
                U32 unused5[3];

                cfgT CHCONFIG[8];       //0x510-0x58C

                U32 unused6[(0x5F0-0x590)/4];
                U32 RESOLUTION;         //0x5F0
                U32 OVERSAMPLE;
                U32 SAMPLERATE;
                U32 unused7[(0x62C-0x5FC)/4];
                U32 RESULTPTR;          //0x62C
                U32 RESULTMAXCNT;
                U32 RESULTAMOUNT;
    };
    #pragma GCC diagnostic pop

//============
    public:
//============

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
SA  channelSetup    (CH e, U32 cfg, PSEL p, PSEL n = NC) { 
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

SA  limitH          (CH e, I16 v)   { reg.CHCONFIG[e].LIMITH = v; } 
SA  limitH          (CH e)          { return reg.CHCONFIG[e].LIMITH; } 
SA  limitL          (CH e, I16 v)   { reg.CHCONFIG[e].LIMITL = v; }
SA  limitL          (CH e)          { return reg.CHCONFIG[e].LIMITL; } 
SA  limitHL         (CH e, I16 H, I16 L) { limitH(e,H); limitL(e,L); }

//--------------------
//  config
//--------------------
SA  resolution      (RES e)         { reg.RESOLUTION = e; }
SA  resolution      ()              { return RES(reg.RESOLUTION); }

SA  overSample      (OVERSAMP e)    { reg.OVERSAMPLE = e; }
SA  overSample      ()              { return OVERSAMP(reg.OVERSAMPLE); }

SA  sampleRate      (U16 v)         {   
                                        if( v < 80 ) v = 80; 
                                        if( v > 2047 ) v =2047;
                                        reg.SAMPLERATE = v bitor (1<<12);
                                    }
SA  sampleRateTask  ()              { reg.SAMPLERATE = 0; }

SA  bufferAddr      (U32 v)         { reg.RESULTPTR = v; }
SA  bufferAddr      ()              { return reg.RESULTPTR; }
SA  bufferSize      (U16 v)         { reg.RESULTMAXCNT = v; } //15bits (max 32767)
SA  bufferSize      ()              { return reg.RESULTMAXCNT; }
SA  bufferSet       (U32 v, U16 n)  { bufferAddr(v); bufferSize(n); }
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
struct SaadcChan : Saadc, SAADC_GAIN, SAADC_REFSEL, SAADC_TACQ,
                   SAADC_MODE, SAADC_RESISTOR, SAADC_BURST {

//============
    private:
//============

    SI CH   ch_     { CH0 };
    SI PSEL pselP_  { NC };
    SI PSEL pselN_  { NC };
    SI U32  config_ { 0 };

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
SA setConfig        (I16& v) {
                        if( isBusy() ) return false;        //is in use
                        if( pselP_ == NC and pselN_ == NC ) return false; //or we are not init
                        channelSetup( ch_, config_, pselP_, pselN_ );  //set config and inputs
                        bufferSet( (U32)&v, 1 );
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
SA  read            (I16& v, RES r, OVERSAMP s = OVEROFF) {
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

#undef U32
#undef U16
#undef I16
#undef SA 
#undef SI
#undef SCA