#ifndef  _MEMC_H
#define  _MEMC_H

////////////////////////////////////////////////////////////////////////////////////////
//
// memc.h: the memory card controller
//
// The card is 262144 words (1 MiB) behind a 8-byte "V32-MEMC" signature in its file.
// The controller keeps the file in step with card memory the way the DesktopEmulator
// does, and (when the card is formatted as one) keeps the v32dev mailbox live:
//
//   * every write the program makes marks its 256-word page dirty
//   * at the end of each frame the dirty pages are written to the file
//   * with a live mailbox, at the start of each frame the host-owned area is read
//     back from the file (a host process writes there); the header and that area are
//     never written to the file, and each cart-owned part's 16 control ("commit")
//     words are written after all data, so a reader that sees a new ring index also
//     sees the data it announces
//
// See docs/v32dev/PROTOCOL.md of the v32io project, section 2, for the protocol.
//
////////////////////////////////////////////////////////////////////////////////////////

#define  MEMC_PAGE_SHIFT         8
#define  MEMC_PAGE_WORDS         (1 << MEMC_PAGE_SHIFT)
#define  MEMC_NUM_PAGES          (MEMC_WORDS / MEMC_PAGE_WORDS)

////////////////////////////////////////////////////////////////////////////////////////
//
// v32dev mailbox geometry (words, relative to the start of the card)
//
#define  DEV_REGION_WORDS        16384
#define  DEV_REGION_START        (MEMC_WORDS - DEV_REGION_WORDS)       // 245760
#define  DEV_HEADER_WORDS        64
#define  DEV_CHANNELS            4
#define  DEV_PART_WORDS          2040
#define  DEV_CONTROL_WORDS       16
#define  DEV_RING_WORDS          (DEV_PART_WORDS - DEV_CONTROL_WORDS)  // 2024
#define  DEV_HOST_START          (DEV_REGION_START + DEV_HEADER_WORDS) // 245824
#define  DEV_HOST_WORDS          (DEV_CHANNELS * DEV_PART_WORDS)       // 8160
#define  DEV_CART_START          (DEV_HOST_START + DEV_HOST_WORDS)     // 253984

#define  DEV_MAGIC               0x31564544                            // "DEV1"
#define  DEV_VERSION             1
#define  DEV_HOST_STILL          120      // frames without a host heartbeat = gone

// header words (relative to DEV_REGION_START)
#define  DEV_H_MAGIC             0
#define  DEV_H_VERSION           1
#define  DEV_H_REGION_WORDS      2
#define  DEV_H_CHANNELS          3
#define  DEV_H_PART_WORDS        4
#define  DEV_H_CONTROL_WORDS     5
#define  DEV_H_EMU_FLAGS         8        // emulator-owned, memory only
#define  DEV_H_EMU_BEAT          9        // emulator-owned, memory only

// control words of a host part (relative to the part)
#define  DEV_HP_TYPE             0
#define  DEV_HP_FLAGS            1
#define  DEV_HP_BEAT             2
#define  DEV_HP_H2C_HEAD         3
#define  DEV_HP_C2H_TAIL         4
#define  DEV_HP_SESSION          5
#define  DEV_HP_STATUS           6
#define  DEV_HP_DESC             8

// control words of a cart part (relative to the part)
#define  DEV_CP_FLAGS            0
#define  DEV_CP_C2H_HEAD         1
#define  DEV_CP_H2C_TAIL         2
#define  DEV_CP_BEAT             3
#define  DEV_CP_SESSION          4
#define  DEV_CP_STATUS           5
#define  DEV_CP_DESC             8

////////////////////////////////////////////////////////////////////////////////////////
//
// memc.c
//
void      memc_attach        (const char *);   // the card was loaded from this file
void      memc_detach        (void);           // flush and forget the card
void      memc_touch         (uint32_t);       // program wrote card word N
void      memc_touch_all     (void);           // everything must reach the file
void      memc_flush         (void);           // dirty pages -> file (two phases)
void      memc_save          (void);           // flush, at exit / unload / crash
void      memc_power_on      (void);           // first frame's mailbox sync
void      memc_frame         (void);           // frame boundary: flush, then sync
uint8_t   memc_attached      (void);
uint8_t   memc_mailbox       (void);           // is the mailbox live?
void      memc_region_get    (uint32_t *);     // copy the mailbox region out
void      memc_region_put    (const uint32_t *);
void      memc_show          (int);            // 'mailbox' command: -1 = overview

#endif
