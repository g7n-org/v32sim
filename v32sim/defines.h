#ifndef  __DEFINES_H
#define  __DEFINES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

#define  RAM_LAST_ADDR           0x003FFFFF
#define  BIOS_LAST_ADDR          0x100FFFFF
#define  CART_LAST_ADDR          0x27FFFFFF
#define  MEMC_LAST_ADDR          0x3003FFFF

#define  NO_CART_ERROR           1
#define  REGEX_COMPILE_ERROR     2
#define  REGEX_EXECUTE_ERROR     3

#define  IOPORTS_ALLOC_FAIL      4
#define  IOPORTS_READ_ERROR      5
#define  IOPORTS_WRITE_ERROR     6
#define  IOPORTS_BAD_PORT        7

#define  MEMORY_ALLOC_FAIL       8
#define  MEMORY_READ_ERROR       9
#define  MEMORY_WRITE_ERROR      10
#define  MEMORY_BAD_ACCESS       11

#define  FILE_OPEN_ERROR         12
#define  FILE_POSITION_ERROR     13

#define  STRING_ALLOC_FAIL       14
#define  DATA_ALLOC_FAIL         15
#define  LIST_ALLOC_FAIL         16

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
#define  LIST_MEM                1
#define  LIST_IOP                2

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

#define  OPCODE_MASK             0xFC000000
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
#define  BP_REG                  (reg+BP)  -> value.i32
#define  SP_REG                  (reg+SP)  -> value.i32
#define  IP_REG                  (reg+IP)  -> value.i32
#define  IR_REG                  (reg+IR)  -> value.i32
#define  IV_REG                  (reg+IV)  -> value.i32
#define  FREG(x)                 (reg+x)   -> value.f32
#define  FDSTREG                 (reg+dst) -> value.f32
#define  FSRCREG                 (reg+src) -> value.f32
#define  FIV_REG                 (reg+IV)  -> value.f32

#define  REG_RAW                 0
#define  REG_INT                 1
#define  REG_FLOAT               2

#define  FLAG_NONE               0
#define  FLAG_DISPLAY            1
#define  FLAG_PROCESS            2 
#define  FLAG_IMMEDIATE          4

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
#define  PUSH                    0x15
#define  POP                     0x16
#define  IN                      0x17
#define  OUT                     0x18
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
#define  FADD                    0x2F
#define  FSUB                    0x30
#define  FMUL                    0x31
#define  FDIV                    0x32
#define  FMOD                    0x33

#endif
