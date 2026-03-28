#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// ralloc(): resource allocation function. A wrapper around malloc()/calloc() based
// on passed flag, which also tracks allocated resources via a dedicated list (mpoint)
// for later deallocation in rfree()
//
void *ralloc (size_t  size, size_t  number, uint8_t  flag)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // declare and initialize variables
    //
    linked_l *mtmp              = NULL;
    void     *resource          = NULL;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // only proceed if there is something to allocate
    //
    if ((size                  >  0) &&
        (number                >  0))
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // difference between calloc(3) and malloc(3) is whether we are zeroing the
        // allocated memory. That is controlled by whether or not FLAG_ZERO is set
        //
        if ((flag & FLAG_ZERO) == FLAG_ZERO)
        {
            resource            = calloc (size, number);
            if ((resource      == NULL) &&
                (FLAG_RETERR   != (flag & FLAG_RETERR)))
            {
                fprintf (stderr,
                         "[alloc] calloc() ERROR: unable to allocate memory!\n");
                exit    (MEMORY_ALLOC_FAIL);
            }
        }
        else
        {
            resource            = malloc (size * number);
            if ((resource      == NULL) &&
                (FLAG_RETERR   != (flag & FLAG_RETERR)))
            {
                fprintf (stderr,
                         "[alloc] malloc() ERROR: unable to allocate memory!\n");
                exit    (MEMORY_ALLOC_FAIL);
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // create and add a new node to the list 
        //
        if ((resource          != NULL) &&
            (FLAG_TRACK        == (flag & FLAG_TRACK)))
        {
            mtmp                = listnode (LIST_PTR, 0);
            mtmp -> space       = size * number;
            mtmp -> pointer     = resource;
            mtmp -> dpointer    = &resource;
            mpoint              = list_add (mpoint, mtmp);
        }
    }

    return (resource);
}

void  rfree (void *resource)
{
    linked_l *mptr                   = NULL;
    if (resource                    != NULL)
    {
        mptr                         = find_ptr  (mpoint,  resource);
        if (mptr                    != NULL)
        {
            mptr                     = list_grab (&mpoint, mptr);
            if (mptr                != NULL)
            {
                free (mptr -> pointer);
                *(mptr -> dpointer)  = NULL;
                free (mptr);
                mptr                 = NULL;
            }
            else
            {
                free (resource);
            }
        }
    }
}
