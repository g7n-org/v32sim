#!/usr/bin/env bash
##
## gamepad_gadget.sh
##
##############################################################################

##############################################################################
##
## Exit immediately on error (non-zero status)
##
set -e

##############################################################################
##
## Load the needed module
##
modprobe libcomposite

##############################################################################
##
## Create entry in usb_gadget (v32kbd)
##
cd       /sys/kernel/config/usb_gadget
mkdir -p v32kbd
cd       v32kbd

##############################################################################
##
## IDs (Linux Foundation for dev/testing; for products, use your own VID/PID)
##
echo '0x1d6b'                                   >  idVendor
echo '0x0104'                                   >  idProduct
echo '0x0100'                                   >  bcdDevice
echo '0x0200'                                   >  bcdUSB

##############################################################################
##
## Addition gadget attributes
##
STRING="strings/0x409"
mkdir -p ${STRING}
echo "deadbeef0001"                             >  ${STRING}/serialnumber
echo "Raspberry Pi"                             >  ${STRING}/manufacturer
echo "USB Gamepad"                              >  ${STRING}/product

##############################################################################
##
## Configuration
##
CONFIG="configs/c.1/strings/0x409"
mkdir -p ${CONFIG}
echo "Config 1"                                 >  ${CONFIG}/configuration
echo 120                                        >  configs/c.1/MaxPower

##############################################################################
##
## HID function: 7 buttons + 2 axes (X,Y), 4-byte report
##
FUNCTION="functions/hid.usb0"
mkdir -p ${FUNCTION}
echo 0                                          >  ${FUNCTION}/protocol
echo 0                                          >  ${FUNCTION}/subclass
echo 4                                          >  ${FUNCTION}/report_length

# HID report descriptor (4 bytes/report: [buttons(8)] [pad(8)] [X(8)] [Y(8)])
# Generic Desktop Joystick, 8 buttons, 8-bit pad, 8-bit X, 8-bit Y (0..255)
echo -ne \
"\x05\x01\x09\x04\xa1\x01\x05\x09\x19\x01\x29\x08\x15\x00\x25\x01\x95\x08\x75\x01\x81\x02\x95\x01\x75\x08\x81\x03\x05\x01\x09\x30\x09\x31\x15\x00\x26\xff\x00\x75\x08\x95\x02\x81\x02\xc0" \
> functions/hid.usb0/report_desc

ln -s functions/hid.usb0 configs/c.1/

# Bind to a UDC (USB Device Controller)
echo "$(ls /sys/class/udc | head -n1)" > UDC

# Create a friendly symlink to the character device if present
[ -e /dev/hidg0 ] && udevadm settle || true

echo "Gamepad gadget enabled."

exit 0
