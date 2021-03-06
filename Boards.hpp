#pragma once

#include "nRFconfig.hpp"

#include "nrf_delay.h"

#include "Gpio.hpp"
#include "Print.hpp"

#ifdef  NRF52840 //for 52840 based boards
/*------------------------------------------------------------------------------
    Pca10059 (nrf52 dongle, nrf52840)
------------------------------------------------------------------------------*/
struct Pca10059 {

    //these pins are not init until init() is run
    //(could init each here, but do not want possible resulting constructor guard)
    SI Gpio<P0_6, LOWISON>  led1G;
    SI Gpio<P1_9, LOWISON>  led2G;
    SI Gpio<P0_12, LOWISON> led2B;
    SI Gpio<P0_8, LOWISON>  led2R;
    SI Gpio<P1_6, LOWISON>  sw1;

    //'generic' led names
    SCA &ledGreen2  { led1G };
    SCA &ledRed1    { led2R };
    SCA &ledGreen1  { led2G };
    SCA &ledBlue1   { led2B };
    SCA &ledRed     { led2R };
    SCA &ledGreen   { led2G };
    SCA &ledBlue    { led2B };

            //someone is required to run init to setup pins
SA  init    () {
                DebugRtt << "Pca10059::init..." << endl;
                // Debug( "Pca10059::init...\n" );
                led1G.init( OUTPUT );
                led2G.init( OUTPUT );
                led2R.init( OUTPUT );
                led2B.init( OUTPUT );
                sw1.init( INPUT, PULLUP );
            }

            //signal board is alive
SA  alive   () {
                led1G.blinkN(1,25,25,200);
                led2R.blinkN(1,25,25,200);
                led2G.blinkN(1,25,25,200);
                led2B.blinkN(1,25,25,200);
            }

            //show error code via led's, skip leading 0's
            //show 1-15 blinks for each nibble (0x01-0x0f)
            //a zero will be a short blue blink
SA  error   (u16 hex) {
                bool lz = true;
                for( auto i = 12; i >= 0; i -= 4 ){
                    u8 v = (hex>>i) bitand 0xf;
                    if( v == 0 and lz == true ) continue; //skip leading 0's
                    lz = false;
                    if( v ) ledRed1.blinkN( v, 500 );
                    else ledBlue1.blinkN( 1, 50 ); //0 is 1 short blue blink
                    nrf_delay_ms( 500 );
                }
            }

            //show a caution blink
SA  caution (u16 ms = 1) {
                ledRed1.blinkN( 1, ms );
            }

            //show an ok blink
SA  ok      (u16 ms = 1) {
                ledGreen1.blinkN( 1, ms );
            }

};

#endif //NRF52840

#ifdef NRF52810
/*------------------------------------------------------------------------------
    Laird module w/nrf52810
------------------------------------------------------------------------------*/
struct BL651tempBoard {

    //SDA - P0_13 - input, S0D1
    //SCL - P0_15 - input, SOD1
    //PWR - P0_17 - output, power to i2c devices tmp117/si7051
    Gpio<P0_13>  sda; 
    Gpio<P0_15>  scl;
    Gpio<P0_17>  i2cDevicePwr; 

//taken care of in Twim
// SA  i2cInit () {
//                 sda.init( INPUT, S0D1, PULLUP );
//                 scl.init( INPUT, S0D1, PULLUP );
//                 i2cDevicePwr.init( OUTPUT, S0H1 );
//             }
// SA  i2cDeinit () {
//                 sda.init(); //all default values
//                 scl.init();
//                 i2cDevicePwr.init();
//             }

    //these pins are not init until init() is run
    SI Gpio<P0_7>  ledRed; //board label 1
    SI Gpio<P0_8>  ledGreen; //board label 2
    SI Gpio<P0_27, LOWISON>  sw1; //SW1

    //alias for other led names already in use
    SCA &ledGreen2  { ledGreen };
    SCA &ledRed1    { ledRed };

            //someone is required to run init to setup pins
SA  init    () {
                DebugRtt << "BL651tempBoard::init..." << endl;
                ledRed.init( OUTPUT, S0S1 ); //standard drive 1
                ledGreen.init( OUTPUT, S0S1 ); //standard drive 1
                sw1.init( INPUT, PULLUP );
            }

            //signal board is alive
SA  alive   () {
                ledGreen.blinkN(2,50,50,200);
                ledRed.blinkN(2,50,50,200);
            }

            //show error code via led's, skip leading 0's
            //show 1-15 blinks for each nibble (0x01-0x0f)
            //a zero will be a short green blink
SA  error   (u16 hex) {
                bool lz = true;
                for( auto i = 12; i >= 0; i -= 4 ){
                    u8 v = (hex>>i) bitand 0xf;
                    if( v == 0 and lz == true ) continue; //skip leading 0's
                    lz = false;
                    if( v ) ledRed.blinkN( v, 500 ); //v times, 500ms on, 0ms off, no post delay
                    else ledGreen.blinkN( 1, 50 ); //0 is 1 short green blink
                    nrf_delay_ms( 500 );
                }
            }

            //show a caution blink
SA  caution (u16 ms = 5) {
                ledRed.blinkN( 1, ms );
            }

            //show an ok blink
SA  ok      (u16 ms = 5) {
                ledGreen.blinkN( 1, ms );
            }

};

#endif


//choose board in nrfConfig.hpp
//someone will need to run board.init()


#ifdef NRF52840_DONGLE
inline Pca10059 board;
#endif

#ifdef NRF52810_BL651_TEMP
inline BL651tempBoard board;
#endif
