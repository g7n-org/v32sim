#ifndef  _GLOBALS_H
#define  _GLOBALS_H

typedef signed   long long int slli;
typedef unsigned long long int ulli;
typedef struct timespec        TimeSpec;

// Simplify opcode display
struct instruction_opcode
{
    uint8_t  name[7];
};
typedef struct instruction_opcode opcode_t;

union  word_type
{
    int32_t   i32;
    float     f32;
    uint32_t  raw;
};
typedef union word_type word_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// data0: shorthand union to "overload" member elements in the linked_list
// struct (fmt vs active)
//
union  data0
{
    uint8_t   fmt;
    uint8_t   active;
};
typedef union data0     d0_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// data1: shorthand union to "overload" member elements in the linked_list
// struct (number vs frames)
//
union  data1
{
    uint32_t  number;
    uint32_t  frames;
};
typedef union data1     d1_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// data2: shorthand union to "overload" member elements in the linked_list
// struct (line vs cycles)
//
union  data2
{
    uint32_t  line;
    uint32_t  cycles;
};
typedef union data2     d2_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// data3: shorthand union to "overload" member elements in the linked_list
// struct (count vs ???)
//
union  data3
{
    uint32_t  count;
};
typedef union data3     d3_t;

////////////////////////////////////////////////////////////////////////////////////////
//
// data4: shorthand union to "overload" member elements in the linked_list
// struct (subcall vs ???)
//
union  data4
{
    uint32_t  subcall;
};
typedef union data4     d4_t;

typedef struct linked_list linked_l;
struct linked_list
{
    int8_t    *name;  // to store filename
    int8_t    *cname;
    int8_t    *label; // to label display points
    uint8_t    type;
    uint8_t    space;
    d0_t       item0;
    d1_t       item1;
    d2_t       item2;
    d3_t       item3;
    d4_t       item4;
    float      time;
    void      *pointer;
    void     **dpointer;
    word_t     data;
    linked_l  *next;
    linked_l  *end;
};

struct data_type
{
    word_t   value;
    uint8_t  flag;
    uint8_t  mode;
    uint8_t  qty;
    uint8_t  fmt;
    int8_t  *name;
    int8_t  *alias;
};
typedef struct data_type data_t;

struct memory_type
{
    uint8_t   type;
    data_t   *data;
    uint8_t   flag;
    uint32_t  firstaddr;
    uint32_t  last_addr;
    uint32_t  size;
    uint32_t  checksum;
};
typedef struct memory_type mem_t;

extern FILE      *display;
extern FILE      *devnull;
extern FILE      *debug;
extern FILE      *verbose;
extern int8_t    *biosfile;
extern int8_t    *cartfile;
extern int8_t    *memcfile;
extern int8_t    *commandfile;
extern int8_t    *biosasmdebug;
extern int8_t    *cartasmdebug;
extern int8_t    *bioscdebug;
extern int8_t    *cartcdebug;
extern int8_t     sys_error;
extern mem_t     *memory;
extern data_t    *reg;
extern int8_t    *token_label;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts
//
extern data_t   **ioports;
extern vtex_t    *bios_vtex;  // for managing textures and regions within textures
extern vtex_t    *cart_vtex;  // for managing textures and regions within textures
extern gamepad_t *gamepad;
extern uint8_t    sys_reg_show;

extern uint8_t    action;
extern uint8_t    modeflag;
extern uint8_t    biosasmdebugflag;
extern uint8_t    bioscdebugflag;
extern uint8_t    cartasmdebugflag;
extern uint8_t    cartcdebugflag;
extern uint8_t    profileflag;
extern uint8_t    runflag;
extern uint8_t    colorflag;
extern uint8_t    branchflag;
extern uint8_t    ignoreflag;
extern uint8_t    retflag;
extern uint8_t    derefaddr;
extern uint8_t    errorcheck;
extern uint8_t    nextflag;
extern uint8_t    haltflag;
extern uint8_t    waitflag;
extern uint8_t    wordsize;
extern uint32_t   profcount;
extern uint32_t   profcountsub;
extern uint32_t   profcountopcode[64];
extern uint32_t   rom_offset;
extern uint32_t   seek_word;
extern uint32_t   watch_word;
extern linked_l  *bpoint; // breakpoint list
extern linked_l  *dpoint; // display list
extern linked_l  *lpoint; // label list
extern linked_l  *mpoint; // tracking allocated memory
extern linked_l  *ppoint; // subroutine profiling list
extern linked_l  *csub;   // current subroutine
extern linked_l  *tpoint;
extern opcode_t   lookup[64];

