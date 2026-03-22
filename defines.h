#ifndef  __DEFINES_H
#define  __DEFINES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <getopt.h>
#include <time.h>
#include "globals.h"
#include "ioports.h"

#define  BIOS_DEFAULT_PATH       "/usr/local/Vircon32/Emulator/Bios/StandardBios.v32"

#define  NUM_MEMORY_PAGES        4
#define  V32_PAGE_RAM            0
#define  V32_PAGE_BIOS           1
#define  V32_PAGE_CART           2
#define  V32_PAGE_MEMC           3
#define  V32_PAGE_MASK           0x30000000

#define  V32_BIOS_TEXTURES       1
#define  V32_CART_TEXTURES       256
#define  V32_REGIONS_PER_TEXTURE 4096

#define  RAM_FIRST_ADDR          0x00000000
#define  BIOS_FIRST_ADDR         0x10000000
#define  CART_FIRST_ADDR         0x20000000
#define  MEMC_FIRST_ADDR         0x30000000

#define  RAM_FINAL_ADDR          0x003FFFFF
#define  BIOS_FINAL_ADDR         0x100FFFFF
#define  CART_FINAL_ADDR         0x27FFFFFF
#define  MEMC_FINAL_ADDR         0x3003FFFF

#define  NO_CART_ERROR           1
#define  REGEX_COMPILE_ERROR     2
#define  REGEX_EXECUTE_ERROR     3

#define  IOPORTS_ALLOC_FAIL      4
#define  IOPORTS_ACCESS_ERROR    5
#define  IOPORTS_BAD_PORT        6

#define  MEMORY_ALLOC_FAIL       7
#define  MEMORY_READ_ERROR       8
#define  MEMORY_WRITE_ERROR      9
#define  MEMORY_BAD_ACCESS       10

#define  FILE_OPEN_ERROR         11
#define  FILE_POSITION_ERROR     12

#define  STRING_ALLOC_FAIL       13
#define  DATA_ALLOC_FAIL         14
#define  LIST_ALLOC_FAIL         15

#define  BIOS_START_OFFSET       0x10000004
#define  BIOS_ERROR_OFFSET       0x10000000
#define  CART_START_OFFSET       0x20000000

#define  ERROR_NONE             -1
#define  ERROR_MEMORY_READ       0
#define  ERROR_MEMORY_WRITE      1
#define  ERROR_PORT_READ         2
#define  ERROR_PORT_WRITE        3
#define  ERROR_STACK_OVERFLOW    4
#define  ERROR_STACK_UNDERFLOW   5
#define  ERROR_DIVISION          6
#define  ERROR_ARC_COSINE        7
#define  ERROR_ARC_TANGENT_2     8
#define  ERROR_LOGARITHM         9
#define  ERROR_POWER             10
#define  ERROR_UNKNOWN           11

#define  LIST_REG                0
#define  LIST_REG_DEREF          1
#define  LIST_MEM                2
#define  LIST_MEM_DEREF          3
#define  LIST_IOP                4

#define  INPUT_NONE              0
#define  INPUT_INIT              1
#define  INPUT_BREAK             2
#define  INPUT_CONTINUE          3
#define  INPUT_DISPLAY           4
#define  INPUT_LABEL             5
#define  INPUT_NEXT              6
#define  INPUT_PRINT             7
#define  INPUT_STEP              8
#define  INPUT_HELP              9
#define  INPUT_QUIT              10
#define  INPUT_REGISTER          11
#define  INPUT_REGISTERS         12
#define  INPUT_MEMADDR           13
#define  INPUT_MEMRANGE          14
#define  INPUT_IOPORT            15
#define  INPUT_IGNORE            16
#define  INPUT_REPLACE           17
#define  INPUT_SET               18
#define  INPUT_UNDO              19
#define  INPUT_LOAD              20
#define  INPUT_UNLOAD            21
#define  INPUT_INVENTORY         22

#define  PARSE_IMMEDIATE         0x79
#define  PARSE_REGISTER          0x7A
#define  PARSE_REGISTERS         0x7B
#define  PARSE_MEMORY            0x7C
#define  PARSE_MEMRANGE          0x7D
#define  PARSE_IOPORT            0x7E
#define  PARSE_NONE              0x7F

#define  FORMAT_BINARY           0
#define  FORMAT_OCTAL            1
#define  FORMAT_UNSIGNED         2
#define  FORMAT_SIGNED           3
#define  FORMAT_FLOAT            4
#define  FORMAT_HEX              5
#define  FORMAT_BOOLEAN          6
#define  FORMAT_DECODE           7
#define  FORMAT_LOWERHEX         8
#define  FORMAT_DEFAULT          9

#define  NUM_PORT_CATEGORIES     7
#define  NUM_TIM_PORTS           4
#define  NUM_RNG_PORTS           1
#define  NUM_GPU_PORTS           18
#define  NUM_SPU_PORTS           14
#define  NUM_INP_PORTS           13
#define  NUM_CAR_PORTS           4
#define  NUM_MEM_PORTS           1

#define  TIM_PORT                0
#define  RNG_PORT                1
#define  GPU_PORT                2
#define  SPU_PORT                3
#define  INP_PORT                4
#define  CAR_PORT                5
#define  MEM_PORT                6

