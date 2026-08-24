# Smart Chess Board

A physical chess board that can detect the position of chess pieces in real time using Hall effect sensors and provide visual feedback through RGB LEDs.

The project combines custom electronics, firmware and a 3D printed enclosure to create a chess board that can understand what is happening on the board without using mechanical switches.

## Current Status

The PCB design is complete and the first board is being prepared for production and assembly.

The project is still in development. Some features described below are planned and will be added as the hardware and firmware develop.

Once I assemble and test the first board, I will add more information to this repository, including photos, test results and a much more detailed assembly guide.

## Features

* Real-time piece detection using Hall effect sensors
* 128 Hall effect sensors
* RGB lighting under the chess squares
* Custom PCB designed specifically for the board
* Magnetic chess pieces using neodymium magnets
* Firmware developed for the board
* Planned Stockfish integration
* Planned connection to online chess platforms

## How It Works

Each chess piece has a small magnet in its base. Hall effect sensors placed underneath the board detect the magnetic field and determine whether a piece is present on a square.

The board uses 128 sensors connected through 74HC165 shift registers. This allows the microcontroller to read a large number of sensors without requiring a separate input pin for every sensor.

RGB LEDs are placed underneath the squares and can be used to show things such as possible moves, selected pieces, warnings and other information.

The main idea is to keep the playing surface completely clean while putting all of the electronics underneath it.

## Hardware

The main components currently used in the design are:

* Custom PCB designed in KiCad
* 128x HAL2041SO Hall effect sensors
* 16x 74HC165 shift registers
* SK6812 / WS2812B RGB LEDs
* 100nF capacitors
* 10kΩ and 330Ω resistors
* Chess pieces with neodymium magnets

The exact components and values may change after the first prototype is assembled and tested.

## Repository Structure

```text
CAD/              3D models and enclosure design
Firmware/         Board firmware
PCB/              KiCad PCB design files
Production/       Manufacturing and production files
Smart chess board Additional project files
README.md         Project documentation
```

## Assembly

The first version of the board has not been fully assembled yet, so the complete assembly guide is not available at the moment.

After I build the first board, I will add a detailed guide covering:

* PCB assembly
* Hall sensor placement
* LED installation
* Wiring and power
* Magnet placement in the chess pieces
* Firmware installation
* Initial testing and calibration
* Common problems and fixes

I want the guide to be based on the actual first build rather than documenting steps that have not been tested yet.

## Future Plans

The project is still being developed, and more features will be added after the first working board is finished.

Some of the planned improvements include:

* Stockfish integration
* Better move detection and validation
* More RGB lighting effects
* Online chess integration
* Calibration tools
* Improved enclosure
* More detailed documentation

The first assembled board will be an important milestone because it will show which parts of the current design work as expected and which parts need to be improved.

## Libraries and Tools

* KiCad
* PlatformIO
* FastLED
* Stockfish

## Project Status

PCB design: Complete
CAD design: In progress
Firmware: In progress
First board assembly: Upcoming
Detailed assembly guide: Coming after the first build

This project is being developed step by step. The documentation will be updated as the physical board is built, tested and improved.
