#include "defines.h"
#include "watchpoint.h"

////////////////////////////////////////////////////////////////////////////////
//
// wloc_read() - read the current value of a location without side effects (reading
// RNG_CurrentValue through ioports_get() would consume a random number)
//
static uint8_t wloc_read(const wloc_t *loc, uint32_t *value)
{
    word_t   *w = NULL;
    data_t   *p = NULL;

    switch (loc->kind)
    {
        case WLOC_IMM:
            *value = loc->id;
            return TRUE;

        case WLOC_REG:
            if (reg == NULL || loc->id >= NUM_REGISTERS)
            {
                return FALSE;
            }
            *value = (reg + loc->id)->value.raw;
            return TRUE;

        case WLOC_MEM:
            w = memory_get(loc->id, TRUE);
            if (w == NULL)
            {
                return FALSE;
            }
            *value = w->raw;
            return TRUE;

        case WLOC_IOPORT:
            if (ioports_chk((uint16_t) loc->id, FLAG_READ, TRUE) != TRUE)
            {
                return FALSE;
            }
            p = (*(ioports + ((loc->id & 0x0700) >> 8))) + (loc->id & 0x00FF);
            *value = p->value.raw;
            return TRUE;
    }
    return FALSE;
}

static void wloc_name(const wloc_t *loc, char *out, size_t len)
{
    int8_t *name = NULL;

    switch (loc->kind)
    {
        case WLOC_REG:
            snprintf(out, len, "%s", reg_get_name((uint8_t) loc->id));
            break;
        case WLOC_MEM:
            snprintf(out, len, "[0x%.8X]", loc->id);
            break;
        case WLOC_IOPORT:
            name = (*(ioports + ((loc->id & 0x0700) >> 8)) + (loc->id & 0x00FF))->name;
            if (name != NULL)
            {
                snprintf(out, len, "%s", name);
            }
            else
            {
                snprintf(out, len, "0x%.3X", loc->id);
            }
            break;
        default:
            snprintf(out, len, "0x%.8X", loc->id);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_eval() - evaluate the condition of a watchpoint; optionally returns the
// current values of both sides. An unreadable location makes the condition false.
//
static uint8_t wpoint_eval(wpoint_t *wp, uint32_t *lhs_out, uint32_t *rhs_out)
{
    uint32_t l = 0;
    uint32_t r = 0;
    uint8_t  result = FALSE;

    if (wloc_read(&wp->target, &l) == TRUE && wloc_read(&wp->rhs, &r) == TRUE)
    {
        switch (wp->operator)
        {
            case WATCH_OP_EQUAL:         result = (l == r); break;
            case WATCH_OP_NOT_EQUAL:     result = (l != r); break;
            case WATCH_OP_LESS:          result = (l <  r); break;
            case WATCH_OP_GREATER:       result = (l >  r); break;
            case WATCH_OP_LESS_EQUAL:    result = (l <= r); break;
            case WATCH_OP_GREATER_EQUAL: result = (l >= r); break;
            default: break;
        }
    }
    if (lhs_out != NULL) *lhs_out = l;
    if (rhs_out != NULL) *rhs_out = r;
    return result;
}

void wpoint_describe(wpoint_t *wp, char *out, size_t len)
{
    char a[64];
    char b[64];

    wloc_name(&wp->target, a, sizeof(a));
    wloc_name(&wp->rhs, b, sizeof(b));
    snprintf(out, len, "%s %s %s", a, wpoint_op_to_string(wp->operator), b);
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_new() - Create a new wpoint node
//
wpoint_t *wpoint_new(wloc_t target, uint8_t op, wloc_t rhs)
{
    wpoint_t *wp = NULL;

    wp = (wpoint_t *) ralloc(sizeof(wpoint_t), 1, FLAG_ZERO);
    if (wp == NULL)
    {
        fprintf(stderr, "[ERROR] Failed to allocate memory for wpoint\n");
        return NULL;
    }

    wp->target = target;
    wp->rhs = rhs;
    wp->operator = op;
    wp->active = TRUE;
    wp->label = NULL;
    wp->next = NULL;
    wp->was_true = wpoint_eval (wp, NULL, NULL);   // do not fire for what is already true

    return wp;
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_add() - Add a wpoint to the list
//
void wpoint_add(wpoint_t **list, wpoint_t *wp)
{
    if (*list == NULL)
    {
        *list = wp;
    }
    else
    {
        wpoint_t *tmp = *list;
        while (tmp->next != NULL)
        {
            tmp = tmp->next;
        }
        tmp->next = wp;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_remove() - Remove a wpoint by index
//
uint8_t wpoint_remove(wpoint_t **list, int index)
{
    wpoint_t *tmp = NULL;
    wpoint_t *prev = NULL;
    int count = 0;

    if (*list == NULL)
    {
        fprintf(stderr, "[ERROR] Watchpoint index %d not found\n", index);
        return FALSE;
    }

    tmp = *list;
    while (tmp != NULL)
    {
        if (count == index)
        {
            if (prev == NULL)
            {
                *list = tmp->next;
            }
            else
            {
                prev->next = tmp->next;
            }

            if (tmp->label != NULL)
            {
                rfree(tmp->label);
            }

            rfree(tmp);
            return TRUE;
        }

        prev = tmp;
        tmp = tmp->next;
        count++;
    }

    fprintf(stderr, "[ERROR] Watchpoint index %d not found\n", index);
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_remove_by_label() - Remove a wpoint by its label
//
uint8_t wpoint_remove_by_label(wpoint_t **list, const int8_t *label)
{
    wpoint_t *tmp = NULL;
    wpoint_t *prev = NULL;

    if ((*list == NULL) || (label == NULL))
    {
        fprintf(stderr, "[ERROR] Watchpoint with label '%s' not found\n", (label != NULL) ? (const char *)label : "");
        return FALSE;
    }

    tmp = *list;
    while (tmp != NULL)
    {
        if ((tmp->label != NULL) && (strcmp((const char *)tmp->label, (const char *)label) == 0))
        {
            if (prev == NULL)
            {
                *list = tmp->next;
            }
            else
            {
                prev->next = tmp->next;
            }

            rfree(tmp->label);
            rfree(tmp);
            return TRUE;
        }

        prev = tmp;
        tmp = tmp->next;
    }

    fprintf(stderr, "[ERROR] Watchpoint with label '%s' not found\n", label);
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_check() - Check all wpoints and return TRUE if any triggered
//
uint8_t wpoint_check(void)
{
    wpoint_t *tmp = wpoint;
    uint32_t  l = 0;
    uint32_t  r = 0;
    uint8_t   cond = FALSE;
    uint8_t   triggered = FALSE;
    char      desc[160];

    while (tmp != NULL)
    {
        if (tmp->active)
        {
            cond = wpoint_eval(tmp, &l, &r);
            if ((cond == TRUE) && (tmp->was_true == FALSE))
            {
                triggered = TRUE;
                wpoint_describe(tmp, desc, sizeof(desc));
                if (colorflag == TRUE)
                {
                    fprintf(stdout, "\e[1;31m");
                }
                fprintf(stdout, "[wpoint] triggered: ");
                if (tmp->label != NULL)
                {
                    fprintf(stdout, "%s ", tmp->label);
                }
                fprintf(stdout, "%s (current: 0x%.8X vs 0x%.8X)\n", desc, l, r);
                if (colorflag == TRUE)
                {
                    fprintf(stdout, "\e[m");
                }
            }
            tmp->was_true = cond;
        }
        tmp = tmp->next;
    }

    return triggered;
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_display() - Display all wpoints
//
void wpoint_display(void)
{
    wpoint_t *tmp = wpoint;
    int count = 0;

    if (tmp == NULL)
    {
        fprintf(stdout, "No wpoints set.\n");
        return;
    }

    fprintf(stdout, "Watchpoints:\n");
    fprintf(stdout, "-----------\n");

    while (tmp != NULL)
    {
        char desc[160];

        wpoint_describe(tmp, desc, sizeof(desc));
        fprintf(stdout, "[%d] %s (%s)",
                count,
                desc,
                tmp->active ? "active" : "inactive");
        if (tmp->label != NULL)
        {
            fprintf(stdout, " - %s", tmp->label);
        }
        fprintf(stdout, "\n");
        tmp = tmp->next;
        count++;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_clear() - Clear all wpoints
//
void wpoint_clear(wpoint_t **list)
{
    wpoint_t *tmp = NULL;
    wpoint_t *next = NULL;

    tmp = *list;
    while (tmp != NULL)
    {
        next = tmp->next;
        if (tmp->label != NULL)
        {
            rfree(tmp->label);
        }
        rfree(tmp);
        tmp = next;
    }

    *list = NULL;
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_op_to_string() - Convert operator constant to string
//
const int8_t *wpoint_op_to_string(uint8_t op)
{
    switch (op)
    {
        case WATCH_OP_EQUAL:        return "==";
        case WATCH_OP_NOT_EQUAL:    return "!=";
        case WATCH_OP_LESS:         return "<";
        case WATCH_OP_GREATER:      return ">";
        case WATCH_OP_LESS_EQUAL:   return "<=";
        case WATCH_OP_GREATER_EQUAL: return ">=";
        default:                    return "??";
    }
}
