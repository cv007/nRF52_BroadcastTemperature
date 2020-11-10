#pragma once

#include "nRFconfig.hpp"

#include "nrf_sdh.h"    //reset via sd

#include "Boards.hpp"   //board
#include "Print.hpp"

#undef SA
#define SA [[ gnu::noinline ]] static auto

/*------------------------------------------------------------------------------
    Errors - check return results, if not NRF_SUCCESS send error to board for
             some kind of output ( led's ), optional reboot (default is reboot)
------------------------------------------------------------------------------*/
struct Errors {

                //if error, show error code 3 times, 
                //reset unless also pass in false
SA  check       (i16 err, bool reboot = true) {
                    if( err == 0 ) return;
                    Debug( "{Fred}Error: %d{normal}\n", err );
                    for( auto i = 0; i < 3; i++ ){
                        board.error( err ); //let board put out error codes however it wants
                        nrf_delay_ms(3000);
                    }
                    if( reboot ) sd_nvic_SystemReset();
                }

};

#undef SA
#define SA static auto


//for all who include this file
inline Errors error; 