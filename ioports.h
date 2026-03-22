#ifndef  _IOPORTS_H
#define  _IOPORTS_H

#define  TIM_CurrentDate                   0x000
#define  TIM_CurrentTime                   0x001
#define  TIM_FrameCounter                  0x002
#define  TIM_CycleCounter                  0x003
#define  RNG_CurrentValue                  0x100
#define  GPU_Command                       0x200
#define  GPU_RemainingPixels               0x201
#define  GPU_ClearColor                    0x202
#define  GPU_MultiplyColor                 0x203
#define  GPU_ActiveBlending                0x204
#define  GPU_SelectedTexture               0x205
#define  GPU_SelectedRegion                0x206
#define  GPU_DrawingPointX                 0x207
#define  GPU_DrawingPointY                 0x208
#define  GPU_DrawingScaleX                 0x209
#define  GPU_DrawingScaleY                 0x20A
#define  GPU_DrawingAngle                  0x20B
#define  GPU_RegionMinX                    0x20C
#define  GPU_RegionMinY                    0x20D
#define  GPU_RegionMaxX                    0x20E
#define  GPU_RegionMaxY                    0x20F
#define  GPU_RegionHotspotX                0x210
#define  GPU_RegionHotspotY                0x211
#define  SPU_Command                       0x300
#define  SPU_GlobalVolume                  0x301
#define  SPU_SelectedSound                 0x302
#define  SPU_SelectedChannel               0x303
#define  SPU_SoundLength                   0x304
#define  SPU_SoundPlayWithLoop             0x305
#define  SPU_SoundLoopStart                0x306
#define  SPU_SoundLoopEnd                  0x307
#define  SPU_ChannelState                  0x308
#define  SPU_ChannelAssignedSound          0x309
#define  SPU_ChannelVolume                 0x30A
#define  SPU_ChannelSpeed                  0x30B
#define  SPU_ChannelLoopEnabled            0x30C
#define  SPU_ChannelPosition               0x30D
#define  INP_SelectedGamepad               0x400
#define  INP_GamepadConnected              0x401
#define  INP_GamepadLeft                   0x402
#define  INP_GamepadRight                  0x403
#define  INP_GamepadUp                     0x404
#define  INP_GamepadDown                   0x405
#define  INP_GamepadButtonStart            0x406
#define  INP_GamepadButtonA                0x407
#define  INP_GamepadButtonB                0x408
#define  INP_GamepadButtonX                0x409
#define  INP_GamepadButtonY                0x40A
#define  INP_GamepadButtonL                0x40B
#define  INP_GamepadButtonR                0x40C
#define  CAR_Connected                     0x500
#define  CAR_ProgramROMSize                0x501
#define  CAR_NumberOfTextures              0x502
#define  CAR_NumberOfSounds                0x503
#define  MEM_Connected                     0x600

#define  GPUCommand_ClearScreen            0x10
#define  GPUCommand_DrawRegion             0x11
#define  GPUCommand_DrawRegionZoomed       0x12
#define  GPUCommand_DrawRegionRotated      0x13
#define  GPUCommand_DrawRegionRotozoomed   0x14

#endif
