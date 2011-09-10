
Derived from the mbed_samples blinker01.

The lpc1343 has the fast gpio thing, by default.  Didnt use it that way
with the mbed 17xx.

The led is on port p0.7.

The way the mask thing works is the lower bits of the address you write
to are anded with the lower data bits and those are the only bits
modified on the output.

So port 0.7 is bit 7.  So we want the address to allow port bit 7

..76543210
0010000000

Since these mask registers are 32 bit registers/addresses we need
to be on four byte boundaries, so add two more zeros to the end

..76543210
001000000000

0010 0000 0000
which is 0x200.

add the 0x200 to the GPIO0 base address and you get 0x50000200

The sample program writes 0x00000000 or 0xFFFFFFFF to the port
only 12 bits or fewer (8) matter depending on the port.  When masked
the data bit from the write that lines up with the masked bit from the
address changes the output port, other bits are unchanged.

..76543210
0010000000  <-- mask turned to address
0000000000  <-- data
uu0uuuuuuu  <-- output bit written a 0 other bits unchanged

..76543210
0010000000  <-- mask turned to address
1111111111  <-- data
uu1uuuuuuu  <-- output bit written a 1 other bits unchanged

