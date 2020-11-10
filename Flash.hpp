#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include <cstring> //strlen

#include "nrf_sdh_ble.h"

#include "Errors.hpp" //error
#include "Print.hpp"

/*------------------------------------------------------------------------------
    Flash
    currently just allowing writing to last page before bootloader
    and writing only from start of last page

    will assume sd is enabled (from ble.init), then have to use use the sd
    functions to erase/write flash
------------------------------------------------------------------------------*/
struct Flash {

    private:

    //bootloader is at end of flash, so use page before (set in nRFconfig.hpp)
    SI char* fullname_{ reinterpret_cast<char*>(LAST_PAGE) };
    SCA lastPageFlash_{LAST_PAGE};
    SI char fullnameTemp_[32]{0};
    SI bool saveName_{false};


SA  dump            () {
                        Debug( "fullname flash values:\n    " );
                        for( auto i = 0; i < 32; i++ ){
                            Debug( "%02X", fullname_[i] );
                            if( (i == 15) or (i == 31) ){
                                Debug(" ");
                                for( auto j = i-15; j <= i; j++ ){
                                    char c = fullname_[j] > ' ' and fullname_[j] < 128 ? fullname_[j] : '.';
                                    Debug("%c", c);
                                }
                                Debug("\n    ");
                            }
                        }
                        Debug( "\n" );
                    }
                    //check if flash fullname is 0 terminated within 32 bytes
                    //if so, assume is a valid string 
                    //(at least it will have an end)
SA  fullnameValid   () {
                        for( auto i = 0; i <= 32; i++ ){
                            if( fullname_[i] == 0 ) return true;
                        }
                        return false;
                    }
SA  fullnameErased  () {
                        for( auto i = 0; i <= 32; i++ ){
                            if( fullname_[i] != 0xFF ) return false;
                        }
                        return true;
                    }

SA  sdErasePage     () { 
                        if( fullnameErased() ) return true;
                        Debug( "Flash::sdErasePage\n" );
                        dump();
                        u32 err = sd_flash_page_erase(lastPageFlash_/4096);
                        if( err != NRF_SUCCESS ) return false;
                        //cpu is halted (if code running from flash) during erase
                        //don't know when sd will start page erase, so will just
                        //allow enough time to pass and assume is enough time
                        nrf_delay_ms( 500 ); //tPAGEERASE is 85ms
                        dump();
                        if( fullname_[0] == 0xFF ) return true;
                        Debug( "    failed\n" );
                        return false;
                    }

SA  sdFlashWrite32  (const u32* vals, u16 valsN) { 
                        Debug( "Flash::sdFlashWrite32\n" );
                        u32 err = sd_flash_write((u32*)lastPageFlash_, vals, valsN );
                        Debug( "    return val: %u\n", err );
                        if( err != NRF_SUCCESS ) return false;
                        nrf_delay_ms( 200 ); //write word 41us*32max=1.3ms
                        if( *(u32*)lastPageFlash_ == vals[0] ){
                            dump();
                            return true;
                        }
                        Debug( "    failed\n" );
                        return false;                        
                    }

SA  saveName        () {
                        Debug( "Flash::saveName : %s\n", fullnameTemp_ );             
                        if( not nrf_sdh_is_enabled() ) return; //these functions use sd
                        if( not sdErasePage() ) return;
                        auto len = strlen(fullnameTemp_);
                        u32 tmp[32/4];
                        memset( (void*)tmp, 0, 32 ); //will provide zero terminator
                        memcpy( (void*)tmp, (void*)fullnameTemp_, len );
                        //will try again from readName() if failed
                        //seem to get BUSY error after page erase (even though that
                        //works ok and has done the erase)
                        saveName_ = not sdFlashWrite32( tmp, 32/4 );
                    }

    public:
                    //stored flash name to ram, or use default if not set
SA  init            () {
                        Debug( "Flash::init...\n" );  
                        if( fullnameValid() ){
                            //copy to ram (include 0 terminator)
                            memcpy( (void*)fullnameTemp_, (void*)fullname_, strlen(fullname_)+1 );
                        } else {
                            memcpy( (void*)fullnameTemp_, (void*)"NoName", strlen("NoName")+1 );
                        }
                        Debug( "    name: %s\n", fullnameTemp_ );
                    }

                    //truncated to 32chars including 0 terminator
                    //but only 15 or 16 can be used as setup in adv
                    //(which will do its own truncation)
SA  updateName      (const char* str) {
                        Debug( "Flash::updateName : %s\n", str );
                        auto len = strlen(str);
                        if( len > 31 ) len = 31;
                        memcpy( (void*)fullnameTemp_, (void*)str, len );
                        fullnameTemp_[len] = 0; //0 terminate
                        saveName_ = true;
                    }

SA  readName        () {
                        //was updated?, need to save in flash
                        if( saveName_ ) saveName();                        
                        return (const char*)fullnameTemp_;                       
                    }

};

//for all who include this file
inline Flash flash;

