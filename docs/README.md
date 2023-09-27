# Documentation

Currently not organized, but you may find something useful in this folder.

## Overview:
* This project is using an ESP32. I happen to be using one that came with a great GPIO extension board with built-in power supply, although I've only been using USB power directly to the ESP32 so far.
* The long module at the bottom right of the left picture is the SLIC module that handles the phone, with off-hook detection, audio in and out pins and manages the local loop current and the ringer voltage. 
* The blue square module at the bottom left is the DTMF decoder module. It provides a flag when ready to read a 4-bit value for the decoded digit. 
* The small 8-pin chip is a LM385 op-amp to finesse the audio in and out.
* The small 10-pin green module is a level shifter from 5V to 3.3V or vice-versa. The DTMF module runs at 5V and the ESP32 runs at 3.3. 
* The red module is a single addressable RBG LED for visual status.

<p float="left">
  <a href="modules and chips.jpg"><img src="modules and chips.jpg" width="49%" /></a>
  <a href="prototype on breadboard.jpg"><img src="prototype on breadboard.jpg" width="49%" /></a>
</p>

## Prototype schematic v0.1 
(click image for PDF or look in folder)
[![Prototype schematic v0.1](RetroPhone%20schematic%20v0.1.PNG)](RetroPhone%20schematic%20v0.1.pdf)
