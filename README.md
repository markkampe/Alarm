# Arduino house monitoring board

Old-school alarm systems are simple: buy a $80 box, hook it up to a
control panel, a battery, the bells, and some NC-sensor loops ... 
and you've got it.  But when the system won't arm (because something
is open) it can take a while to find the offending sensor.  Similarly,
if the alarm was triggered, I would like to know from where.  This
is the situation we found ourselves in when we bought our first home
(in 1978).

I had long wanted an alarm status panel that showed, on a house map, the
status of every instrumented door/window/sensor, and (if the system had
been armed) which sensors had been triggered.

   - Today I could have a much better system (e.g. based on Ring) for $400 
     and two days of work (as opposed much more money and months of work
     that have gone into this system).

   - Even if I insisted on building everything myself, an alarm system built on 
     a newer SoC (e.g. Raspberry Pi, BeagleBone) the interface could have been a
     web-server with maps, status indications and an event log.

But I grew up in the 60's and had wanted to build this panel for a long time.
*Damn the obsolescence!  Full speed ahead!*
I am not a hardware guy, but I eventually decided that the most appropriate
technology was an Arduino and a bunch of shift registers:
   - dozens of sensors (NO or NC) are each connected to a shift register input
   - corresponding (tri-color LED) indicators are each connected to a shift register output
   - digital outputs for four zone status relays (entries, exterior, breakage, interior)
     provide the NO/NC loops for the (vanilla) alarm controller.
   - analog (low-asserted control) inputs for *system armed* and *zone enables*.

The indicator lights show the status of each sensor:
   - green: closed/OK (flashing means it is armed)
   - yellow: open (flashing means it has been triggered)
   - red: armed and open (flashing meaning it has triggered the alarm)

The relays (which can be configured for high-asserted or low-asserted) are only
on when a sensor (in that zone) is not in the closed/OK state.  This is to minimize
current-draw and heat.

## Code Overview

### Excuses

This was designed for old-school Arduinos, and so supports only a pathetic 
amount of stack/data.  This forced me to do unnatural gymnastics to:
   - put most of my initialized data into the (in EPROM) code
   - minimize stack depth (calls, parameters, locals)
   - minimize table sizes

This also led me to compile-time-disable DEBUG code that generates 
lots of diagnostic and per-event information, and implements a few
debug commands.

I built this in the Arduino IDE, which requires a somewhat *unique*
source structure.  To build this project, I download this repo and 
rename it to be `$HOME/Arduino`.

### Main Program

`Alarm/Alarm.ino` is the main program.
All it does is 
   - `setup`: instantiate the `ShiftRegister` controllers, `SensorManager` and `ControlManager`.
   - `loop`:
      - on first call run a lamp test (`Sensor.lampTest`)
      - sample the status of each sensor and update the indicators (`SensorManager.sample`)
      - check for changes in zone enables and armed status (`SensorManager.arm`)

### Configuration

`libraries/Config/Config.h` defines the data structures that configure the program:
   - the output pins that control each shift register cascade
   - the index and sense of each sensor, and the corresponding indicator indices
   - the sense and scale of each analog input

`libraries/Config/Config.cpp` initializes all of this information, and uses 
mnemonic defines and heavy commenting to describe each provided value. 
It also includes methods to return this information (which is in 
EPROM code-space rather than data).

### Shift Register Controllers
`libraries/ShiftReg/ShiftReg.h` defines `OutShifter` and `InShifter` sub-classes
with methods to set or get the value at a particular index.

`libraries/ShiftReg/ShiftReg.cpp` implements these methods, and includes the
`read` and `write` methods that actually talk to the shift registers to read 
or write the entire contents of an input or output cascade.

### Control Inputs
`libraries/Control/Control.h` and `libraries/Control/Control.cpp` implements
the code to initialize the AIO input pin programming and the `read` method
which reads and returns their status as a byte of status bits.

### Sensor Manager
This is the module that contains the most interesting code:
`libraries/Sensor/Sensor.h` and `Sensor.cpp`.

The constructor allocates arrays for per-sensor status and debounce info,
and programs the (zone status) digital output pins.

The method that does the most interesting work is `sample`, which runs through
all the sensors and:
   - calls the `InShifter.get` method to get the current status,
      - debouncing (make sure a value lasts long enough before accepting it)
      - defibrillating (checking for rapidly oscillating values that indicate
        a sensor failure, and could burn out a zone status relay)
   - update the sensor status LEDs, with color and blinking based on the sensor and system states

Each indicator has a combination of color (RED, YELLOW, GREEN) and illumination (OFF, ON, SLOW-FLASH, FAST-FLASH).  
The `setLed` method sets the desired color and illumination state for a particular indicator.
The `update` method looks at this (per indicator) status, and sets the appropriate
current RED and GREEN LED states, and uses the OutShifter to make it so.
