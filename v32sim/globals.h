#ifndef  _GLOBALS_H
#define  _GLOBALS_H

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

typedef struct display_list display_l;
struct display_list
{
	int8_t    *label; // to label display points
    uint8_t    type;
    uint8_t    num;
	uint32_t   addr;  // for breakpoints
    word_t    *list;
    display_l *next;
};

struct data_type
{
    word_t   value;
    uint8_t  flag;
	uint8_t  mode;
    int8_t  *name;
};
typedef struct data_type data_t;

struct memory_type
{
    uint8_t   type;
    data_t   *data;
    uint32_t  firstaddr;
    uint32_t  last_addr;
    uint32_t  size;
};
typedef struct memory_type mem_t;

extern FILE      *display;
extern FILE      *devnull;
extern FILE      *program;
extern FILE      *verbose;
extern uint8_t   *destination;
extern uint8_t   *source;
extern int8_t    *biosfile;
extern int8_t    *cartfile;
extern int8_t     sys_error;
extern mem_t     *memory;
extern data_t    *reg;
extern int8_t    *token_label;

////////////////////////////////////////////////////////////////////////////////////////
//
// Variables related to IOPorts
//
extern data_t   **ioports;
extern uint8_t    sys_force;
extern uint8_t    sys_reg_show;

extern uint8_t    action;
extern uint8_t   *data;
extern uint8_t    runflag;
extern uint8_t    branchflag;
extern uint8_t    indexflag;
extern uint8_t    haltflag;
extern uint8_t    waitflag;
extern uint8_t    wordsize;
extern uint32_t   rom_offset;
extern uint32_t   seek_word;
extern display_l *dpoint;

////////////////////////////////////////////////////////////////////////////////////////
//
// Function prototypes
//
size_t     get_filesize   (int8_t *);
uint32_t   get_word       (FILE *);
void       put_word       (uint32_t,    uint8_t);
void       decode         (uint32_t,    uint32_t,    float,    uint8_t);
void       decode_display (uint32_t,    uint32_t,    float,    uint8_t);
void       decode_process (uint32_t,    uint32_t,    float,    uint8_t);
void       init_ioports   (void);                              // initialize IOPorts
int32_t    ioports_get    (uint16_t);                          // get value from port
void       ioports_set    (uint16_t,    int32_t);              // set value to port
void       init_memory    (void);                              // initialize memory
void       load_memory    (uint32_t,    int8_t *);             // load file into memory
word_t    *memory_get     (uint32_t);                          // get value from memory
void       memory_set     (uint32_t,    uint32_t);             // set value to memory
void       init_registers (void);
word_t    *reg_get        (uint8_t,     uint8_t);
void       reg_set        (uint8_t,     uint32_t,    uint8_t);
word_t    *new_word_i32   (uint32_t *,  uint8_t);
void       update_cycle   (void);                              // updating CycleCounter
void       update_frame   (void);                              // updating FrameCounter
uint32_t   word2int       (word_t *);
float      word2float     (word_t *);
display_l *newdispnode    (uint8_t,     word_t *,    uint8_t);
display_l *display_add    (display_l *, display_l *);
void       displayshow    (display_l *, uint8_t);
void       show_sysregs   (void);
void       process_args   (int32_t,     int8_t **);
void       usage          (int8_t *);
uint32_t   tokenize_asm   (uint8_t *);
uint8_t    tokenize_input (uint8_t *);
uint8_t   *get_input      (FILE *,      const uint8_t *);
uint8_t    prompt         (uint32_t);

#endif
