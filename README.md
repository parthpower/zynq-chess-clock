# zynq-chess-clock
Dirt simple chess clock project on Xilinx Zynq. Why? I mean, why not!?

# Please Donate

This code is absolutely free. I made it in hope to help people on some boilerplate Zynq code with GPIO and Timer interrupts. If this project has helped you in any way, please give back to the people who really needs it. Even a dollar matters.

Have a look at https://www.giveindia.org/certified-indian-ngos.


# How to Build?
```text
Steps to build:

Viviado:
1. Create a vivado project for ZedBoard
2. On the top menu click on Tools->Run Tcl Script. and select design_1.tcl attached here.
3. Create HDL wrapper for the block design.
4. Build Bitstream.
5. Export Hardware, and launch SDK

SDK:
1. Create empty application project.
2. Use main.c attached here.

You might have to change LEFT_BUTTON, RIGHT_BUTTON and RESET_BUTTON for proper button settings. I didn't verify for ZedBoard. I tried it out on a PYNQ-Z1

Board:
Plug 7 segment into JA and JB (the two Pmods in front)
```
