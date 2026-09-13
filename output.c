#include "defines.h"

////////////////////////////////////////////////////////////////////////////////////////
//
// output.c
//
// Rendering of displaylist and print items for registers (output_reg),
// memory locations (output_mem), and IOPorts (output_iop).
//
// All three functions follow the same pipeline, implemented once in the
// static helpers below:
//
//   1. build a textual description ("addr") of the item being displayed
//   2. build_entry()    append any format suffix (/X /u /f ...) to form
//                       the displaylist "entry"
//   3. output_header()  recalibrate displaylist column width, print the
//                       entry as the line header, start value color
//   4. validate the item; if invalid, print an indicator and stop
//   5. output_render()  render the 32-bit word in the requested format,
//                       delegating to the shared boolean/binary/string
//                       renderers as needed
//
// What remains specific to each function:
//
//   output_reg:  register naming and aliases; word acquisition is
//                  (deref) IMEMGET(REG(id))    (plain) REG(id)
//   output_mem:  Lua value-tag unboxing; word acquisition is
//                  (deref) IMEMGET(IMEMGET(value))  (plain) IMEMGET(value)
//   output_iop:  port validation; word acquisition is
//                  ISYSPORTGET(value) / FPORTGET(value)
//
// CHANGES folded into this rewrite (relative to the prior revision):
//
//   * output_reg: 'addr' enlarged 19 -> 32 bytes. The deref+alias form
//     "[R15(SP)>0x12345678]" needs 21 bytes and was overflowing the old
//     19-byte buffer by two (eg: 'display [BP]').
//   * output_mem: 'addr' enlarged 26 -> 48, 'entry' 33 -> 64. The Lua
//     invalid-deref form "[0x........(0x........>0x........)]" needs 36
//     bytes ('entry': 38+ with a format suffix) and was overflowing both.
//   * output_reg /s: string characters are now read from the derived
//     string address ('data'), not from the 'addr' display-name buffer.
//     The old code converted that stack buffer pointer into a 32-bit
//     address, which failed the page check and always printed "".
//   * output_mem /s: the string pointer is now derived by memory
//     dereferences (IMEMGET), not REG(value), which was indexing the
//     19-entry register array by a raw memory address (wild read).
//   * output_mem /D (deref): the immediate is now read from
//     IMEMGET(value)+1 (the word that follows the pointed-at
//     instruction). The old code read IMEMGET(IMEMGET(value+1)), which
//     double-dereferenced the wrong slot.
//   * output_mem: the Lua tag bits (previously computed into 'immv' and
//     then reused for the decode immediate) now live in their own 'tag'
//     variable, so the two roles cannot collide.
//   * output_header(): the compact header used while running
//     (runflag == TRUE) was previously output_mem-only; all three
//     functions now share it, so the end-of-run displaylist dump is
//     consistent. (The 'else' compact branch had also gone missing from
//     output_mem's header in the prior merge.)
//   * build_entry(): IOPort entries now show the /D suffix like the
//     others; FORMAT_DECODE still renders nothing for ports, as before.
//   * format_and_output_value() removed: it was unreferenced and its
//     format switch was incomplete.
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
// -------------------------- shared rendering helpers --------------------------
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
// build_entry(): compose the displaylist entry text: the item's "addr"
// description plus a format suffix (/X, /u, /b, ...) if one was requested.
// Common to all three output_*() functions.
//
static void  build_entry (uint8_t  *addr, uint8_t  fmt, uint8_t  *entry)
{
    switch (fmt)
    {
        case FORMAT_HEX:        sprintf (entry, "%s/X", addr);   break;
        case FORMAT_LOWERHEX:   sprintf (entry, "%s/x", addr);   break;
        case FORMAT_UNSIGNED:   sprintf (entry, "%s/u", addr);   break;
        case FORMAT_OCTAL:      sprintf (entry, "%s/o", addr);   break;
        case FORMAT_FLOAT:      sprintf (entry, "%s/f", addr);   break;
        case FORMAT_SIGNED:     sprintf (entry, "%s/d", addr);   break;
        case FORMAT_DECODE:     sprintf (entry, "%s/D", addr);   break;
        case FORMAT_BOOLEAN:    sprintf (entry, "%s/B", addr);   break;
        case FORMAT_STRING:     sprintf (entry, "%s/s", addr);   break;
        case FORMAT_BINARY:     sprintf (entry, "%s/b", addr);   break;
        default:                sprintf (entry, "%s",   addr);   break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_header(): recalibrate the displaylist column width (if a
// displaylist exists- items may also be rendered via one-time 'print'
// commands), emit the entry as the line header, and start value
// colorization. While the simulator is running (runflag == TRUE) a
// compact header is used instead of the column-aligned one. Common to
// all three output_*() functions.
//
static void  output_header (uint8_t  *entry)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Recalibrate spacing if needed (ie encounter a wider displaylist member)
    //
    if ((dpoint              != NULL) &&
        (dpoint -> space     <  strlen (entry)))
    {
        dpoint  -> space     = strlen (entry);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Display the display list header
    //
    if (runflag              == FALSE)
    {
        fprintf (stdout, "%*s: ", (dpoint != NULL) ? (dpoint -> space) : 0, entry);
    }
    else
    {
        fprintf (stdout, "%s:", entry);
    }

    if (colorflag            == TRUE)
    {
        fprintf (stdout, "\e[1;36m");
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_boolean(): render a 32-bit word as FALSE/TRUE
//
static void  output_boolean (uint32_t  data)
{
    if (data                 == 0)
    {
        fprintf (stdout, "FALSE");
    }
    else
    {
        fprintf (stdout, "TRUE");
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_binary(): render a 32-bit word as 32 binary digits
//
static void  output_binary (uint32_t  data)
{
    int32_t   index         = 0;
    uint8_t   digits[33];

    for (index               = 31;
         index              >= 0;
         index               = index - 1)
    {
        digits[index]        = (data % 2) + 0x30;
        data                 = data / 2;
    }
    digits[32]               = '\0';
    fprintf (stdout, "%s", digits);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_string(): render a null-terminated string held in memory, one
// 32-bit word per character (the Vircon32 string format), up to a limit
// of 255 characters. Non-printable characters appear as \xNN escapes.
//
// When 'validate' is TRUE the address is checked first, displaying
// "<invalid address>" instead of an empty string if it does not point at
// readable memory. Lua-mode callers pass FALSE: they have already done
// whatever validation they needed and prefer a quiet empty read.
//
static void  output_string (uint32_t  addr, uint8_t  validate)
{
    uint8_t   ch            = 0;
    int32_t   len           = 0;
    int32_t   max_len       = 255; // safety limit

    if ((validate           == TRUE) &&
        (memory_chk (addr, FLAG_READ, TRUE) == FALSE))
    {
        fprintf (stdout, "<invalid address>");
        return;
    }

    fprintf (stdout, "\"");
    while (len               <  max_len)
    {
        ch                   = ISYSMEMGET (addr + len);
        if (ch              == 0)
        {
            break; // null terminator
        }
        if ((ch             >= 32) &&
            (ch             <  127))
        {
            fprintf (stdout, "%c", ch);      // printable ASCII
        }
        else
        {
            fprintf (stdout, "\\x%02X", ch); // non-printable
        }
        len                  = len + 1;
    }
    fprintf (stdout, "\"");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_lua_boolean(): Lua-mode /B rendering of a boxed value, based on
// its tag bits ('tag' = data & 0xFFC00000) and payload bits ('payload' =
// data & 0x003FFFFF):
//
//   - the nil/false/true/tombstone singletons (RAMSTRING tag, low payloads)
//   - RAM and ROM strings (shown via the shared string renderer)
//   - tables (RAM-only tag) and functions (ROM-only tag)
//   - untagged values: genuine raw IEEE-754 floats (ordinary Lua numbers)
//
// Only output_mem() uses this; it lives here with the other shared
// renderers. ('tag' is compared directly- equivalent to the previous
// re-masking of 'data', which is unchanged since the prelude.)
//
static void  output_lua_boolean (uint32_t  data, uint32_t  tag, uint32_t  payload)
{
    float     fvalue        = 0.0;

    if ((tag                 == 0xFFC00000) && // RAMSTRING tag with a low
        (payload             <  4))            // payload: the primitives
    {
        switch (data)
        {
            case 0xFFC00000: fprintf (stdout, "nil");         break;
            case 0xFFC00001: fprintf (stdout, "false");       break;
            case 0xFFC00002: fprintf (stdout, "true");        break;
            case 0xFFC00003: fprintf (stdout, "<tombstone>"); break;
        }
    }
    else if ((tag            == 0xFFC00000) || // RAM string
             (tag            == 0x7FC00000))   // ROM string
    {
        // 'payload' already holds the unboxed, page-adjusted address
        output_string (payload, FALSE);
    }
    else if (tag             == 0xFF800000) // table (RAM-only tag)
    {
        fprintf (stdout, "table: 0x%06X", data & 0x003FFFFF);
    }
    else if (tag             == 0x7F800000) // function (ROM-only tag)
    {
        fprintf (stdout, "function: 0x%06X", data & 0x003FFFFF);
    }
    else
    {
        // untagged: the exponent bits are not the all-ones NaN pattern,
        // so this is a genuine raw float, not an error case
        memcpy (&fvalue, &data, sizeof (fvalue));
        fprintf (stdout, "%.4f", fvalue);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_render(): the common core. Render the 32-bit word 'data' (with
// float view 'fdata') in the requested format. 'immv' and 'dflags'
// pertain to FORMAT_DECODE only. Each caller obtains the word in its own
// fashion:
//
//   output_reg:  (deref) IMEMGET(REG(id))         (plain) REG(id)
//   output_mem:  (deref) IMEMGET(IMEMGET(value))  (plain) IMEMGET(value)
//   output_iop:  ISYSPORTGET(value)
//
// For FORMAT_STRING the word doubles as the address of the string to
// display (validate-checked, as the register and plain-memory paths
// always did). Lua-mode string display is handled by output_mem itself,
// since it renders from the unboxed payload instead.
//
static void  output_render (uint32_t  data, float  fdata,
                            uint32_t  immv, uint8_t  fmt, uint8_t  dflags)
{
    switch (fmt)
    {
        case FORMAT_DEFAULT:
        case FORMAT_HEX:
            fprintf (stdout, "0x%.8X", data);
            break;

        case FORMAT_LOWERHEX:
            fprintf (stdout, "0x%.8x", data);
            break;

        case FORMAT_UNSIGNED:
            fprintf (stdout, "%u",     data);
            break;

        case FORMAT_OCTAL:
            fprintf (stdout, "0%o",    data);
            break;

        case FORMAT_SIGNED:
            fprintf (stdout, "%d",     data);
            break;

        case FORMAT_FLOAT:
            fprintf (stdout, "%.4f",   fdata);
            break;

        case FORMAT_BOOLEAN:
            output_boolean (data);
            break;

        case FORMAT_BINARY:
            output_binary (data);
            break;

        case FORMAT_STRING:
            output_string (data, TRUE);
            break;

        case FORMAT_DECODE:
            decode (data, immv, (float) immv, dflags);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
//
// ------------------------------ public functions ------------------------------
//
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
//
// output_reg(): display the contents of a register. When 'flag' is TRUE
// the register is dereferenced: what is shown is the word at the address
// held in the register, and the entry text marks this with
// "[Rn>0x........]" (or "[Rn(ALIAS)>0x........]").
//
void  output_reg (uint8_t  id, uint8_t  fmt, uint8_t  flag, uint8_t *label)
{
    int32_t   check         = TRUE;
    uint32_t  data          = 0;
    uint32_t  immv          = 0;
    float     fdata         = 0.0;
    uint8_t   dflags        = FLAG_DATA | FLAG_DISPLAY;
    uint8_t   addr[32];  // "[R15(SP)>0x12345678]" = 21 bytes (was 19)
    uint8_t   entry[64];

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Build the "addr" description; for a dereference, validate the
    // register's content as a readable address
    //
    if (flag                == TRUE)
    {
        if (REGALIAS(id)    != NULL)
        {
            sprintf (addr, "[%s(%s)>0x%.8X]", REGNAME(id), REGALIAS(id), REG(id));
        }
        else
        {
            sprintf (addr, "[%s>0x%.8X]", REGNAME(id), REG(id));
        }
        check               = memory_chk (REG(id), FLAG_READ, TRUE);
    }
    else
    {
        if (REGALIAS(id)    != NULL)
        {
            sprintf (addr, "%s(%s)", REGNAME(id), REGALIAS(id));
        }
        else
        {
            sprintf (addr, "%s", REGNAME(id));
        }
    }

    build_entry   (addr, fmt, entry);
    output_header (entry);

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Invalid dereference address: show the indicator. In Lua mode nothing
    // is shown (the value may legitimately be nil)
    //
    if (check              == FALSE)
    {
        if (modeflag       != FLAG_LUA)
        {
            fprintf (stdout, "<invalid address>");
        }
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////
        //
        // Obtain the word to render: the word at the register's address
        // (deref), or the register's own content. 'fdata' is the same word
        // viewed as a float (FORMAT_FLOAT); the check above guarantees the
        // dereferenced read is safe
        //
        data                = (flag == TRUE) ? IMEMGET (REG(id))  : REG (id);
        fdata               = (flag == TRUE) ? FMEMGET (REG(id)) : FREG (id);

        ////////////////////////////////////////////////////////////////////////
        //
        // For instruction decoding of a dereferenced word, also obtain the
        // immediate value that follows it in memory. (Plain register
        // contents provide no way to know where a following immediate
        // would live, so none is fetched- matching the original behavior)
        //
        if ((fmt            == FORMAT_DECODE) &&
            (IMMVAL_MASK    == (data & IMMVAL_MASK)) &&
            (flag           == TRUE))
        {
            immv            = IMEMGET (REG(id) + 1);
            dflags          = dflags | FLAG_IMMEDIATE;
        }

        output_render (data, fdata, immv, fmt, dflags);
    }

    if (label              != NULL)
    {
        fprintf (stdout, " \"%s\"", label);
    }
    fprintf (stdout, "\n");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_mem(): display the contents of a memory location. When 'flag' is
// TRUE the location is dereferenced: what is shown is the word at the
// address stored at 'value', marked with "[0x........(0x........)]".
//
// In Lua mode the word at 'value' is first inspected for a value tag
// (string, table, function, or primitive) so that /B and /s can present
// the underlying Lua value rather than raw bits.
//
void  output_mem (uint32_t  value, uint8_t  fmt, uint8_t  flag, uint8_t *label)
{
    int32_t   check         = FALSE;
    uint32_t  data          = 0;  // word at 'value' (a tagged value in Lua mode)
    uint32_t  word          = 0;  // word to render (depends on 'flag')
    uint32_t  immv          = 0;  // immediate value for FORMAT_DECODE
    uint32_t  tag           = 0;  // Lua value tag (data & 0xFFC00000)
    uint32_t  payload       = 0;  // Lua payload (data & 0x003FFFFF)
    float     fword         = 0.0;
    uint8_t   dflags        = FLAG_DATA | FLAG_DISPLAY;
    uint8_t   addr[48];  // Lua invalid-deref form needs 36 bytes (was 26)
    uint8_t   entry[64]; // ...plus a format suffix (was 33)

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Lua mode: untag the word at 'value' to find what is actually stored
    // there, validating any embedded address. 'tag' and 'payload' are kept
    // for the /B and /s renderings further below. (The original code
    // re-derived 'payload' inside the two string cases after validation;
    // those recomputations were no-ops and have been dropped.)
    //
    if (modeflag           == FLAG_LUA)
    {
        data                = ISYSMEMGET (value);
        tag                 = data & 0xFFC00000;
        payload             = data & 0x003FFFFF;

        switch (tag)
        {
            case 0x7FC00000: // boxed ROM string
                // ROM strings live in the CART page: apply the page bit
                payload     = payload | 0x20000000;
                check       = memory_chk (payload, FLAG_READ, TRUE);
                break;

            case 0xFFC00000: // boxed RAM string (and primitives)
                // payloads 0-3 are the singletons nil/false/true/tombstone;
                // payloads >= 4 are actual RAM string addresses
                if (payload < 4)
                {
                    check   = TRUE; // 'data' already holds the boxed value
                }
                else
                {
                    check   = memory_chk (payload, FLAG_READ, TRUE);
                }
                break;

            default: // untagged word, or a table/function tag
                check       = memory_chk (value, FLAG_READ, TRUE);
                break;
        }
    }
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // plain mode: validate 'value' and obtain the word it holds
    //
    else
    {
        check               = memory_chk (value, FLAG_READ, TRUE);
        if (check          == TRUE)
        {
            data            = ISYSMEMGET (value);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Build the "addr" description. For a dereference, the word at 'value'
    // is itself treated as an address and validated; in Lua mode the
    // unboxed 'payload' is shown alongside when that validation fails
    //
    if (flag                == TRUE)
    {
        check               = memory_chk (data, FLAG_READ, TRUE);
        if (check          == TRUE)
        {
            sprintf (addr, "[0x%.8X(0x%.8X)]", value, data);
        }
        else
        {
            if (modeflag   == FLAG_LUA)
            {
                sprintf (addr, "[0x%.8X(0x%.8X>0x%.8X)]", value, data, payload);
            }
            else
            {
                sprintf (addr, "[0x%.8X(invalid)]", value);
            }
        }
    }
    else
    {
        sprintf (addr, "0x%.8X", value);
    }

    build_entry   (addr, fmt, entry);
    output_header (entry);

    if (check              == FALSE)
    {
        fprintf (stdout, "<invalid address>");
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////
        //
        // Obtain the word to render (and its float view): for a
        // dereference, the word at the address stored at 'value'; for a
        // plain display, the word at 'value' itself. (In Lua mode this is
        // the same word the prelude read into 'data'.)
        //
        word                = (flag == TRUE) ? IMEMGET (IMEMGET (value)) : IMEMGET (value);
        fword               = (flag == TRUE) ? FMEMGET (IMEMGET (value)) : FMEMGET (value);

        ////////////////////////////////////////////////////////////////////////
        //
        // For instruction decoding, also obtain the immediate value that
        // follows the instruction in memory. The instruction lives at
        // 'value' (plain) or at the address stored at 'value' (deref), so
        // the immediate is the word after that address
        //
        if ((fmt            == FORMAT_DECODE) &&
            (IMMVAL_MASK    == (word & IMMVAL_MASK)))
        {
            dflags          = dflags | FLAG_IMMEDIATE;
            if (flag       == TRUE)
            {
                immv        = IMEMGET (IMEMGET (value) + 1);
            }
            else
            {
                immv        = IMEMGET (value + 1);
            }
        }

        ////////////////////////////////////////////////////////////////////////
        //
        // Lua mode: /B and /s present the untagged Lua value; all other
        // formats render the (possibly dereferenced) word exactly as in
        // plain mode
        //
        if (modeflag       == FLAG_LUA)
        {
            switch (fmt)
            {
                case FORMAT_BOOLEAN:
                    output_lua_boolean (data, tag, payload);
                    break;

                case FORMAT_STRING:
                    output_string (payload, FALSE);
                    break;

                default:
                    output_render (word, fword, immv, fmt, dflags);
                    break;
            }
        }
        else
        {
            output_render (word, fword, immv, fmt, dflags);
        }
    }

    if (label              != NULL)
    {
        if (runflag        == FALSE)
        {
            fprintf (stdout, " \"%s\"", label);
        }
        else
        {
            fprintf (stdout, ":%s", label);
        }
    }
    fprintf (stdout, "\n");
}

////////////////////////////////////////////////////////////////////////////////////////
//
// output_iop(): display the contents of an IOPort. Ports are never
// dereferenced, so there is no 'flag' parameter. FORMAT_STRING is a hex
// byte-dump of the port's 32-bit value (existing behavior), and
// FORMAT_DECODE has no port rendering (nothing is printed for it).
//
void  output_iop (uint32_t  value, uint8_t  fmt, uint8_t *label)
{
    int32_t   check         = TRUE;
    int32_t   index         = 0;
    uint32_t  data          = 0;
    float     fdata         = 0.0;
    uint8_t   addr[16];  // "0x%.3X" fits in 6; extra room for safety (was 6)
    uint8_t   entry[64];

    sprintf (addr, "0x%.3X", value);
    check                   = ioports_chk (value, FLAG_READ, TRUE);

    build_entry   (addr, fmt, entry);
    output_header (entry);

    if (check              == FALSE)
    {
        fprintf (stdout, "<invalid port>");
    }
    else
    {
        ////////////////////////////////////////////////////////////////////////
        //
        // Obtain the word to render (and its float view) from the port
        //
        data                = ISYSPORTGET (value);
        fdata               = FPORTGET (value);

        switch (fmt)
        {
            case FORMAT_STRING:
                // byte dump of the port's value, most significant byte
                // first ('entry' is reused as scratch here; the header
                // has already been printed by this point)
                for (index  = 3;
                     index >= 0;
                     index  = index - 1)
                {
                    entry[index] = (data & 0x000000FF);
                    fprintf (stdout, "%.2hhX ", entry[index]);
                    data          = data >> 8;
                }
                entry[4]         = '\0';
                fprintf (stdout, "\"%s\"", entry);
                break;

            case FORMAT_DECODE:
                // no instruction decoding of port values (existing behavior)
                break;

            default:
                output_render (data, fdata, 0, fmt, FLAG_DATA | FLAG_DISPLAY);
                break;
        }
    }

    if (label              != NULL)
    {
        fprintf (stdout, " (%s)", label);
    }
    fprintf (stdout, "\n");
}
