// ------------------------------------------------------------------
// Lamp/Group - include file
// ------------------------------------------------------------------
// Author:    nlv10677
// Copyright: NXP B.V. 2014. All rights reserved
// ------------------------------------------------------------------

teZcbStatus lmpgrp_OnOff(uint16_t u16ShortAddress, uint16_t u16GroupAddress, uint8_t u8Mode);

teZcbStatus lmpgrp_MoveToLevel(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                               uint8_t u8Level, uint16_t u16TransitionTime);

teZcbStatus lmpgrp_MoveToHue(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                             uint8_t u8Hue, uint16_t u16TransitionTime);

teZcbStatus lmpgrp_MoveToSaturation(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                                    uint8_t u8Saturation, uint16_t u16TransitionTime);


teZcbStatus lmpgrp_MoveToHueSaturation(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                      uint8_t u8Hue, uint8_t u8Saturation, uint16_t u16TransitionTime);

teZcbStatus lmpgrp_MoveToColour(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
                                uint16_t u16X, uint16_t u16Y, uint16_t u16TransitionTime);

teZcbStatus lmpgrp_MoveToColourTemperature(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
              uint16_t u16ColourTemperature, uint16_t u16TransitionTime);

teZcbStatus lmpgrp_MoveColourTemperature(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
               uint8_t u8Mode, uint16_t u16Rate,
               uint16_t u16ColourTemperatureMin, uint16_t u16ColourTemperatureMax);

teZcbStatus lmpgrp_ColourLoopSet(uint16_t u16ShortAddress, uint16_t u16GroupAddress,
               uint8_t u8UpdateFlags, uint8_t u8Action, uint8_t u8Direction,
               uint16_t u16Time, uint16_t u16StartHue);

// ------------------------------------------------------------------
// END OF FILE
// ------------------------------------------------------------------

