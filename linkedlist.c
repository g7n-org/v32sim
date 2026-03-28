#include "defines.h"

linked_l *listnode (uint8_t  type, uint32_t  value)
{
    size_t    size       = sizeof (linked_l);
    linked_l *newnode    = (linked_l *) ralloc (size, 1, FLAG_RETERR | FLAG_ZERO);
    if (newnode         == NULL)
    {
        fprintf (stderr, "[ERROR] Could not allocate memory for linked list node!\n");
        exit    (LIST_ALLOC_FAIL);
    }

    newnode -> label     = NULL;
    newnode -> type      = type;
    newnode -> number    = 0;
    newnode -> space     = 7;
    newnode -> pointer   = NULL;
    newnode -> dpointer  = NULL;
    newnode -> data.raw  = value;
    newnode -> next      = NULL;

    fprintf (debug, "[listnode] type: %hhu, value: %u\n", newnode -> type, value);

    return (newnode);
}

linked_l *list_add (linked_l *list, linked_l *node)
{
    linked_l *tmp               = NULL;

    if (node                   != NULL)
    {
        node -> next            = NULL;
        if (list               != NULL)
        {
            tmp                 = list;
            while (tmp -> next != NULL)
            {
                tmp             = tmp -> next;
            }

            tmp -> next         = node;
        }
        else
        {
            list                = node;
        }
    }

    return (list);
}

linked_l *list_grab (linked_l **list, linked_l *node)
{
    linked_l *tmp                   = NULL;
    if (list                       != NULL)
    {
        if ((*list)                != NULL)
        {
            tmp                     = (*list);
            if (tmp                == node) // match is first node
            {
                (*list)             = (*list) -> next;
            }
            else
            {
                while (tmp -> next != node)
                {
                    tmp             = tmp    -> next;
                }
                tmp -> next         = node   -> next;
            }
            node -> next            = NULL;
        }
    }

    return (node);
}

linked_l *find_label (linked_l *list, int8_t *label)
{
    int32_t   check       = 0;
    linked_l *tmp         = list;

    while (tmp           != NULL)
    {
        if (tmp -> label != NULL)
        {
            check         = strncmp (tmp -> label, label, strlen (tmp -> label));
            if (check    == 0)
            {
                break;
            }
        }
        tmp               = tmp -> next;
    }

    return (tmp);
}

linked_l *find_value (linked_l *list, uint32_t  value)
{
    linked_l *tmp               = list;

    while (tmp                 != NULL)
    {
        if (tmp -> data.raw    == value)
        {
            break;
        }
        tmp                     = tmp -> next;
    }

    return (tmp);
}

linked_l *find_ptr (linked_l *list, void *pointer)
{
    linked_l *tmp               = list;

    while (tmp                 != NULL)
    {
        if (tmp -> pointer     == pointer)
        {
            break;
        }
        tmp                     = tmp -> next;
    }

    return (tmp);
}
