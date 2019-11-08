// ------------------------------------------------------------------
// Lamp/Group
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

/** \addtogroup zb
 * \file
 * \brief Lamp/group helpers
 */

#include "zcb.h"
#include "SerialLink.h"

// ------------------------------------------------------------------
// Macros
// ------------------------------------------------------------------

#define LMPGRP_DEBUG

#ifdef LMPGRP_DEBUG
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif /* LMPGRP_DEBUG */

// ------------------------------------------------------------------
// Message helper
// ------------------------------------------------------------------

static void initDefaultHeader( void * mess,
            uint16_t u16ShortAddress, uint16_t u16GroupAddress ) {
    
    typedef struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
    } __attribute__((__packed__)) sDefaultMessage;

    sDefaultMessage * message = (sDefaultMessage *)mess;
    message->u8SourceEndpoint = ZB_ENDPOINT_ZHA;
    
    if (u16ShortAddress != 0xFFFF) {
        message->u8TargetAddressMode   = E_ZB_ADDRESS_MODE_SHORT;
        message->u16TargetAddress      = htons(u16ShortAddress);
        
        message->u8DestinationEndpoint = ZB_ENDPOINT_LAMP;
    } else {
        message->u8TargetAddressMode   = E_ZB_ADDRESS_MODE_GROUP;
        message->u16TargetAddress      = htons(u16GroupAddress );
        message->u8DestinationEndpoint = ZB_DEFAULT_ENDPOINT_ZLL;
    }
}

// ------------------------------------------------------------------
// Exported Functions
// ------------------------------------------------------------------

