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
    newnode -> FMT       = FORMAT_DEFAULT;
    newnode -> NUMBER    = 0;
    newnode -> LINE      = 0;
    newnode -> COUNT     = 0;
    newnode -> space     = 7;
    newnode -> pointer   = NULL;
    newnode -> dpointer  = NULL;
    newnode -> time      = -1.0;
    newnode -> RAW       = value;
    newnode -> next      = NULL;
    newnode -> end       = NULL;

    fprintf (debug, "[listnode] type: %hhu, value: %u\n", newnode -> type, value);

    return (newnode);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// list_add(): function to append a node to the end of the indicated list
//
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

        list -> end             = node;
    }

    return (list);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// add_list(): function to insert a node to the start of the indicated list
//
linked_l *add_list (linked_l *list, linked_l *node)
{
    if (node             != NULL)
    {
        node -> next      = NULL;
        if (list         != NULL)
        {
            node -> next  = list;
            node -> end   = list -> end;
            list -> end   = NULL;
        }
        list              = node;
    }

    return (list);
}

linked_l *list_grab (linked_l **list, linked_l *node)
{
    linked_l *tmp                   = NULL;
    if ((list                      != NULL) &&
        (node                      != NULL))
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

                if (tmp -> next    == (*list) -> end)
                {
                    (*list) -> end  = tmp;
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

    while ((tmp                != NULL) &&
           (tmp -> RAW         != value))
    {
        tmp                     = tmp -> next;
    }

    return (tmp);
}

linked_l *find_ptr (linked_l *list, void *pointer)
{
    linked_l *tmp               = list;

    while ((tmp                != NULL) &&
           (tmp -> pointer     != pointer))
    {
        tmp                     = tmp -> next;
    }

    return (tmp);
}
