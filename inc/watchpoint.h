#ifndef __WATCHPOINT_H
#define __WATCHPOINT_H

// Watchpoint operator constants
#define WATCH_OP_EQUAL          0
#define WATCH_OP_NOT_EQUAL      1
#define WATCH_OP_LESS           2
#define WATCH_OP_GREATER        3
#define WATCH_OP_LESS_EQUAL     4
#define WATCH_OP_GREATER_EQUAL  5

// What a watchpoint looks at (target) or compares against (rhs)
#define WLOC_NONE               0
#define WLOC_REG                1   // register: id is the register id
#define WLOC_MEM                2   // memory word: id is the address
#define WLOC_IOPORT             3   // IOPort: id is the port number
#define WLOC_IMM                4   // immediate value (rhs only)

typedef struct wloc
{
    uint8_t   kind;                 // WLOC_*
    uint32_t  id;                   // register id / address / port number / immediate
} wloc_t;

// Watchpoint structure. A watchpoint triggers when its condition (target OP rhs, all
// compared as unsigned 32-bit values) becomes true; it does not trigger again until
// the condition has been false and then true once more.
typedef struct wpoint
{
    wloc_t         target;      // what is watched
    wloc_t         rhs;         // what it is compared against
    uint8_t        operator;    // WATCH_OP_* constant
    uint8_t        active;      // Enable/disable flag
    uint8_t        was_true;    // condition state at the last check
    int8_t        *label;       // Optional label for display
    struct wpoint *next;
} wpoint_t;

// Function prototypes
extern wpoint_t *wpoint; // Watchpoint list (declared in globals.h)

wpoint_t     *wpoint_new             (wloc_t,      uint8_t,        wloc_t);
void          wpoint_add             (wpoint_t **, wpoint_t     *);
uint8_t       wpoint_remove          (wpoint_t **, int);                 // TRUE if removed
uint8_t       wpoint_remove_by_label (wpoint_t **, const int8_t *);      // TRUE if removed
uint8_t       wpoint_check           (void);
void          wpoint_display         (void);
void          wpoint_describe        (wpoint_t *,  char *, size_t);      // "SP < 0x003FFFFF"
void          wpoint_clear           (wpoint_t **);
const int8_t *wpoint_op_to_string    (uint8_t );

#endif // __WATCHPOINT_H