////////////////////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
size_t    get_filesize   (int8_t *);
uint32_t  get_word       (FILE *);
void      put_word       (uint32_t,    uint8_t);
uint8_t   decode         (uint32_t,    uint32_t,    float,    uint8_t);
void      decode_display (uint32_t,    uint32_t,    float,    uint8_t);
uint8_t   decode_check   (uint32_t,    uint32_t,    float,    uint8_t);
void      decode_process (uint32_t,    uint32_t,    float,    uint8_t);
void      init_ioports   (void);                              // initialize IOPorts
uint8_t   ioports_chk    (uint16_t,    uint8_t,     uint8_t);
int16_t   ioports_num    (uint8_t *);
data_t   *ioports_ptr    (uint16_t);
word_t   *ioports_get    (uint16_t,    uint8_t);              // get value from port
uint8_t   ioports_set    (uint16_t,    int32_t,     float,    uint8_t); // set value to port
void      update_ioports (void);
void      init_memory    (void);                              // initialize memory
uint8_t   alloc_memory   (int32_t);
void      load_command   (void);
uint32_t  load_labels    (uint8_t *,   uint8_t,     uint8_t);
uint8_t   load_memory    (uint32_t,    int8_t *);             // load file into memory
uint8_t   unload_memory  (uint32_t);
uint8_t   memory_chk     (uint32_t,    uint8_t,     uint8_t);
word_t   *memory_get     (uint32_t,    uint8_t);              // get value from memory
void      memory_set     (uint32_t,    uint32_t,    uint8_t); // set value to memory
void      fmemory_set    (uint32_t,    float,       uint8_t);
void      init_registers (void);
word_t   *reg_get        (uint8_t,     uint8_t);
int8_t   *reg_get_name   (uint8_t,     uint8_t);
void      reg_set        (uint8_t,     uint32_t,    uint8_t);
word_t   *reg_get        (uint8_t,     uint8_t);
void      reg_set        (uint8_t,     uint32_t,    uint8_t);
void      output_reg     (uint8_t,     uint8_t,     uint8_t,  uint8_t *);
void      output_mem     (uint32_t,    uint8_t,     uint8_t,  uint8_t *);
void      output_iop     (uint32_t,    uint8_t,     uint8_t *);
word_t   *new_word_i32   (uint32_t *,  uint8_t);
void      update_cycle   (void);                              // updating CycleCounter
void      update_frame   (void);                              // updating FrameCounter
uint32_t  word2int       (word_t *);
float     word2float     (word_t *);
linked_l *listnode       (uint8_t,     uint32_t);
linked_l *list_add       (linked_l *,  linked_l *);
linked_l *add_list       (linked_l *,  linked_l *);
linked_l *list_grab      (linked_l **, linked_l *);
void      displayshow    (linked_l *,  uint8_t);
void      show_sysregs   (void);
void      display_config (uint8_t);
uint8_t  *show_size      (uint32_t);
void      process_args   (int32_t,     int8_t  **);
void      usage          (int8_t   *);
void      help           (uint8_t);
uint8_t  *parse_deref    (uint8_t  *,  uint8_t  *);
uint8_t   parse_token    (uint8_t  *,  uint8_t  *,  uint8_t);
uint8_t   parse_memrange (uint8_t  *);
uint8_t   parse_imm      (uint8_t  *,  data_t   *);
uint8_t   parse_reg      (uint8_t  *);
uint32_t  tokenize_asm   (uint8_t  *);
uint8_t   tokenize_input (uint8_t  *,  uint8_t  *);
uint8_t  *get_input      (FILE     *,  const uint8_t *);
uint8_t   prompt         (uint32_t);
linked_l *find_label     (linked_l *,  int8_t *);
linked_l *find_value     (linked_l *,  uint32_t);
linked_l *find_ptr       (linked_l *,  void     *);
slli      timediff_ns    (TimeSpec *,  TimeSpec *);
void      gamepad_io     (uint16_t);
void     *ralloc         (size_t,      size_t,      uint8_t);
void      rfree          (void     *);
void      sigint         (int32_t);

#endif
