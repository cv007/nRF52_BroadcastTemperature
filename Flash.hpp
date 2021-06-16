#pragma once

/*-----------------------------------------------------------------------------
    includes
-----------------------------------------------------------------------------*/
#include "nRFconfig.hpp"

#include <cstring> //strlen

#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "Errors.hpp" //error
#include "Print.hpp"


/*------------------------------------------------------------------------------
    Flash
    currently just allowing writing to last page before bootloader
    and writing only first 32 bytes from start of last page for device name
    storage

    will assume sd is enabled (from ble.init), so have to use use the sd
    functions to erase/write flash
------------------------------------------------------------------------------*/
struct Flash {

    private:

    //bootloader is at end of flash, so use page before (set in nRFconfig.hpp)
    SI char* fullnameFlash_{ reinterpret_cast<char*>(LAST_PAGE_ADDR) };
    SCA fullnameSiz_{32};
    SCA lastPageFlash_{LAST_PAGE};
    SI char fullnameRam_[fullnameSiz_]{0};
    SI bool saveName_{false};
    SI bool busy_{false};


                    //sd soc event handler to get success/error from erase/write
SA  evtHandler      (u32 evtId, void* ctx) -> void {
                        DebugRtt << "Flash::handler event : " << evtId << endl;
                        if( evtId == NRF_EVT_FLASH_OPERATION_SUCCESS ){
                            DebugRtt << "    success" << endl;
                            dump();
                            busy_ = false;
                        }
                        if( evtId == NRF_EVT_FLASH_OPERATION_ERROR ){
                            DebugRtt << "    error" << endl;
                            busy_ = false;
                        }
                    }
                    //register handler, section .sdh_soc_observers##Priority
                    [[ using gnu : section(".sdh_soc_observers0"), used ]]
                    SI nrf_sdh_soc_evt_observer_t observer_ = { evtHandler, NULL };


                    //dump 32 bytes
SA  dump            () -> void {
                        DebugRtt << "fullname flash values:" << endl << "    ";
                        for( auto i = 0; i < fullnameSiz_; i++ ){
                            DebugRtt << setw(2) << setfill('0') << fullnameFlash_[i];
                            if( (i bitand 15) == 15 ){
                                DebugRtt << " ";
                                for( auto j = i-15; j <= i; j++ ){
                                    DebugRtt << (fullnameFlash_[j] > ' ' and fullnameFlash_[j] < 128 ? 
                                                fullnameFlash_[j] : '.');  
                                }
                                DebugRtt << endl;
                            }
                        }
                        DebugRtt << endl;
                    }
                    //check if flash fullname is 0 terminated within 32 bytes
                    //if so, assume is a valid string 
                    //(at least it will have an end)
SA  fullnameValid   () {
                        for( auto i = 0; i < fullnameSiz_; i++ ){
                            if( fullnameFlash_[i] == 0 ) return true;
                        }
                        return false;
                    }
SA  fullnameErased  () {
                        for( auto i = 0; i < fullnameSiz_; i++ ){
                            if( fullnameFlash_[i] != 0xFF ) return false;
                        }
                        return true;
                    }

                    //returns true if already erased or a page erase was accepted by sd
SA  sdErasePage     () { 
                        if( fullnameErased() ) return true;
                        DebugRtt << "Flash::sdErasePage" << endl;
                        if( busy_ ){
                            DebugRtt << "    flash busy" << endl;
                            return false;
                        } 
                        dump();
                        u32 err = sd_flash_page_erase(lastPageFlash_);
                        if( err != NRF_SUCCESS ) return false;
                        busy_ = true;
                        return true;
                    }

                    //returns true if write was accepted by sd
SA  sdFlashWrite32  (const u32* vals, u16 valsN) { 
                        DebugRtt << "Flash::sdFlashWrite32" << endl;
                        if( busy_ ){
                            DebugRtt << "    flash busy" << endl;
                            return false;
                        } 
                        u32 err = sd_flash_write((u32*)fullnameFlash_, vals, valsN );
                        DebugRtt << "    return val: " << err << endl;
                        if( err != NRF_SUCCESS ) return false;
                        busy_ = true;
                        return true;                       
                    }

SA  saveName        () {
                        DebugRtt << "Flash::saveName : " << fullnameRam_ << endl; 
                        if( not nrf_sdh_is_enabled() ) return; //these functions use sd
                        if( not sdErasePage() ) return;
                        //will try again from readName() if write is not accepted by sd
                        saveName_ = not sdFlashWrite32( (const u32*)fullnameRam_, fullnameSiz_/4 );
                    }

    public:
                    //stored flash name to ram, or use default if not set
SA  init            () {
                        DebugRtt << "Flash::init..." << endl; 
                        if( fullnameValid() ){
                            //copy to ram (include 0 terminator)
                            memcpy( (void*)fullnameRam_, (void*)fullnameFlash_, strlen(fullnameFlash_)+1 );
                        } else {
                            memcpy( (void*)fullnameRam_, (void*)"NoName", strlen("NoName")+1 );
                        }
                        DebugRtt << "    name: " << fullnameRam_ << endl;
                    }

                    //truncated to 32chars including 0 terminator
                    //but only 15 or 16 can be used as setup in adv
                    //(which will do its own truncation)
SA  updateName      (const char* str) {
                        DebugRtt << "Flash::updateName : " << str << endl;
                        auto len = strlen(str);
                        if( len > fullnameSiz_-1 ) len = fullnameSiz_-1;
                        memset( (void*)fullnameRam_, 0, fullnameSiz_ ); //clear all
                        memcpy( (void*)fullnameRam_, (void*)str, len );
                        //0 terminated since was cleared
                        saveName_ = true;
                    }

SA  readName        () {
                        //was updated?, need to save in flash
                        if( saveName_ ) saveName();                        
                        return (const char*)fullnameRam_;                       
                    }

};

//for all who include this file
inline Flash flash;