teZcbStatus lmpgrp_OnOff(uint16_t u16ShortAddress, uint16_t u16GroupAddress, uint8_t u8Mode) {
    uint8_t u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Mode;
    } __attribute__((__packed__)) sOnOffMessage;
    
    DEBUG_PRINTF( "On/Off (Set Mode=%d) to 0x%04x/0x%04x\n", u8Mode,
         u16ShortAddress, u16GroupAddress );
    
    if (u8Mode > 2) {
        /* Illegal value */
        return E_ZCB_ERROR;
    }
    
    initDefaultHeader( &sOnOffMessage, u16ShortAddress, u16GroupAddress );

    sOnOffMessage.u8Mode = u8Mode;
    
    if (eSL_SendMessage(E_SL_MSG_ONOFF, sizeof(sOnOffMessage), &sOnOffMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_MoveToLevel(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                               uint8_t u8Level, uint16_t u16TransitionTime) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8OnOff;
        uint8_t     u8Level;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sLevelMessage;
    
    DEBUG_PRINTF( "Set Level %d to 0x%04x/0x%04x\n", u8Level,
             u16ShortAddress, u16GroupAddress );
    
    if (u8Level > 254) {
        u8Level = 254;
    }
    
    initDefaultHeader( &sLevelMessage, u16ShortAddress, u16GroupAddress );

    sLevelMessage.u8OnOff               = 0; // we don't allow automatic changes to on or off
    sLevelMessage.u8Level               = u8Level;
    sLevelMessage.u16TransitionTime     = htons(u16TransitionTime);
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_LEVEL_ONOFF, sizeof(sLevelMessage),
                  &sLevelMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_MoveToHue(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                             uint8_t u8Hue, uint16_t u16TransitionTime) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Hue;
        uint8_t     u8Direction;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToHueMessage;
    
    DEBUG_PRINTF( "Set Hue %d to 0x%04x/0x%04x\n", u8Hue,
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sMoveToHueMessage, u16ShortAddress, u16GroupAddress );

    sMoveToHueMessage.u8Hue               = u8Hue;
    sMoveToHueMessage.u8Direction         = 0;
    sMoveToHueMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE, sizeof(sMoveToHueMessage),
              &sMoveToHueMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_MoveToSaturation(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                                    uint8_t u8Saturation, uint16_t u16TransitionTime) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Saturation;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToSaturationMessage;
    
    DEBUG_PRINTF( "Set Saturation %d to 0x%04x/0x%04x\n", u8Saturation,
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sMoveToSaturationMessage, u16ShortAddress, u16GroupAddress );

    sMoveToSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_SATURATION, sizeof(sMoveToSaturationMessage),
             &sMoveToSaturationMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}



teZcbStatus lmpgrp_MoveToHueSaturation(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                      uint8_t u8Hue, uint8_t u8Saturation, uint16_t u16TransitionTime) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Hue;
        uint8_t     u8Saturation;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToHueSaturationMessage;
    
    DEBUG_PRINTF( "Set Hue %d, Saturation %d to 0x%04x/0x%04x\n", u8Hue, u8Saturation,
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sMoveToHueSaturationMessage, u16ShortAddress, u16GroupAddress );

    sMoveToHueSaturationMessage.u8Hue               = u8Hue;
    sMoveToHueSaturationMessage.u8Saturation        = u8Saturation;
    sMoveToHueSaturationMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_HUE_SATURATION, sizeof(sMoveToHueSaturationMessage),
             &sMoveToHueSaturationMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_MoveToColour(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                                uint16_t u16X, uint16_t u16Y, uint16_t u16TransitionTime) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16X;
        uint16_t    u16Y;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToColourMessage;
    
    DEBUG_PRINTF( "Set X %d, Y %d to 0x%04x/0x%04x\n", u16X, u16Y,
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sMoveToColourMessage, u16ShortAddress, u16GroupAddress );

    sMoveToColourMessage.u16X                = htons(u16X);
    sMoveToColourMessage.u16Y                = htons(u16Y);
    sMoveToColourMessage.u16TransitionTime   = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR, sizeof(sMoveToColourMessage),
              &sMoveToColourMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_MoveToColourTemperature(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
              uint16_t u16ColourTemperature, uint16_t u16TransitionTime) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint16_t    u16ColourTemperature;
        uint16_t    u16TransitionTime;
    } __attribute__((__packed__)) sMoveToColourTemperatureMessage;
    
    DEBUG_PRINTF( "Set colour temperature %d to 0x%04x/0x%04x\n", u16ColourTemperature,
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sMoveToColourTemperatureMessage, u16ShortAddress, u16GroupAddress );

    sMoveToColourTemperatureMessage.u16ColourTemperature    = htons(u16ColourTemperature);
    sMoveToColourTemperatureMessage.u16TransitionTime       = htons(u16TransitionTime);

    if (eSL_SendMessage(E_SL_MSG_MOVE_TO_COLOUR_TEMPERATURE, sizeof(sMoveToColourTemperatureMessage),
         &sMoveToColourTemperatureMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_MoveColourTemperature(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
               uint8_t u8Mode, uint16_t u16Rate,
               uint16_t u16ColourTemperatureMin, uint16_t u16ColourTemperatureMax) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8Mode;
        uint16_t    u16Rate;
        uint16_t    u16ColourTemperatureMin;
        uint16_t    u16ColourTemperatureMax;
    } __attribute__((__packed__)) sMoveColourTemperatureMessage;
    
    DEBUG_PRINTF( "Move colour temperature to 0x%04x/0x%04x\n",
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sMoveColourTemperatureMessage, u16ShortAddress, u16GroupAddress );

    sMoveColourTemperatureMessage.u8Mode                    = u8Mode;
    sMoveColourTemperatureMessage.u16Rate                   = htons(u16Rate);
    sMoveColourTemperatureMessage.u16ColourTemperatureMin   = htons(u16ColourTemperatureMin);
    sMoveColourTemperatureMessage.u16ColourTemperatureMax   = htons(u16ColourTemperatureMax);
    
    if (eSL_SendMessage(E_SL_MSG_MOVE_COLOUR_TEMPERATURE, sizeof(sMoveColourTemperatureMessage),
              &sMoveColourTemperatureMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


teZcbStatus lmpgrp_ColourLoopSet(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
               uint8_t u8UpdateFlags, uint8_t u8Action, uint8_t u8Direction,
               uint16_t u16Time, uint16_t u16StartHue) {
    uint8_t             u8SequenceNo;
    
    struct {
        uint8_t     u8TargetAddressMode;
        uint16_t    u16TargetAddress;
        uint8_t     u8SourceEndpoint;
        uint8_t     u8DestinationEndpoint;
        uint8_t     u8UpdateFlags;
        uint8_t     u8Action;
        uint8_t     u8Direction;
        uint16_t    u16Time;
        uint16_t    u16StartHue;
    } __attribute__((__packed__)) sColourLoopSetMessage;
    
    DEBUG_PRINTF( "Colour loop set to 0x%04x/0x%04x\n",
             u16ShortAddress, u16GroupAddress );
    
    initDefaultHeader( &sColourLoopSetMessage, u16ShortAddress, u16GroupAddress );

    sColourLoopSetMessage.u8UpdateFlags             = u8UpdateFlags;
    sColourLoopSetMessage.u8Action                  = u8Action;
    sColourLoopSetMessage.u8Direction               = u8Direction;
    sColourLoopSetMessage.u16Time                   = htons(u16Time);
    sColourLoopSetMessage.u16StartHue               = htons(u16StartHue);
    
    if (eSL_SendMessage(E_SL_MSG_COLOUR_LOOP_SET, sizeof(sColourLoopSetMessage),
            &sColourLoopSetMessage, &u8SequenceNo) != E_SL_OK) {
        return E_ZCB_COMMS_FAILED;
    }
    
    return E_ZCB_OK;
}


// ------------------------------------------------------------------
// END OF FILE
// ------------------------------------------------------------------

