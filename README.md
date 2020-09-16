# Arduino house monitoring board

Alarm systems are simple: buy a $80 box, hook it up to a control panel,
a battery, the bells, and some NC-sensor loops ... and you've got it.
But when the system won't arm (because something is open) it can take
a while to find the offending sensor.  Similarly, it the alarm was triggered,
I would like to know from where.

I had (for decades) wanted an alarm status panel that showed, on a house map,
the status of every instrumented door/window/senso, and (if the system had
been armed) which sensors had been triggered.  I am not a hardware guy, but
I eventually decided that the most appropriate technology was an Arduino and 
a bunch of shift registers:
   - dozens of sensors (NO or NC) are each connected to a shift register input
   - corresponding (tri-color LED) indicators are each connected to a shift register output
   - digital outputs for four zone status relays (entries, exterior, breakage, interior)
   - analog (low-asserted control) inputs for *system armed* and *zone enables*.

The indicator lights show the status of each sensor:
   - green: closed/OK (flashing means it is armed)
   - yellow: open (flashing means it has been triggered)
   - red: armed and open

The relays (which can be configured for high-asserted or low-asserted) are only
on when a sensor (in that zone) is not in the closed/OK state.  This is to minimize
current-draw and heat.

If I were cooler, and had been designing this in the 21st century, I probably 
would have made it a web-server with status indications and an event log.  But
I grew up in the '60s and I always wanted this panel.  In fact, it is so old
that it has holes and lights for window sensors that no longer exist 
(due to remodels).

## Code Overview

### Excuses

This was designed for old-school Arduinos, and so supports only a pathetic 
amount of stack/data.  This forced me to do unnatural gymnastics to:
   - put most of my initialized data into the (in EPROM) code
   - minimize stack depth (calls, parameters, locals)
   - minimize table sizes

This also led me to compile-time-disable DEBUG code that generates 
lots of diagnostic and per-event information, and a few debug commands.

I built this in the Arduino IDE, which requires a somewhat unique
source structure.  To build this project, I download this repo and 
rename it to be `$HOME/Arduino`.

### Main Program

`Alarm/Alarm.ino` is the main program.
All it does is 
   - `setup`: instantiate the shift register controllers, `SensorManager` and `ControlManager`.
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
`This is the module that contains the most interesting code:
`libraries/Sensor/Sensor.h` and `Sensor.cpp`.

The constructor allocates arrays for per-sensor status and debounce info,
and programs the (zone status) digital output pins.

The method that does the most interesting work is `sample`, which runs through
all the sensors and:
   - calls the `InShifter.get` method to get the current status,
      - debouncing (make sure a value lasts long enough before accepting it)
      - defibrillating (checking for rapidly oscillating values that indicate
        a sensor failure, and could burn out a zon status relay)
   - update the sensor status LEDs, with color and blinking based on the sensor and system states

Each indicator has a combination of color (RED, YELLOW, GREEN) and illumination
(OFF, ON, SLOW-FLASH, FAST-FLASH).  
The `setLed` method specifies the color and illumination for a particular indicator.
The `update` method looks at the nominal status for each LED, sets the appropriate
current RED and GREEN LED states, and uses the OutShifter to make it so.
