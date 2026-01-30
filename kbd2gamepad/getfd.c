//
// getfd.c - get a file descriptor  (fd) for use with kbd/console ioctls.
//           Different   methods   are   tried  because  the  opening  of
//           /dev/console will fail  if  an  X session  has  taken  place
//           (as  it does  a  chown  on /dev/console).
//
////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include "nls.h"
#include "getfd.h"

static int32_t  is_a_console (int32_t  fd)
{
    int8_t  arg   = 0;
    return ((ioctl (fd, KDGKBTYPE, &arg) == 0) &&
           ((arg == KB_101) ||
            (arg == KB_84)));
}

static int32_t  open_a_console (const int8_t *filename)
{
    int32_t  fd  = 0;

    ////////////////////////////////////////////////////////////////////////////////////
    //
    // for ioctl purposes an fd is needed. Permissions do not matter.
    //
    // NOTE: setfont:activatemap() performs a write.
    //
    fd           = open (filename, O_RDWR);
    if ((fd     <  0) &&
        (errno  == EACCES))
    {
        fd       = open (filename, O_WRONLY);
    }

    if ((fd     <  0) &&
        (errno  == EACCES))
    {
        fd       = open (filename, O_RDONLY);
    }

    if (fd      <  0)
    {
        return (-1);
    }

    if (!is_a_console (fd))
    {
        close (fd);
        return (-1);
    }

    return (fd);
}

int32_t  getfd (const int8_t *filename)
{
    int32_t  fd   = 0;
    if (filename == NULL)
    {
        fd        = open_a_console (filename);
        if (fd   >= 0)
        {
            return (fd);
        }

        fprintf (stderr, _("Couldnt open %s\n"), filename);
        exit (1);
    }

    fd            = open_a_console ("/dev/tty");
    if (fd       >= 0)
    {
        return (fd);
    }

    fd            = open_a_console ("/dev/tty0");
    if (fd       >= 0)
    {
        return (fd);
    }

    fd            = open_a_console ("/dev/vc/0");
    if (fd       >= 0)
    {
        return (fd);
    }

    fd            = open_a_console ("/dev/console");
    if (fd       >= 0)
    {
        return (fd);
    }

    for (fd       = 0;
         fd      <  3;
         fd       = fd + 1)
    {
        if (is_a_console (fd))
        {
            return (fd);
        }
    }

    fprintf (stderr, _("Couldnt get a file descriptor referring to the console\n"));

    exit (1);        /* total failure */
}
