#include "defines.h"
#include "watchpoint.h"

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_new() - Create a new wpoint node
//
wpoint_t *wpoint_new(uint8_t reg_id, uint8_t op, uint32_t value)
{
    wpoint_t *wp = NULL;

    wp = (wpoint_t *) ralloc(sizeof(wpoint_t), 1, FLAG_NONE);
    if (wp == NULL)
    {
        fprintf(stderr, "[ERROR] Failed to allocate memory for wpoint\n");
        return NULL;
    }

    wp->reg_id = reg_id;
    wp->operator = op;
    wp->value = value;
    wp->active = TRUE;
    wp->label = NULL;
    wp->next = NULL;

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
void wpoint_remove(wpoint_t **list, int index)
{
    wpoint_t *tmp = NULL;
    wpoint_t *prev = NULL;
    int count = 0;

    if (*list == NULL)
    {
        return;
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
            return;
        }

        prev = tmp;
        tmp = tmp->next;
        count++;
    }

    fprintf(stderr, "[ERROR] Watchpoint index %d not found\n", index);
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_remove_by_label() - Remove a wpoint by its label
//
void wpoint_remove_by_label(wpoint_t **list, const int8_t *label)
{
    wpoint_t *tmp = NULL;
    wpoint_t *prev = NULL;

    if ((*list == NULL) || (label == NULL))
    {
        return;
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
            return;
        }

        prev = tmp;
        tmp = tmp->next;
    }

    fprintf(stderr, "[ERROR] Watchpoint with label '%s' not found\n", label);
}

////////////////////////////////////////////////////////////////////////////////
//
// wpoint_check() - Check all wpoints and return TRUE if any triggered
//
uint8_t wpoint_check(void)
{
    wpoint_t *tmp = wpoint;
    uint32_t reg_value = 0;
    uint8_t triggered = FALSE;

    while (tmp != NULL)
    {
        if (tmp->active)
        {
            reg_value = REG(tmp->reg_id);

            switch (tmp->operator)
            {
                case WATCH_OP_EQUAL:
                    triggered = (reg_value == tmp->value);
                    break;
                case WATCH_OP_NOT_EQUAL:
                    triggered = (reg_value != tmp->value);
                    break;
                case WATCH_OP_LESS:
                    triggered = (reg_value < tmp->value);
                    break;
                case WATCH_OP_GREATER:
                    triggered = (reg_value > tmp->value);
                    break;
                case WATCH_OP_LESS_EQUAL:
                    triggered = (reg_value <= tmp->value);
                    break;
                case WATCH_OP_GREATER_EQUAL:
                    triggered = (reg_value >= tmp->value);
                    break;
                default:
                    fprintf(stderr, "[ERROR] Invalid wpoint operator: %d\n", tmp->operator);
                    break;
            }

            if (triggered == TRUE)
            {
                if (colorflag == TRUE)
                {
                    fprintf(stdout, "\e[1;31m");
                }
                fprintf(stdout, "[wpoint] triggered: ");
                if (tmp->label != NULL)
                {
                    fprintf(stdout, "%s ", tmp->label);
                }
                fprintf(stdout, "%s %s 0x%.8X (current: 0x%.8X)\n",
                        reg_get_name(tmp->reg_id),
                        wpoint_op_to_string(tmp->operator),
                        tmp->value,
                        reg_value);
                if (colorflag == TRUE)
                {
                    fprintf(stdout, "\e[m");
                }
                return TRUE;
            }
        }
        tmp = tmp->next;
    }

    return FALSE;
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
        fprintf(stdout, "[%d] %s %s 0x%.8X (%s)",
                count,
                reg_get_name(tmp->reg_id),
                wpoint_op_to_string(tmp->operator),
                tmp->value,
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
