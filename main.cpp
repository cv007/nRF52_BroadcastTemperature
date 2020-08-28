/*-----------------------------------------------------------------------------
    
nRF52840 Dongle    

advertise temperature, with the bluetooth name showing a letter A-Z followed
by the value in degrees F to the tenth degree
    A 80.2  C 72.5  T-10.3
also a UUID value is used to show battery voltage in millivolts,
a counter and temperature history data in bcd
    de9f0001-3021-0769-0779-077907700771
    de9f is a fixed value (degF)
    0001 is a counter, incremented every time a new temp is taken (1 minute)
    3021 is battery voltage in millivolts (3.021v)- battery is connected to VDD
    the rest of the 4 digit bcd values are 5 minute averages
    0769 is the current 5 minute average (including current temp)
    0779 is the previous 5 minute average, and so on

the letter (A - Z) can be changed at power on- 
    green led 5 blinks = can start setting new letter if wanted
    press sw1 - each press increments letter, 
        first press sets to A, second to B, etc.
    timeout of 10 seconds (no sw pressed)
    5 green led blinks if no change was made, 2 if new value was saved
    any new value set is stored in flash

2xAAA power connected to VDD (and VDDH)
dongle has dcdc circuitry for both REG0 and REG1
dongle manual indicates a jumper needs to be cut (Vbus->VDDH), and
    another jumpered (VDD->VDDH), to use non-vbus power
I did not want to do this as I still wanted to be able to plug into
usb (with usb then supplying the power)
connecting power to VDD works, but you then get an extra ~600uA+ in use
unless you also jumper VDD to VBUS (VDDH), which then results in ~15uA in sleep
(probably REG0 ldo stays in use when VDDH is unused, or something)


VDDH -> REG0 -> VDD/GPIO -> REG1

dongle sits on a carrier board with VDD and VDDH shorted and powered by the
 pair of AAA's, so REG0 is unused, GPIO is battery voltage ~3V or less, and
 REG1 dc-dc is enabled to provide the system power of 1.8v)

defaults - 
    DCDEN0 DCDCEN are off (REG0/1 using LDO)
    UICR.REGOUT0 default is 1.8v (7), but the dongle is set to 3.3v (5)
    (although makes no difference since REG0 is bypassed)

if DCDCEN0 is on, then REG0 DCDC converter is used to supply VDD (and GPIO)
if DCDCEN is on, then REG1 DCDC converter is used to supply system power (1.3v)

will enable DCDCEN for REG1

-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include <cstdbool>
#include <cstdint>

#include "nrf_power.h"
#include "nrf_pwr_mgmt.h"

//(none of these have constructors)
#include "Boards.hpp"       //provides inline class var 'board'
#include "Advertising.hpp"  //provides inline class var 'adv'
#include "Errors.hpp"       //provides inline class var 'error'
#include "Ble.hpp"          //provides inline class var 'ble'

#include "Print.hpp"


/*-----------------------------------------------------------------------------
    functions
-----------------------------------------------------------------------------*/
int main() {

    Debug( "{normal}{Fgreen}\nBoot...\n" );
    Debug( "{Fmagenta}board.init()...\n" );
    board.init();           //init board pins
    board.alive();          //blink led's to show boot


#if 1
//========================
//test 2 - deal with twi directly, no Tmp117
//testing twi problem, test with no bluetooth, 

//result - similar problem - now rbuf[0] (the upper byte) will be the same
//as the previous rbuf[0] status read value, and rbuf[1] will be a valid byte
//the LA shows the correct values on the line
//also adding a nop to Twim::writeRead will also 'fix' the problem

Twim0< board.sda.pinNumber(),   
       board.scl.pinNumber(), 
       board.i2cDevicePwr.pinNumber() > twi{ 0x48 }; //tmp117 address, default 100kHz

//tmp117 is now powered up, with a 2ms delay

//just check status 1/sec, get temp if ready
//default is continuous, 8samples (125ms), repeat 1/sec

uint8_t tbuf[1]; //1 = status register, 0 = temperature
uint8_t rbuf[2]; //status value read
for(;;){
    nrf_delay_ms(1000);
    tbuf[0] = 1; //status register address

    //read status to get ready bit
    if( twi.writeRead(tbuf, rbuf) ){        //if no error
        int16_t v = (rbuf[0]<<8)|rbuf[1];   //big endian-> little endian
        Debug("{Fgreen} status: 0x%04X\n", v );
        if( not (rbuf[0] bitand 0x20) ) continue; //not ready

        //ready, read temp register
        tbuf[0] = 0; //temp register address
        if( twi.writeRead(tbuf, rbuf) ){ //read temp
            v = (rbuf[0]<<8)|rbuf[1];
            // v = ((v * 9L)>>6) + 320; //convert to Fx10
            Debug("{Forange} temp: 0x%04X\n", v );
        }
    }

}
//========================
#endif


#if 0
//========================
//test 1
//testing twi problem, test with no bluetooth, 

//result- same- timeout because reading status register as 0
//although the LA shows a valid status register value
//but if add a nop to Twim::writeRead then works ok

using twi_ = Twim0< board.sda.pinNumber(),   
                    board.scl.pinNumber(), 
                    board.i2cDevicePwr.pinNumber() >;

Tmp117< twi_ > tmp;

for(;; nrf_delay_ms(5000) ){
    //init will power on, w/2ms delay time for startup
    tmp.init();
    //default is continuous conversion, 8 samples, 15.5ms*8 = 124ms
    nrf_delay_ms(130);
    //poll for data ready (up to 5ms)
    auto i = 10; //500us * 10 = 5ms timeout
    for( ; i; i-- ){
        if( tmp.isDataReady() ) break;
        nrf_delay_us(500); 
    }
    int16_t t = 0;
    if( i and not tmp.tempRaw(t) ) t = -32768;
    tmp.deinit(); //turn off power to ic

    DebugFuncHeader();
    if( i == 0 ) {
        Debug("  timeout\n");
    } else if( t == -32768) {
        Debug("  failed to read, or returned default temp value\n");
    } else {
        Debug("  tmp117 raw: %d  F: %d\n", t, tmp.x10F( t ) );
    }
}
//========================
#endif
                            //start power management
    Debug( "nrf_pwr_mgmt_init()...\n" );
    error.check( nrf_pwr_mgmt_init() );
                            //enable REG1 Dc-Dc (instead of LDO, for 1.8v system)
    nrf_power_dcdcen_set( true );

    Debug( "ble.init()...\n" );
    ble.init();             //ble stack init

    ////// now can use sd_* functions //////

    Debug( "adv.init()...\n" );
    adv.init();             //advertising init

    while( true ) { 
        DebugFuncHeader();
        Debug("{Forange}  low power...{Fwhite} \n");
        nrf_pwr_mgmt_run();  
    }

}

