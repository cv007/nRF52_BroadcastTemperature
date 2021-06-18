/*-----------------------------------------------------------------------------
    
nRF52840 Dongle   
and now a BL651 module (nRF52810) 

advertise temperature, with the bluetooth name as follows-
    77.5F NoName
    -5.5F NoName
    -10.3F No Name
    (max 22 chars used)
    see below to change name

also advertise battery service data- 0-100%
    2.%%v
    <=2.00v = 0%, >=3.00v = 100% 


the name can be changed at power on by connecting within first couple minutes
of power on- 
    use nRF Connect, 
    connect, click Generic Access, click up arrow by Device Name,
    type in name (max 15 or 16 chars available for name)
    click send, then disconnect
    new name will be written to flash so if powered off name will be retained

    after disconnect (or timeout), can no longer connect so if need to change
    name it has to be done after a power on (remove/reinsert battery)


nRF52840 USB Dongle-

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


BL651 module (nRF52810)-

    uses a CR2 battery
    2 led's- green and red
    2 push button switches- 1 for reset, the other a user button
    a tmp117 temperature ic
    a si7051 temperature ic

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

//(none of these have constructors)
#include "Boards.hpp"       //provides inline class var 'board'
#include "Advertising.hpp"  //provides inline class var 'adv'
#include "Errors.hpp"       //provides inline class var 'error'
#include "Ble.hpp"          //provides inline class var 'ble'
#include "Conn.hpp"         //provides inline class var 'conn'
#include "Gap.hpp"          //provides inline class var 'gap'
#include "Power.hpp"        //provides inline class var 'power'
#include "Flash.hpp"        //provides inline class var 'flash'
#include "Print.hpp"


// TESTING
// checking all temperature sources to compare
// run every 20 seconds, each function Debug will show info
#if defined(NRF52810_BL651_TEMP) && 1
Timer timerTestTemp{
    20_sec, 
    [](void*){ 
        TemperatureInternal<1>::read();
        TemperatureTmp117<1>::read();
        TemperatureSi7051<1>::read(); 
    }, 
    timerTestTemp.REPEATED 
};
#endif





/*-----------------------------------------------------------------------------
    functions
-----------------------------------------------------------------------------*/
int main() {

    DebugRtt 
        << ANSI_NORMAL FG MEDIUM_PURPLE << endl
        << cdup('=',60) << endl
        << "\tBoot start..." << endl 
        << cdup('=',60) << endl << ANSI_NORMAL;

    board.init();           //init board pins
    board.alive();          //blink led's to show boot
    power.init();           //start power management
    flash.init();           //get name from flash
    ble.init();             //ble stack init
    ////// now can use sd_* functions //////
    gap.init();             //gap init
    conn.init();            //connection init
    adv.init();             //advertising init

    DebugRtt << clear
        << ANSI_NORMAL FG MEDIUM_PURPLE << endl
        << cdup('=',60) << endl
        << "\t...Boot end" << endl
        << cdup('=',60) << endl << ANSI_NORMAL;

    power.loop();           //power.sleep() loop

    //power.loop will not return

}

