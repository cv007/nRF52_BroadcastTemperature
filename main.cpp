/*-----------------------------------------------------------------------------
    
nRF52840 Dongle   
and now a BL651 module (nRF52810) 

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


BL651 module (nRF52810) 

uses a CR2 battery
2 led's- green and red
2 push button switches- 1 for reset, the other a user button
a tmp117 temperature ic
a si7051 temperature ic

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

#if 1
/*
testing twi - bypass Tmp117 class, use Twi directly, no bluetooth

problem shows up in optimization- -O1,-O2,-O3 
-Os always ok, or if Ywi::writeRead is made noinline

run loop 1 time

terminal print-
    status: 0x0000     (should be 0x2220)
    temp: 0x0000       (shoud be in 0x0D?? range)

    logic analyzer shows correct data on the twi pins

    Debug prints status as 0x0000, but then passes the (rbuf[0] bitand 0x20) test
        so value is 0x2220 at some point after the Debug print, then the read
        of temp works but the rbuf values are still 0 (which were set to 0 before using)

    with status Debug commented out, will not pass the (rbuf[0] bitand 0x20) test
        so now ready bit is not seen as set
    status ready bit not set


    with adding nop #1 uncommented, passes ready bit test but temp read value is 0x0000
    status: 0x2220
    temp: 0x0000

    with adding nop #2 uncommented, prints correct temp value

    so each uncommented nop seems to make reading the rbuf values ok
    status: 0x2220
    temp: 0x0D64

    also makes no difference if Debug is disabled (uses RTT), the led's will
    still indicate an error

*/

Twim0< board.sda.pinNumber(),   
       board.scl.pinNumber(), 
       board.i2cDevicePwr.pinNumber() >twi;

int main(){

    board.init();           //init board pins
    board.alive();          //blink led's to show boot

    Debug("{normal}\n\n");

    twi.init( 0x48 ); //tmp117=0x48, default 100kHz
    //pins now setup, 2ms powerup delay

    //tmp117 default is continuous conversion at 1s interval
    //8 sample average - first result ready in 124ms

    for(auto n = 1; ;){ //do n times

        nrf_delay_ms(5000);

        //makes no difference if these are global vars or here on the stack
        uint8_t tbuf[1]{ 1 };   //status register address
        uint8_t rbuf[2]{ 0,0 }; //status register read value, 2bytes

        if( not twi.writeRead( tbuf, rbuf ) ){
            Debug("writeRead failed\n");
            board.ledRed.on();
            continue;
        }
        //no errors and write/read amounts match requested amounts

asm("nop"); //#1
        
        Debug("status: 0x%02X%02X\n", rbuf[0], rbuf[1]); //big endian

        //check ready bit
        if( not (rbuf[0] bitand 0x20) ){
            Debug("status ready bit not set\n");
            board.ledRed.on();
            continue;
        }

        tbuf[0] = 0; //temp register address 
        rbuf[0] = 0; rbuf[1] = 0; //clear rbuf (so can easily see if changed)

        if( not twi.writeRead( tbuf, rbuf ) ){
            Debug("writeRead failed\n");
            board.ledRed.on();
            continue;
        }

asm("nop"); //#2

        Debug("temp: 0x%02X%02X\n", rbuf[0], rbuf[1]); //big endian

        //assume not 0 is ok
        if( (rbuf[0] != 0) and (rbuf[1] != 0) ) board.ledGreen.on(); 
        else board.ledRed.on();

        //run n times
        if( not --n ) for(;;){}
    }

} //main
#endif

//normal
#if 0
int main() {

    Debug( "{normal}{Fgreen}\nBoot...\n" );
    Debug( "{Fmagenta}board.init()...\n" );
    board.init();           //init board pins
    board.alive();          //blink led's to show boot

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
#endif
