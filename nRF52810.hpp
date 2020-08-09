#pragma once

#define NRF52810
//nRF52810.hpp

/*------------------------------------------------------------------------------
    PIN values for mcu - port[5], pin[4:0]
------------------------------------------------------------------------------*/
namespace GPIO {
    enum PIN {
        P0_0, P0_1, P0_2, P0_3, P0_4, P0_5, P0_6, P0_7,
        P0_8, P0_9, P0_10,P0_11,P0_12,P0_13,P0_14,P0_15,
        P0_16,P0_17,P0_18,P0_19,P0_20,P0_21,P0_22,P0_23,
        P0_24,P0_25,P0_26,P0_27,P0_28,P0_29,P0_30,P0_31
    };
}



/* 
Supported tx_power values: 
    -40dBm, -20dBm, -16dBm, -12dBm, -8dBm, -4dBm, 
    0dBm, +3dBm, +4dBm

S112
using 1-9 as power levels (and 0=0dbm)
*/
inline constexpr int8_t SD_TX_LEVELS[]{0,-40,-20,-16,-12,-8,-4,0,3,4};
