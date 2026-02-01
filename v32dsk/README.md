# v32dsk

Vircon32 driver/library for jury-rigged storage2memcard peripheral hack.

## IMPLEMENTATION

The current thought on  how to implement this for Vircon32  is to use the
standard  MEMCARD interface  in Vircon32,  reading and  writing to  it as
normal from the Vircon32 side of things.

However,  that  particular  MEMCARD  file will  be  monitored  externally
(perhaps using  the Linux kernel's `inotify`  functionality), and thereby
retrieve  or overwrite  the contents  of that  MEMCARD file  so that  the
Vircon32 environment  can access far  more information than  is typically
available to it.

Actual schemes could vary: the v32dsk  header on the MEMCARD could make a
request for a file (which would  mean establishing some sort of directory
structure). Perhaps perform a metadata transaction leading into an actual
data transaction.

In the end,  the developer will need  to make the v32dsk  calls (once per
frame).

Possible  rethinking of  transitioning  this to  a  more generic  `v32io`
system, of  which disk  access is  a component.  Then other  things, like
networking, could be another module in that system.
