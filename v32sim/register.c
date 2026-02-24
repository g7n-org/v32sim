#include "defines.h"

void  init_registers (void)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t    index                       = 0;
    size_t     len                         = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // reg is an array of word_t, one for each specified register.
    //
    len                                = sizeof (data_t) * NUM_REGISTERS;
    reg                                = (data_t *) malloc (len);
    if (reg                           == NULL)
    {
        fprintf (stderr, "[ERROR] Failed to allocate resources for registers\n");
        exit    (DATA_ALLOC_FAIL);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize the registers
    //
    for (index                         = 0;
         index                        <  NUM_REGISTERS;
         index                         = index + 1)
    {
        len                            = sizeof (int8_t) * 4;
        REGNAME(index)                 = (int8_t *) malloc (len);
        sprintf (REGNAME(index), "R%u", index);
        REG(index)                     = 0x00000000;
        REGMODE(index)                 = REG_INT;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // set up aliases for string registers
    //
    sprintf (REGNAME(CR), "CR");
    sprintf (REGNAME(SR), "SR");
    sprintf (REGNAME(DR), "DR");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // set up aliases for stack registers
    //
    sprintf (REGNAME(BP), "BP");
    sprintf (REGNAME(SP), "SP");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize stack registers to system defaults
    //
    BP_REG                             = 0x003FFFFF;
    SP_REG                             = 0x003FFFFF;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // set up aliases for system registers
    //
    sprintf (REGNAME(IP), "IP");
    sprintf (REGNAME(IR), "IR");
    sprintf (REGNAME(IV), "IV");

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // initialize the system registers to system defaults
    //
    IP_REG                             = 0x10000004; // BIOS entry point
}

word_t *reg_get (uint8_t  id, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    word_t   *wptr          = NULL;

    if (id                 <  NUM_REGISTERS)
    {
        wptr                = &(reg+id) -> value;

        if (id             >  15)
        {
            if (sys_force  == FALSE)
            {
                fprintf (stderr, "[ERROR] register '%s' not accessible via READ!\n",
                                 (reg+id) -> name);
                wptr        = NULL;
            }
        }
    }

    return (wptr);
}

void  reg_set (uint8_t  id, uint32_t  value, uint8_t  sys_force)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize local variables
    //
    if (id                 <  NUM_REGISTERS)
    {
        if ((id            >  15) &&
            (sys_force     == FALSE))
        {
            fprintf (stderr, "[reg_set] ERROR: register '%s' cannot WRITE!\n",
                             (reg+id) -> name);
        }
        memcpy ((reg+id) -> value.raw, value, 4);
    }
    else
    {
        fprintf (stderr, "[reg_set] ERROR: invalid, non-existent register!\n");
    }
}
