The first task of this final project consisted of interacting with a ChipKIT Uno32 board (PIC32MX320F128H microcontroller) using only its JTAG port. We interact with the port using an Arduino Uno Board which then communicates through UART with the computer where we insert commands corresponding to the different implemented interactions: getting the ID code of the microcontroller, turning on the internal led LD5, turning the led off and getting the state of a button connected to one of Chipkit's pins.

The JTAG port for boundary scan testing consists of 4 pins: TMS (Test Mode select), TDI (Test Data Input), TDO (Test Data Output) and TCK (Test Clock). The location of these pins on the board can be found consulting the board's reference manual [2].

We define the Arduino's pins connected to TMS, TDI and TCK as outputs and the pin connected to the TDO as a digital input (the TDI defined in the arduino source code is the output which connects to the BST TDI). 

Communication with the board is done exclusively through these pins. 

Setting the TMS pin high or low and completing clock cycles (setting the clock high, waiting 1ms and then low, waiting 1ms again), enables us to navigate the TAP controller states. 

The first section of the source code establishes the asynchronous communication with the monitoring computer. It's followed by a section with functions for facilitating the TAP states navigation and finally the main JTAG functions:

- Shifting in an instruction
- Getting the ID code
- Turning the led on or off
- Getting the button state 


# General method and getting the id code

The BST infrastructure is controlled by shifting in instructions to the instruction register (IR) which then controls what the data register (DR) consists of.

The instructions and test vectors are introduced into the TDI pin and we read the values shifted out by the boundary scan chain in the TDO.

Information on the different instructions available, the size of the instruction register and the various data registers are found on the microcontroller's BSDL (Boundary-Scan Description Language) file, which is usually made available by the MCU's manufacturer, as was indeed the case for this one. It can be found here http://ww1.microchip.com/downloads/en/DeviceDoc/PIC32MX320F128H.bsdl

The instructions consist of 5 bits and the instruction for the ID code to be made available in the data register is (00001), as specified in the BSDL file.

For shifting in an instruction, we go the the shift-ir state (soft reseting, which is done by setting TMS and completing 5 clock cycles, helps ensure the we know where we start), begin by setting the TDI as the first bit to shift (starting on the LSB) and then make the first shift by exiting the state onto itself (TMS=0). The shifts are done upon exiting the state, so we do this 4 times and the 5th and last shift is done by setting TMS=1, which will get us the the exit-1 ir state.

For getting the ID itself, we proceed to the "Shift-DR" state through the same process (set TMS to the adequate value and complete clock cycles to change TAP state).

This time, we have to shift 31 values before exiting the "Shift-DR" state, as the ID code consists of 32 bits.

This time, we don't worry about the values of TDI and instead only store the TDO values in 32 bit variable, which is then printed and correctly contains the ID code of the MCU.

# Turning the internal led LD5 led on and off

For setting values in the pins of the MCU, we give the "EXTEST" instruction (00110). This makes the data register consist of the boundary scan cells of the MCU.

The instruction and the number of cells present in the register, is also information present in the BSDL file. 

Since there are 148 cells, we created a 19-element array of bytes (152 bits in total) for storing the 148 values which are shifted out and also to store the values we want to shift in.

Following the procedure described in the previous section, we must only know in what position the led lies. The BSDL file has the 148 cells labeled from 0 to 147. Consulting the Chipkit's reference manual, we find out the led LD5 is the pin labeled "RF0". There are 3 cells corresponding to this pin. One for its input, n=18, another for its output, n=19 and another labeled "control", n=20, for setting the wanted mode.

Since we want to control the state of the led, which is fed by the output of the cell, we control it by setting the output cell in n=19 to the wanted state (high for turning the led on and low for turning it off) and setting the control bit (n=20) high.

The rest of the values in the 148 bit test vector array are simply 0.

The first value to be shifted is that corresponding to n=0, so, to turn on the led, we set TDI high before the 20th and 21st shifts (n=19 and n=20) and for turning it off, we only set the TDI high before the 21st shift, while for the rest of the shifts it's set low.

# Getting the state of the button

The button is connected to pin 29 of the ChipKit board. This corresponds to pin 63 of the PIC32, labeled "RE3". Consulting the BSDL file again, this pin also has 3 corresponding cells: for its input, output and control, n=3, n=4 and n=5, respectively.

We again, give the instruction for "EXTEST" and proceed to shift 148 values of the data register, this time not worrying about what we set in the TDI (all zeros) and simply storing the values read in the TDO.

Knowing the state of the button consists of knowing the input of pin. Since this input information, corresponds to n=3, we know this will be the 4th value read in the TDO (after 3 shifts). 

# Important notes

- As mentioned, the shifting of the registers occurs upon EXITING the shift state.
- The first value of the TDO is read BEFORE the first shift, that is, at the same time of setting the first value of the TDI.
- The boundary scan cells which make up the data register during the EXTEST mode are listed in the BSDL file from n=0 to n=147. One could think the n=0 cell is the first in contact with the TDI and n=147 would be the end of the chain, with the TDO. In this case, the value we intend to impose on the cell n=147 would be the first to be shifted. The opposite is true. The chain begins at n=147 and ends at n=0 at the TDO. Thus, the the first value to be shifted is that which we want to impose (or read) on n=0.