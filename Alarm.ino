/**
 * This program is not an alarm proper ... as alarms
 * are easy to buy.  Rather, this program:
 *	is configurable for sensors, indicators and relays
 *	supports up to 8 independently enableable zones
 *	watches, debounces, and normalizes the sensors
 *	reflects their status on per-sensor indicators
 *	reflects their status in per-zone relays
 * 
 * The indicators are for human use.
 * The relays are input to a traditional alarm system.
 *	which does not have to support multiple zones.
 *
 * NOTES on the code:
 *   The interesting code is in library/Sensor/Sensor.cpp
 *   The configuration stuff is in library/Config/Config.cpp
 *   The class design has been perverted by the fact that an
 *       ardunio has only 1024 bytes of RAM!
 */
#include <Config.h>
#include <Shiftreg.h>
#include <Zone.h>
#include <Sensor.h>
#include <Control.h>
#include <stdio.h>
    
const  int ledPin = 13; // on-board status LED

SensorManager *mgr;	// sensor collection manager
ZoneManager *zones;     // zone collection manager
ControlManager *ctrls;  // control collection manager
int debug = 0;          // enables serial port logging

// glue to enable printf to write to the serial port
static FILE uartout = {0};
static int uart_putchar( char c, FILE *stream ) {
    Serial.write(c);
    return( 0 );
}

// run-time diagnostic options
void doDebug( char c ) {
  switch( c ) {
	case 'c': case '1':  
		debug = 1;
		break;

	case 'f': case 'a': case '2':
		debug = 2;
		break;

	case 'n': case '0':  
		debug = 0;
		break;

	case 'l':	// one minue lamp test
		mgr->lampTest(true);
		return;
	}
	printf("D=%d\n", debug);
}

void freemem() {
}

/**
 * configure the pins and initialize the resource managers.
 */
void setup() {                
	// initialize serial port for diagnostics
	Serial.begin(9600);
	fdev_setup_stream( &uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE );
	stdout = &uartout;

        freemem();

	// set default debug level
	doDebug('2');

	// initialize the LED for status
	pinMode(ledPin, OUTPUT);  

	// get our configuration information
	Config *cfg = new Config();

	// configure the shift register cascades
	InShifter *input = new InShifter( cfg->input->num_regs,
				cfg->input->data, cfg->input->clock, cfg->input->latch );
	OutShifter *output = new OutShifter( cfg->output->num_regs,
				cfg->output->data, cfg->output->clock, cfg->output->latch );

	// allocate a zone manager for known zone relays
	zones = new ZoneManager( cfg );

	// allocate a sensor manager for the known sensors
	mgr = new SensorManager( cfg, input, output, zones );

        // allocate a control manager for the defined input controls
        ctrls = new ControlManager( cfg );
}

/** arduino main loop
 *  on start-up we run lamp tests
 *  after that
 *    sample the sensor status
 *    run the indicators through one duty cycle
 *    maintain an idle pattern on the board LED
 *      (2hz when armed, 1hz when not armed)
 */
void loop() {
	static unsigned char prevArm = 0;	// we keep track of system armed status
	static int prevFlash = 0;	// we keep track of the progress pattern

	/*
	 * initially the indicators are set by the lamp tester,
	 * but after that completes, they are set based on the
	 * status of their respective sensors.
	 */
	if (!mgr->lampTest(false))
		mgr->sample();  // sample does all the work

	mgr->update();          // update the LEDs

	/*
	 * when we change the activity LED once or twice a second
	 * and this is also a good time to check the armed and debug
	 * indicators.  We do this infrequently because analog reads
	 * seem to be slow.
	 */
	int flash = (millis() / (prevArm ? 500 : 1000)) & 1;
	if (flash != prevFlash) {
		digitalWrite(ledPin, flash ? HIGH : LOW );
		if (flash) {	// once per on/off cycle
			// check for (unlikely) debug console input
			if (Serial.available()) {
				doDebug( Serial.read() );
			}

			// check for changes in zone armedness
			unsigned char armed = ctrls->read();
			unsigned char difs = armed ^ prevArm;
			if (difs != 0) {
				for( int i = 0; i < 8; i++ ) {
					char mask = 1 << i;
					if ((difs & mask) != 0) {
						if (armed & mask) {  // arm
							zones->arm(i+1, true);
							mgr->reset();
						} else {	// disarm
							zones->arm(i+1, false);
						}
					}
				}
			}
                        prevArm = armed;
		}
	}
	
	prevFlash = flash;
}
