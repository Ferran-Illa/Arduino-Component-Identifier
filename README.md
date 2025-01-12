# Arduino Component-Identifier
Arduino Code for a component identifier prototype. Divided into a library - MultiTesterLib, and a .ino Main file to ease uploading into the microcontroller.

To upload the program to the Arduino board, set the folder MultiTesterLib into the Arduino Library directory, as with any Arduino library. Then open and upload the Main.ino code to the board through Arduino IDE.

This code is designed for the **ATMEGA328PB** Microcontroller. Inspired on the (much better) code by gojimmy pi - *https://github.com/gojimmypi/ComponentTester*
## Supported Components:
Currently supports basic electronic components:
- Resistances $150\Omega - 5M\Omega$ with 5% accuracy. Satisfactory measures down to $1\Omega$.
- Capacitors $50nF - 1mF$. 10% accuracy. Big Capacitors take a while.
- Inductances $50\mu H - 5mH$. 40% accuracy.
- Diodes
- BJT
- MOSFET

# General Header Structure

*config.h* stores basic constants and callibrated/adjusted values (component values, pins, ...).

*common.h* hold the classes, includes, and some definitions shared between all files.

*functions.h* has the function definitions.

Do not hesitate to contact me if any doubts arise - *ferran.illa1011@gmail.com*
