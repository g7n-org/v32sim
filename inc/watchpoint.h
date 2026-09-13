#ifndef __WATCHPOINT_H
#define __WATCHPOINT_H

// Watchpoint operator constants
#define WATCH_OP_EQUAL          0
#define WATCH_OP_NOT_EQUAL      1
#define WATCH_OP_LESS           2
#define WATCH_OP_GREATER        3
#define WATCH_OP_LESS_EQUAL     4
#define WATCH_OP_GREATER_EQUAL  5

// Watchpoint structure
typedef struct wpoint
{
    uint8_t        reg_id;      // Register ID (0-15 for R0-R15, or system regs)
    uint8_t        operator;    // WATCH_OP_* constant
    uint32_t       value;       // Hex value to compare against
    uint8_t        active;      // Enable/disable flag
    int8_t        *label;       // Optional label for display
    struct wpoint *next;
} wpoint_t;

// Function prototypes
extern wpoint_t *wpoint; // Watchpoint list (declared in globals.h)

wpoint_t     *wpoint_new             (uint8_t,     uint8_t,        uint32_t);
void          wpoint_add             (wpoint_t **, wpoint_t     *);
void          wpoint_remove          (wpoint_t **, int);
void          wpoint_remove_by_label (wpoint_t **, const int8_t *);
uint8_t       wpoint_check           (void);
void          wpoint_display         (void);
void          wpoint_clear           (wpoint_t **);
const int8_t *wpoint_op_to_string    (uint8_t );

#endif // __WATCHPOINT_H
