#include "defines.h"

size_t    get_filesize (int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t  offset   = 0;
    size_t   size     = 0;
    FILE    *fptr     = NULL;

    if (filename     != NULL)
    {
        fptr          = fopen (filename, "rb"); // open in (binary) read mode

        ////////////////////////////////////////////////////////////////////////////////
        //
        // open indicated file for reading (in binary mode, if pertinent)
        //
        if (fptr     == NULL)
        {
            fprintf (stderr, "[ERROR] Unable to open '%s' for reading\n", filename);
            exit    (FILE_OPEN_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // adjust FILE pointer to end of file
        //
        offset        = fseek (fptr, 0, SEEK_END);
        if (offset   == 0)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Get the current position (file size in bytes)
            //
            size      = ftell (fptr);
            if (size == -1)
            {
                fprintf (stderr, "[ERROR] Unable to obtain file position\n");
                exit    (FILE_POSITION_ERROR);
            }
        }
        else
        {
            fprintf (stderr, "[ERROR] Unable to seek to end of file\n");
            exit    (FILE_POSITION_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Close the file
        //
        fclose (fptr);
    }

    return (size);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// rom_rd32(): the little endian word at a byte offset of the file
//
static uint32_t  rom_rd32 (FILE *fptr, uint64_t pos)
{
    uint8_t  b[4] = {0, 0, 0, 0};

    if ((fseek (fptr, (long) pos, SEEK_SET) != 0) || (fread (b, 1, 4, fptr) != 4))
    {
        return (0);
    }
    return ((uint32_t) b[0] | ((uint32_t) b[1] << 8) | ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24));
}

////////////////////////////////////////////////////////////////////////////////////////
//
// rom_fail(): report a malformed BIOS or cartridge file
//
static uint8_t  rom_fail (int8_t *filename, const char *what, const char *msg)
{
    fprintf (stderr, "[ERROR:ROM] cannot load %s '%s': %s\n", what, filename, msg);
    return (FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// rom_check(): validate a BIOS or cartridge file the way the console does before it
// loads one (V32Console::LoadBios/LoadCartridge): file and header sanity, section
// layout, program ROM size limits, and the signature and limits of every texture and
// sound. Returns the counts that size the page: program ROM words, number of textures
// and sounds, and the total pixels of all textures.
//
// The console reads sections without noticing a file that ends early, so a ROM whose
// sections do not fit its own file is accepted there (with whatever the read left in
// memory); here it is refused, as there is nothing sensible to run.
//
static uint8_t  rom_check (FILE *fptr, int8_t *filename, uint32_t page, uint32_t *nwords,
                           uint32_t *ntex, uint32_t *nsnd, uint32_t *texwords)
{
    const char *what      = (page == V32_PAGE_BIOS) ? "BIOS" : "cartridge";
    uint64_t    filebytes = (uint64_t) get_filesize (filename);
    uint8_t     hdr[128];
    uint64_t    pos       = 0;
    uint32_t    words     = 0;
    uint32_t    total     = 0;
    uint32_t    i         = 0;
    uint32_t    w[8];

    if ((filebytes % 4) != 0)
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (file size must be a multiple of 4)"));
    }
    if (filebytes < sizeof (hdr))
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (file is too small)"));
    }
    fseek (fptr, 0, SEEK_SET);
    if (fread (hdr, 1, sizeof (hdr), fptr) != sizeof (hdr))
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (file is too small)"));
    }

    if (memcmp (hdr, (page == V32_PAGE_BIOS) ? "V32-CART" : "V32-BIOS", 8) == 0)
    {
        return (rom_fail (filename, what, (page == V32_PAGE_BIOS)
                ? "Input V32 ROM cannot be loaded as a BIOS (is it a cartridge instead)"
                : "Input V32 ROM cannot be loaded as a cartridge (is it a BIOS instead)"));
    }
    if (memcmp (hdr, (page == V32_PAGE_BIOS) ? "V32-BIOS" : "V32-CART", 8) != 0)
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (file does not have a valid signature)"));
    }
    if ((rom_rd32 (fptr, 8) > 1) || (rom_rd32 (fptr, 12) > 0))     // Vircon version 1.0
    {
        return (rom_fail (filename, what, "made for a more recent version of Vircon32. Please use an updated simulator"));
    }

    for (i = 0; i < 8; i = i + 1)       // words 22..29: textures, sounds, three sections
    {
        w[i] = rom_rd32 (fptr, (uint64_t) (22 + i) * 4);
    }
    *ntex = w[0];
    *nsnd = w[1];

    if (page == V32_PAGE_BIOS)
    {
        if (w[0] != 1)
        {
            return (rom_fail (filename, what, "A BIOS video rom should have exactly 1 texture"));
        }
        if (w[1] != 1)
        {
            return (rom_fail (filename, what, "A BIOS audio rom should have exactly 1 sound"));
        }
    }
    else
    {
        if (w[0] > 256)
        {
            return (rom_fail (filename, what, "Video ROM contains too many textures (Vircon GPU only allows up to 256)"));
        }
        if (w[1] > 1024)
        {
            return (rom_fail (filename, what, "Audio ROM contains too many sounds (Vircon SPU only allows up to 1024)"));
        }
    }
    if (w[2] != sizeof (hdr))
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (program ROM is not located after file header)"));
    }
    if (w[4] != (uint32_t) (w[2] + w[3]))
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (video ROM is not located after program ROM)"));
    }
    if (w[6] != (uint32_t) (w[4] + w[5]))
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (audio ROM is not located after video ROM)"));
    }
    if (filebytes != (uint32_t) (w[6] + w[7]))
    {
        return (rom_fail (filename, what, "Incorrect V32 file format (file size does not match indicated ROM contents)"));
    }

    // program ROM: "V32-VBIN", number of words, the words
    pos = sizeof (hdr);
    if ((pos + 12 > filebytes) || (rom_rd32 (fptr, pos) != 0x2D323356) || (rom_rd32 (fptr, pos + 4) != 0x4E494256))
    {
        return (rom_fail (filename, what, (page == V32_PAGE_BIOS) ? "BIOS binary does not have a valid signature"
                                                                  : "Cartridge binary does not have a valid signature"));
    }
    words = rom_rd32 (fptr, pos + 8);
    if ((words < 1) || (words > ((page == V32_PAGE_BIOS) ? 1024U * 1024U : 128U * 1024U * 1024U)))
    {
        return (rom_fail (filename, what, (page == V32_PAGE_BIOS)
                ? "BIOS binary does not have a correct size (from 1 word up to 1M words)"
                : "Cartridge program ROM does not have a correct size (from 1 word up to 128M words)"));
    }
    pos = pos + 12 + (uint64_t) words * 4;
    if (pos > filebytes)
    {
        return (rom_fail (filename, what, "Unexpected end of file reading the program ROM"));
    }

    // video ROM: "V32-VTEX", width, height, then width*height pixels; each way up to 1024
    *texwords = 0;
    for (i = 0; i < w[0]; i = i + 1)
    {
        uint32_t  wide = 0;
        uint32_t  high = 0;

        if ((pos + 16 > filebytes) || (rom_rd32 (fptr, pos) != 0x2D323356) || (rom_rd32 (fptr, pos + 4) != 0x58455456))
        {
            return (rom_fail (filename, what, (page == V32_PAGE_BIOS) ? "BIOS texture does not have a valid signature"
                                                                      : "Cartridge texture does not have a valid signature"));
        }
        wide = rom_rd32 (fptr, pos + 8);
        high = rom_rd32 (fptr, pos + 12);
        if ((wide < 1) || (wide > 1024) || (high < 1) || (high > 1024))
        {
            return (rom_fail (filename, what, (page == V32_PAGE_BIOS)
                    ? "BIOS texture does not have correct dimensions (from 1x1 up to 1024x1024 pixels)"
                    : "Cartridge texture does not have correct dimensions (1x1 up to 1024x1024 pixels)"));
        }
        pos = pos + 16 + (uint64_t) wide * high * 4;
        if (pos > filebytes)
        {
            return (rom_fail (filename, what, "Unexpected end of file reading a texture"));
        }
        *texwords = *texwords + wide * high;
    }

    // audio ROM: "V32-VSND", number of samples, the samples
    for (i = 0; i < w[1]; i = i + 1)
    {
        uint32_t  samples = 0;
        uint32_t  limit   = (page == V32_PAGE_BIOS) ? 1024U * 1024U : 256U * 1024U * 1024U;

        if ((pos + 12 > filebytes) || (rom_rd32 (fptr, pos) != 0x2D323356) || (rom_rd32 (fptr, pos + 4) != 0x444E5356))
        {
            return (rom_fail (filename, what, (page == V32_PAGE_BIOS) ? "BIOS sound does not have a valid signature"
                                                                      : "Cartridge sound does not have a valid signature"));
        }
        samples = rom_rd32 (fptr, pos + 8);
        if ((samples < 1) || (samples > limit))
        {
            return (rom_fail (filename, what, (page == V32_PAGE_BIOS)
                    ? "BIOS sound does not have a correct length (from 1 up to 1M samples)"
                    : "Cartridge sound does not have correct length (1 up to 256M samples)"));
        }
        total = total + samples;
        if ((page == V32_PAGE_CART) && ((total > limit) || (total < samples)))
        {
            return (rom_fail (filename, what, "Cartridge sounds contain too many total samples (Vircon SPU only allows up to 256M total samples)"));
        }
        pos = pos + 12 + (uint64_t) samples * 4;
        if (pos > filebytes)
        {
            return (rom_fail (filename, what, "Unexpected end of file reading a sound"));
        }
    }

    *nwords = words;
    return (TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// load_memory(): load data files from disk into page-appropriate location in memory
//
uint8_t  load_memory (uint32_t  page, int8_t *filename)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    FILE     *fptr                      = NULL;
    int32_t   count                     = 0;
    int32_t   index                     = 0;
    uint32_t  offset                    = 0x00000000;
    uint32_t  num_vtex                  = 0x00000000;
    uint32_t  num_vsnd                  = 0x00000000;
    uint32_t  vsnd_start                = 0x00000000;
    uint32_t  data_size                 = 0x00000000;
    uint32_t  rom_words                 = 0;
    uint32_t  rom_ntex                  = 0;
    uint32_t  rom_nsnd                  = 0;
    uint32_t  rom_texwords              = 0;
    uint32_t  data                      = 0x00000000;
    uint32_t *checksum                  = NULL;
    size_t    size                      = 0;
    uint32_t  len                       = 0;
    uint8_t   chk                       = FALSE;
    uint8_t   result                    = FALSE;
    vtex_t   *vptr                      = NULL;

    if (filename                       != NULL)
    {
        ////////////////////////////////////////////////////////////////////////////////
        //
        // Adjust offset to be at the start of the indicated page
        //
        offset                          = offset | (page << 28);

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Open indicated filename for reading
        //
        fptr                            = fopen (filename, "rb");
        if (fptr                       == NULL)
        {
            fprintf (stderr, "[ERROR] Could not open '%s' for reading\n", filename);
            exit    (FILE_OPEN_ERROR);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // A memory card file must carry the V32-MEMC signature (2 words) followed by
        // exactly 262144 words
        //
        if (page                       == V32_PAGE_MEMC)
        {
            uint32_t  sig0              = get_word (fptr);
            uint32_t  sig1              = get_word (fptr);

            if ((sig0                  != MEMC_SIGNATURE0) ||
                (sig1                  != MEMC_SIGNATURE1))
            {
                fprintf (stderr, "[ERROR:MEMC] '%s' is not a V32-MEMC memory card file\n", filename);
                fclose  (fptr);
                return  (FALSE);
            }
            if (get_filesize (filename) != (uint32_t) ((2 + MEMC_WORDS) * wordsize))
            {
                fprintf (stderr, "[ERROR:MEMC] '%s' has the wrong size for a memory card (expected %u bytes)\n",
                                 filename, (uint32_t) ((2 + MEMC_WORDS) * wordsize));
                fclose  (fptr);
                return  (FALSE);
            }
            fseek (fptr, 0, SEEK_SET);
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // A BIOS or cartridge is validated as the console does. Its page is as large as
        // the program ROM (the CPU sees nothing past it); the texture pixels that follow
        // are kept beyond that, outside the addressable range
        //
        if ((page == V32_PAGE_BIOS) || (page == V32_PAGE_CART))
        {
            if (rom_check (fptr, filename, page, &rom_words, &rom_ntex, &rom_nsnd, &rom_texwords) == FALSE)
            {
                fclose (fptr);
                return (FALSE);
            }
            fseek (fptr, 0, SEEK_SET);
            (memory+page) -> extra      = rom_texwords;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // Adjust size of region of memory
        //
        (memory+page) -> size           = (page == V32_PAGE_MEMC) ? MEMC_WORDS : rom_words;
        (memory+page) -> last_addr      = (memory+page) -> firstaddr;
        (memory+page) -> last_addr     += (memory+page) -> size;
        chk                             = alloc_memory (page);
        if (chk                        == TRUE)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // Adjust position in file to move beyond header data, based on page
            //
            switch (page)
            {
                case V32_PAGE_BIOS:
                case V32_PAGE_CART:

                    fseek (fptr, (0x16 * wordsize), SEEK_SET);

                    ////////////////////////////////////////////////////////////////////
                    //
                    // Here we  obtain the number of  textures and sounds
                    // within the CART, for memory allocation of the CART
                    // cart_regions array (and likely whatever soundarray
                    // is eventually concocted)
                    //
                    num_vtex            = get_word (fptr);
                    num_vsnd            = get_word (fptr);

                    ////////////////////////////////////////////////////////////////////
                    //
                    // While we are in the neighborhood, obtain the  vtex
                    // and vsnd offsets
                    //
                    get_word (fptr); // program ROM start (bytes)
                    get_word (fptr); // program ROM length (bytes)
                    get_word (fptr); // video ROM (VTEX) start (bytes)
                    get_word (fptr); // video ROM (VTEX) length (bytes)
                    vsnd_start          = get_word (fptr); // audio ROM (VSND) start (bytes)
                    get_word (fptr); // audio ROM (VSND) length (bytes)

                    ////////////////////////////////////////////////////////////////////
                    //
                    // CART: cart_vtex is an array of vtex_t's, each  one
                    // containing an array of region_t's.  These are used
                    // for   managing   the   regions  present   in   the
                    // potentially many CART  textures. Allocation of the
                    // vtex_t's and region_t's is done at the  load  time 
                    // of the CART, so that resources are  only allocated
                    // for what is needed based on the CART being loaded.
                    //
                    if (page            == V32_PAGE_CART)
                    {
                        size             = sizeof (vtex_t);
                        len              = num_vtex;
                        cart_vtex        = (vtex_t *) ralloc (size, len, FLAG_ZERO); 
                        vptr             = cart_vtex;
                        if (vptr        != NULL)
                        {
                            vptr -> qty  = num_vtex;
                        }
                    }

                    ////////////////////////////////////////////////////////////////////
                    //
                    // BIOS: bios_vtex is a  singular  vtex_t, containing
                    // an array of region_t's. This is used for  managing
                    // for   managing   the   regions  present   in   the
                    // single BIOS texture. Allocation of the vtex_t  and
                    // region_t's are done at the  load  time of the BIOS
                    // that resources are  only to be allocated for  what
                    // is needed based on the BIOS being loaded.
                    //
                    else if (page       == V32_PAGE_BIOS)
                    {
                        size             = sizeof (vtex_t);
                        len              = 1;
                        bios_vtex        = (vtex_t *) ralloc (size, len, FLAG_ZERO);
                        vptr             = bios_vtex;
                        vptr -> qty      = num_vtex;
                    }


                    ////////////////////////////////////////////////////////////////////
                    //
                    // Allocate memory for each texture  (to  have up  to
                    // 4096 regions)
                    //
                    for (index          = 0;
                         index         <  num_vtex;
                         index          = index + 1)
                    {
                        size            = sizeof (region_t);
                        len             = V32_REGIONS_PER_TEXTURE;

                        if (page       == V32_PAGE_CART)
                        {
                            vptr        = (cart_vtex+index);    
                        }
                        else if (page  == V32_PAGE_BIOS)
                        {
                            vptr        = bios_vtex;
                        }

                        vptr -> region  = (region_t *) ralloc (size, len, FLAG_ZERO);
                    }

                    ////////////////////////////////////////////////////////////////////
                    //
                    // we need to skip ahead to word 0x22 (BIOS and CART), this will
                    // contain the length of the VBIN section
                    //
                    fseek (fptr, (0x22 * wordsize), SEEK_SET);
                    data_size           = get_word (fptr);

                    if (page           == V32_PAGE_CART)
                    {
                        SYSPORTSET(CAR_NumberOfTextures, num_vtex);
                        SYSPORTSET(CAR_NumberOfSounds,   num_vsnd);
                        SYSPORTSET(CAR_Connected,        TRUE);
                        SYSPORTSET(CAR_ProgramROMSize,   data_size);
                        fprintf (debug, "[LOAD:CART] ");
                    }
                    else
                    {
                        fprintf (debug, "[LOAD:BIOS] ");
                    }
                    fprintf (debug, "#vtex: %u, #vsnd: %u, romsize: 0x%.8X\n",
                                    num_vtex, num_vsnd, data_size);
                    /*
                    if ((debugflag                 == TRUE) &&
                        (chk                       == TRUE) &&
                        (filename                  != NULL))
                    {
                        load_labels (filename, page, FLAG_NONE);
                    }
                    */
                    break;

                case V32_PAGE_MEMC: // we need to skip ahead to word ?? (check for value)
                    fprintf (debug, "[LOAD:MEMC]\n");
                    fseek (fptr, (0x02 * wordsize), SEEK_SET);
                    SYSPORTSET(MEM_Connected,        TRUE);
                    data_size           = MEMC_WORDS;
                    break;
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Continually read in words from the file until we reach EOF, placing
            // each read word in memory at the appropriate offset.
            //
            checksum                    = &(memory+page) -> checksum;
            for (index                  = 0;
                 index                 <  data_size;
                 index                  = index + 1)
            {
                data                    = get_word (fptr);
                SYSMEMSET(offset, data);
                if (!feof (fptr))
                {
                    *checksum           = *checksum + data;
                    offset              = offset + 1;
                }
                else
                {
                    if (page           == V32_PAGE_MEMC)
                    {
                        fprintf (stderr, "[ERROR:MEMC] ");
                    }
                    else
                    {
                        fprintf (stderr, "[ERROR:VBIN] ");
                    }
                    fprintf (stderr, "Unexpected end of file reading '%s' at offset 0x%.8X!\n",
                                     filename, offset);
                    exit (FILE_POSITION_ERROR);
                }
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Now at VTEX
            //
            if ((page                  == V32_PAGE_BIOS) ||
                (page                  == V32_PAGE_CART))
            {
                fprintf (debug, "[LOAD:VBIN] final offset is: 0x%.8X\n", offset);
                for (count              = 0;
                     count             <  num_vtex;
                     count              = count + 1)
                {
                    if (page           == V32_PAGE_CART)
                    {
                        vptr            = (cart_vtex+count);
                    }
                    else // BIOS
                    {
                        vptr            = bios_vtex;
                    }

                    ////////////////////////////////////////////////////////////////////
                    //
                    // populate VTEX attributes
                    //
                    get_word (fptr); // "V32_"
                    get_word (fptr); // "VTEX"
                    vptr -> wide        = get_word (fptr); // width
                    vptr -> high        = get_word (fptr); // height
                    vptr -> size        = vptr -> wide * vptr -> high;
                    vptr -> offset      = offset;

                    for (index          = 0;
                         index         <  vptr -> size;
                         index          = index + 1)
                    {
                        data            = get_word (fptr);
                        {
                            word_t  *texel = memory_texel (offset);
                            if (texel  != NULL)
                            {
                                texel -> raw = data;
                            }
                        }
                        if (!feof (fptr))
                        {
                            *checksum   = *checksum + data;
                            offset      = offset + 1;
                        }
                        else
                        {
                            fprintf (stderr, "[ERROR:VTEX] Unexpected end of file reading '%s' at offset 0x%.8X!\n",
                                             filename, offset);
                            exit (FILE_POSITION_ERROR);
                        }
                    }

                    fprintf (debug, "[LOAD:VBIN] final offset of vtex #%d is: 0x%.8X\n", count, offset);
                }

                ////////////////////////////////////////////////////////////////////////
                //
                // Sounds: each is a V32-VSND header (signature, number of samples)
                // followed by that many 32-bit stereo samples. They are not part of
                // CPU-addressable memory; they belong to the SPU (see spu.c).
                //
                if (page               == V32_PAGE_CART)
                {
                    spu_cart_insert (num_vsnd);
                }
                if (num_vsnd           >  0)
                {
                    uint32_t   filebytes  = get_filesize (filename);
                    uint32_t   nsamples   = 0;
                    uint32_t  *samples    = NULL;
                    size_t     got        = 0;
                    uint32_t   n          = 0;

                    fseek (fptr, (long) vsnd_start, SEEK_SET);
                    for (count          = 0;
                         count         <  num_vsnd;
                         count          = count + 1)
                    {
                        get_word (fptr); // "V32-"
                        get_word (fptr); // "VSND"
                        nsamples        = get_word (fptr);
                        if ((feof (fptr))                              ||
                            ((uint64_t) nsamples * 4 > (uint64_t) filebytes))
                        {
                            fprintf (stderr, "[ERROR:VSND] Bad sound #%d in '%s'\n", count, filename);
                            exit (FILE_POSITION_ERROR);
                        }
                        samples         = (uint32_t *) calloc ((nsamples > 0) ? nsamples : 1, sizeof (uint32_t));
                        if (samples    == NULL)
                        {
                            fprintf (stderr, "[ERROR:VSND] Out of memory loading sound #%d\n", count);
                            exit (STRING_ALLOC_FAIL);
                        }
                        got             = fread (samples, sizeof (uint32_t), nsamples, fptr);
                        if (got        != nsamples)
                        {
                            fprintf (stderr, "[ERROR:VSND] Unexpected end of file reading sound #%d of '%s'\n",
                                             count, filename);
                            exit (FILE_POSITION_ERROR);
                        }
                        for (n          = 0; n < nsamples; n = n + 1)
                        {
                            // samples are little endian in the file
                            uint8_t  *b = (uint8_t *) (samples + n);
                            samples[n]  = (uint32_t) b[0]         | ((uint32_t) b[1] << 8) |
                                          ((uint32_t) b[2] << 16) | ((uint32_t) b[3] << 24);
                        }
                        fprintf (debug, "[LOAD:VSND] sound #%d: %u samples\n", count, nsamples);
                        spu_sound_load ((page == V32_PAGE_BIOS) ? -1 : count, nsamples, samples);
                    }
                }
            }
            else
            {
                fprintf (debug, "[LOAD:MEMC] final offset is: 0x%.8X (%u)\n", offset, offset);
            }
            
            fclose (fptr);

            //
            // The card is loaded: from now on the controller keeps its file in step
            // (and may find that the card carries a live v32dev mailbox)
            //
            if (page                   == V32_PAGE_MEMC)
            {
                memc_attach (filename);
            }

            ////////////////////////////////////////////////////////////////////////////
            //
            // Operation deemed successful
            //
            result                      = TRUE;
        }
    }

    return (result);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// unload_memory(): unload data files from memory
//
uint8_t  unload_memory (uint32_t  page)
{
    ////////////////////////////////////////////////////////////////////////////////////
    //
    // Declare and initialize variables
    //
    int32_t   index                 = 0;
    linked_l *ltmp                  = NULL;
    linked_l *ltmp2                 = NULL;
    uint8_t   result                = TRUE;

    switch (page)
    {
        case V32_PAGE_BIOS:
            if (bios_vtex != NULL)
            {
                rfree (bios_vtex -> region);
                rfree (bios_vtex);
                bios_vtex = NULL;
            }
            break;

        case V32_PAGE_CART:
            SYSPORTSET(CAR_Connected,        FALSE);
            SYSPORTSET(CAR_NumberOfTextures, 0);
            SYSPORTSET(CAR_NumberOfSounds,   0);
            SYSPORTSET(CAR_ProgramROMSize,   0);
            spu_cart_remove ();
            if (cart_vtex != NULL) // a CART without textures has no cart_vtex
            {
                for (index          = 0;
                     index         <  cart_vtex -> qty;
                     index          = index + 1)
                {
                    rfree ((cart_vtex+index) -> region);
                }
                rfree (cart_vtex);
                cart_vtex           = NULL;
            }
            break;

        case V32_PAGE_MEMC:
            memc_detach ();        // write MEMC data back to its file
            SYSPORTSET(MEM_Connected,        FALSE);
            break;

        default:
            fprintf (verbose, "[unload_memory] invalid page %u specified\n", page);
            result                  = FALSE;
            break;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // If a valid unload transaction, clear the allocated memory and reset the
    // page attributes to reflect the unloaded state
    //
    if (result                     == TRUE)
    {
        rfree ((memory+page) -> data);
        (memory+page) -> data       = NULL;
        (memory+page) -> size       = 0;
        (memory+page) -> extra      = 0;
        (memory+page) -> last_addr  = (memory+page) -> firstaddr;
    }

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // clear the established labels and offsets from the label list pertaining to
    // the page just unloaded
    //
    if ((page                      == V32_PAGE_BIOS) ||
        (page                      == V32_PAGE_CART))
    {
        ltmp                        = lpoint;
        while (ltmp                != NULL)
        {
            ////////////////////////////////////////////////////////////////////////////
            //
            // if the node pertains to the unloaded page, grab and free it
            //
             if (ltmp -> type       == page)   
            {
                ltmp2               = ltmp;
                ltmp                = ltmp -> next;
                ltmp2               = list_grab (&lpoint, ltmp2);
                rfree (ltmp2);
            }
            else
            {
                ltmp                = ltmp -> next;
            }
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // if a BIOS ASM debug file was specified, clear it
        //
        if ((biosasmdebug          != NULL) &&
            (page                  == V32_PAGE_BIOS))
        {
            rfree (biosasmdebug);
            biosasmdebug            = NULL;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // if a CART ASM debug file was specified, clear it
        //
        if ((cartasmdebug          != NULL) &&
            (page                  == V32_PAGE_CART))
        {
            rfree (cartasmdebug);
            cartasmdebug            = NULL;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // if a BIOS C debug file was specified, clear it
        //
        if ((bioscdebug            != NULL) &&
            (page                  == V32_PAGE_BIOS))
        {
            rfree (bioscdebug);
            bioscdebug              = NULL;
        }

        ////////////////////////////////////////////////////////////////////////////////
        //
        // if a CART C debug file was specified, clear it
        //
        if ((cartcdebug            != NULL) &&
            (page                  == V32_PAGE_CART))
        {
            rfree (cartcdebug);
            cartcdebug              = NULL;
        }
    }

    return (result);
}