#define  PORT_TYPE(port)         #port

#define  OPCODE_MASK             0xFC000000
#define  IMMVAL_MASK             0x02000000
#define  DSTREG_MASK             0x01E00000
#define  SRCREG_MASK             0x001E0000
#define  MOVADR_MASK             0x0001C000
#define  IOPORT_MASK             0x00003FFF
#define  OPCODESHIFT             26
#define  IMMED_SHIFT             25
#define  DSTREGSHIFT             21
#define  SRCREGSHIFT             17
#define  MOVADRSHIFT             14

#define  TRUE                    1
#define  FALSE                   0

#define  NUM_REGISTERS           19

#define  R0                      0
#define  R1                      1
#define  R2                      2
#define  R3                      3
#define  R4                      4
#define  R5                      5
#define  R6                      6
#define  R7                      7
#define  R8                      8
#define  R9                      9
#define  R10                     10
#define  R11                     11
#define  CR                      11
#define  R12                     12
#define  SR                      12
#define  R13                     13
#define  DR                      13
#define  R14                     14
#define  BP                      14
#define  R15                     15
#define  SP                      15
#define  IP                      16
#define  IR                      17
#define  IV                      18

#define  REGNAME(x)              (reg+x)   -> name
#define  REGALIAS(x)             (reg+x)   -> alias
#define  REGMODE(x)              (reg+x)   -> mode
#define  REG(x)                  (reg+x)   -> value.i32
#define  DSTREG                  (reg+dst) -> value.i32
#define  SRCREG                  (reg+src) -> value.i32
#define  FREG(x)                 (reg+x)   -> value.f32
#define  FDSTREG                 (reg+dst) -> value.f32
#define  FSRCREG                 (reg+src) -> value.f32

#define  IMEMGET(addr)           word2int (memory_get (addr, FALSE))
#define  ISYSMEMGET(addr)        word2int (memory_get (addr, TRUE))
#define  FMEMGET(addr)           word2float (memory_get (addr, FALSE))
#define  MEMSET(addr, word)      memory_set (addr, word, FALSE)
#define  SYSMEMSET(addr, word)   memory_set (addr, word, TRUE)

#define  IPORTGET(addr)          word2int (ioports_get (addr, FALSE))
#define  ISYSPORTGET(addr)       word2int (ioports_get (addr, TRUE))
#define  FPORTGET(addr)          word2float (ioports_get (addr, FALSE))
#define  PORTSET(addr, word)     ioports_set (addr, word, 0.0, FALSE)
#define  FPORTSET(addr, word)    ioports_set (addr, 0, word, FALSE)
#define  SYSPORTSET(addr, word)  ioports_set (addr, word, 0.0, TRUE)
#define  FSYSPORTSET(addr, word) ioports_set (addr, 0, word, TRUE)

#define  REG_RAW                 0
#define  REG_INT                 1
#define  REG_FLOAT               2

#define  FLAG_NONE               0
#define  FLAG_DISPLAY            1
#define  FLAG_PROCESS            2 
#define  FLAG_IMMEDIATE          4
#define  FLAG_DATA               8
#define  FLAG_ERROR              16
#define  FLAG_DEMO               32

#define  FLAG_READ               4
#define  FLAG_WRITE              2

#define  HLT                     0x00
#define  WAIT                    0x01
#define  JMP                     0x02
#define  CALL                    0x03
#define  RET                     0x04
#define  JT                      0x05
#define  JF                      0x06
#define  IEQ                     0x07
#define  INE                     0x08
#define  IGT                     0x09
#define  IGE                     0x0A
#define  ILT                     0x0B
#define  ILE                     0x0C
#define  FEQ                     0x0D
#define  FNE                     0x0E
#define  FGT                     0x0F
#define  FGE                     0x10
#define  FLT                     0x11
#define  FLE                     0x12
#define  MOV                     0x13
#define  LEA                     0x14
#define  PUSH                    0x15
#define  POP                     0x16
#define  IN                      0x17
#define  OUT                     0x18
#define  MOVS                    0x19
#define  SETS                    0x1A
#define  CMPS                    0x1B
#define  CIF                     0x1C
#define  CFI                     0x1D
#define  CIB                     0x1E
#define  CFB                     0x1F
#define  NOT                     0x20
#define  AND                     0x21
#define  OR                      0x22
#define  XOR                     0x23
#define  BNOT                    0x24
#define  SHL                     0x25
#define  IADD                    0x26
#define  ISUB                    0x27
#define  IMUL                    0x28
#define  IDIV                    0x29
#define  IMOD                    0x2A
#define  ISGN                    0x2B
#define  IMIN                    0x2C
#define  IMAX                    0x2D
#define  IABS                    0x2E
#define  FADD                    0x2F
#define  FSUB                    0x30
#define  FMUL                    0x31
#define  FDIV                    0x32
#define  FMOD                    0x33
#define  FSGN                    0x34
#define  FMIN                    0x35
#define  FMAX                    0x36
#define  FABS                    0x37
#define  FLR                     0x38
#define  CEIL                    0x39
#define  ROUND                   0x3A
#define  SIN                     0x3B
#define  ACOS                    0x3C
#define  ATAN2                   0x3D
#define  LOG                     0x3E
#define  POW                     0x3F

#endif
