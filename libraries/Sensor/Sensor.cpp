/*
 * This module implements detection and display operations
 * for the sensors. 
 *
 * The implementations in terms of bits and wrappers around
 * configuration classes is motivated by the fact that the
 * ardunio has 32K of code, but only 1K bytes of RAM.
 */
#include <Config.h>
#include <Sensor.h>
#include <Arduino.h>

extern int debug;	// debug level
#ifdef DEBUG_EVT
extern void logTime( unsigned long );
#endif

/**
 * a managed collection of LEDs
 *
 * @param config object
 * @param input	InShifter for the sensors
 * @param output OutShifter for the indicators
 * @param zonemanager for zone alarm relays
 */
SensorManager::SensorManager(	Config *config,
				InShifter *input, OutShifter *output ) {
	// note our lower level resource managers
	cfg = config;
	inshifter = input;
	outshifter = output;
	zoneArmed = 0;
	zoneState = 0;

#ifdef	DEBUG_CFG
	if (debug) {
		printf("LEDs: <r,g,0>=<%d,%d,%d>us, blink=<%d,%d,%d>ms\n",
			cfg->leds->usRed(), cfg->leds->usGreen(), cfg->leds->usOff(),
			cfg->leds->fast(), cfg->leds->med(), cfg->leds->slow());
	}
#endif
	// allocate and intialize the sensor status and debounce arrays
	states = (unsigned char *) malloc( cfg->sensors->num_sensors );
#ifdef DEBOUNCE
	debounce = (unsigned char *) malloc( cfg->sensors->num_sensors );
#endif
	for ( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
		// all sensors start out normal, all enabled sensors green
		states[i] = S_status | S_prev | (cfg->sensors->sense(i) ? S_sense : 0);	
		if (cfg->sensors->in(i) != 255)
			states[i] |= S_green;
#ifdef DEBOUNCE
		debounce[i] = 0;	// we are not currently debouncing
#endif
#ifdef DEBUG_CFG
		if (debug) {
			printf("Sensor: %s zone=%d, s/d=<%d,%d>, in=%d, <red,grn>=<%d,%d>\n",
				cfg->sensors->name(i),
				cfg->sensors->zone(i),
				cfg->sensors->sense(i),
				cfg->sensors->delay(i),
				cfg->sensors->in(i),
				cfg->sensors->red(i),
				cfg->sensors->green(i) );
		}
#endif
	}
	
	// configure the zone relay control pins
	for( int i = 1; i <= cfg->sensors->numZones(); i++ ) {
		int p = cfg->sensors->zonePin(i);
		if (p >= 0) {
			pinMode(p, OUTPUT);
#ifdef DEBUG_CFG
			if (debug)
				printf("Relay: %d, pin=%d\n", i, p);
#endif
		}
	}
}


/**
 * read all of the inputs, debounce them, and update
 * the sensor and LED status accordingly.
 */
void SensorManager::sample() {

	// note the time of this sample
	unsigned long now = millis();

	// latch the current values
	inshifter->read();

	// start all relays out normal
	zoneState = 0;
	
	// run through all of the configured sensors
	for( int i = 0; i < cfg->sensors->num_sensors; i++ ) {

	    // if sensor isn't configured ignore it
	    int x = cfg->sensors->in(i);
	    if (x == 255) {
		setLed(i, led_off, led_none);	// show no status
		continue;
	    }

	    // get the current (normalized) value of this sensor
	    bool v = (inshifter->get( x ) == cfg->sensors->sense(i));

	    // see if the value is stable (same as last sample)
	    if (v != previous(i)) {
		previous(i,v);
#ifdef DEBOUNCE
		debounce[i] = cfg->sensors->delay(i) + 1;
	    } 

	    if (debounce[i] > 0) {
		debounce[i] = debounce[i] - 1;
		continue;
#endif
	    }

	    // see if the stable value is a change
	    if (v != status(i)) {
	        status( i, v );
#ifdef	DEBUG_EVT
		if (debug > 1) {	
			// excuse: strings take up data space
			logTime( now );
			putchar( v == 0 ? '!' : '-' );
			putchar(' ');
			putchar('S');
			putchar('=');
			putchar('0' + i/10);
			putchar('0' + i%10);
			putchar('\n');
		}
#endif
	    }
	
	    // figure out if this sensor has been triggered
	    if (!v)
		triggered(i, true);
	
	    // figure out whether or not the sensor/zone is armed
	    int z = cfg->sensors->zone(i);
	    unsigned char armed = 0;
	    if (z >= 1 && z <= 8) {
		armed = zoneArmed & (1 << (z-1));
		if (armed && !v)
			zoneState |= 1 << (z-1);
	    }

	    // figure out what to do with the LEDs
	    enum ledState state = led_green;
	    enum ledBlink blink = led_none;
	    if (!v) {
	    	state = (armed != 0) ? led_red : led_yellow;
	    } else if (triggered(i)) {
	    	state = (armed != 0) ? led_red : led_green;
	    	blink = (armed != 0) ? led_fast : led_med;
	    } else if (armed !=0)
	    	blink = led_slow;
	    setLed( i, state, blink );
	}
}

/**
 * run through each of the four color phases 
 * (red, off, green off) for all of the LEDs, 
 * based on the color state for each sensor.
 *
 * Assertion: 
 *	at entry, all LED output shift states are zero
 *	because that is how we initialize them, 
 *	and that is how we leave them from here.
 */
void SensorManager::update() {

	long now = millis();	// time for blink management
	
	// turn on any red LEDs that need to be turned on
	int set = 0;
	for( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
	    // if the sensor isn't configured ignore it

	    // see if we are blinked off
	    int b = blinkRate(i);
	    if (b > 0 && ((now/b) & 1))
		continue;

	    // see if we should turn this red LED on
	    if (hasRed(i)) {
		outshifter->set(cfg->sensors->red(i), true);
		set++;
	    }
	}
	outshifter->write();	// and latch those values
	delayMicroseconds(cfg->leds->usRed());

	// turn off all the red LEDs
	if (set > 0) {
	    for( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
		    outshifter->set(cfg->sensors->red(i), 0);
	    }
	    outshifter->write();	// and latch those values
	}
        if (cfg->leds->usOff() > 1)
		delayMicroseconds(cfg->leds->usOff()/2);
		
	// turn on any green LEDs that need to be turned on
	set = 0;
	for( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
	    // see if we are blinked off
	    int b = blinkRate(i);
	    if (b > 0 && ((now/b) & 1))
		continue;

	    // see if we should turn this green LED on
	    if (hasGreen(i)) {
		outshifter->set(cfg->sensors->green(i), true);
		set++;
	    }
	}
	outshifter->write();	// and latch those values
	delayMicroseconds(cfg->leds->usGreen());

	// turn off all the greens
	if (set > 0) {
	    for( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
	        outshifter->set(cfg->sensors->green(i), 0);
	    }
	    outshifter->write();	// and latch those values
	}

        if (cfg->leds->usOff() > 0)
		delayMicroseconds((1+cfg->leds->usOff())/2);

	// flush out the state of each trigger relay
	for( int i = 0; i < cfg->sensors->numZones(); i++ ) {
		int p = cfg->sensors->zonePin(i + 1);
		if (p > 0) {
			int t = zoneState & (1 << i);
			digitalWrite(p, t ? HIGH : LOW);
		}
	}
}

/**
 * for the first few seconds after start up, we run a lamp test
 *
 * @param	force run test even if it has already run
 * @return	true if we are still in the lamp test
 */
bool SensorManager::lampTest(bool force) {
	static bool done;
	static unsigned long startTime;
	static int numTests = 8;	// power on self-test
	static ledState test[] = {
	    led_off, led_red, led_green, led_yellow,
	};

	if (force && done) {
		done = false;
		startTime = 0;
		numTests = 60;	// one minute of tests
	} else if (done)
		return( false );

	// figure out when the tests started (checking for timer wrap)
	unsigned long now = millis();
	if (startTime == 0 || now < startTime) {
		startTime = now;
#ifdef DEBUG_EVT
		if (debug > 1) {
			// excuse: strings take up data space
			logTime(now);
			putchar('T');
			putchar('E');
			putchar('S');
			putchar('T');
			putchar('\n');
		}
#endif
	}

	// see if we're done with the tests yet
	int second = (now - startTime)/1000;
	if (second > numTests) {
		done = true;
#ifdef DEBUG_EVT
		if (debug > 1) {
			// excuse: strings take up data space
			logTime(now);
			putchar('R');
			putchar('U');
			putchar('N');
			putchar('\n');
		}
#endif
		return false;
	}

	// set all the LEDs according to the test phase
	for ( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
		setLed(i, test[second%4], led_none );
	}
	return( true );
}

/**
 * set the desired state of a LED
 *
 * @param sensor number
 * @param state of LED
 * @param blink period (in ms)
 */
void SensorManager::setLed( int sensor, enum ledState state, enum ledBlink blink ) {
	if (sensor < 0 || sensor >= cfg->sensors->num_sensors)
		return;
	
	// set the red/green indicators in the state byte
	states[sensor] &= ~(S_red+S_green+S_b_hi+S_b_lo);
	switch( state ) {
	   case led_yellow:
		states[sensor] |= S_green;
	   case led_red:
		states[sensor] |= S_red;
		break;

	   case led_green:
		states[sensor] |= S_green;
		break;
	   default:
		break;
	}

	// set the blink speed bits in the state byte
	switch( blink ) {
	   case led_fast:
		states[sensor] |= S_b_hi;
	   case led_slow:
		states[sensor] |= S_b_lo;
		break;

	   case led_med:
		states[sensor] |= S_b_hi;
		break;
		
	   default:
		break;
	}
}

/**
 * @return should the red light be on
 */
bool SensorManager::hasRed( int sensor ) {
	if (sensor >=0 && sensor < cfg->sensors->num_sensors)
		return( (states[sensor] & S_red) != 0 );
	else
		return false;
}

/**
 * @return should the green light be on
 */
bool SensorManager::hasGreen( int sensor ) {
	if (sensor >=0 && sensor < cfg->sensors->num_sensors)
		return( (states[sensor] & S_green) != 0 );
	else
		return false;
}
	
/**
 * @param sensor number
 * @return blink period (in ms) for that sensor
 */
int SensorManager::blinkRate( int sensor ) {
	if (sensor < 0 || sensor >= cfg->sensors->num_sensors)
		return( 0 );

	switch( states[sensor] & (S_b_hi+S_b_lo) ) {
	    case S_b_hi+S_b_lo:
		return cfg->leds->fast();
	    case S_b_hi:
		return cfg->leds->med();
	    case S_b_lo:
		return cfg->leds->slow();
	    default:
		return 0;
	}
}

/**
 * reset the triggered status of all sensors
 */
void SensorManager::reset() {
	for( int i = 0; i < cfg->sensors->num_sensors; i++ ) {
		triggered(i, false);
	}

#ifdef	DEBUG_EVT
	if (debug > 1) {	
		// excuse: strings take up data space
		logTime( millis() );
		putchar('R');
		putchar('E');
		putchar('S');
		putchar('E');
		putchar('T');
		putchar('\n');
	}
#endif
}

/**
 * set the triggered indication for a sensor
 *
 * @param sensor number
 * @param isTriggered
 */
void SensorManager::triggered( int sensor, bool isTriggered ) {
	if (sensor >= 0 && sensor < cfg->sensors->num_sensors)
		if (isTriggered) 
			states[sensor] |= S_trigger;
		else
			states[sensor] &= ~S_trigger;
}

/**
 * @param sensor number
 * @return the triggered status of a sensor
 */
bool SensorManager::triggered( int sensor ) {
	if (sensor >= 0 && sensor < cfg->sensors->num_sensors)
		return( (states[sensor] & S_trigger) != 0 );
	else
		return( false );
}

/**
* @param zone to be updated
* @param armed
*/
void SensorManager::arm( int zone, bool armed ) {
	if (zone < 1 || zone > 8)
		return;
	unsigned char mask = 1 << (zone-1);
	if (armed)
		zoneArmed |= mask;
	else
		zoneArmed &= ~mask;

#ifdef	DEBUG_EVT
	if (debug > 1) {	
		// excuse: strings take up data space
		logTime( millis() );
		putchar(armed ? 'A' : 'd');
		putchar(' ');
		putchar('Z');
		putchar('=');
		putchar('0' + zone%10);
		putchar('\n');
	}
#endif
}

/*
 * note that each sensor has an official reported
 * state, and a last read state.  They are only
 * different during debounce.
 */

/**
 * set the status of a sensor
 *
 * @param sensor number
 * @param isNormal
 */
void SensorManager::status( int sensor, bool isNormal ) {
	if (sensor < 0 || sensor >= cfg->sensors->num_sensors)
		return;
	if (isNormal)
		states[sensor] |= S_status;
	else
		states[sensor] &= ~S_status;
}

/**
 * @param sensor number
 * @return offical status of a sensor
 */
bool SensorManager::status( int sensor ) {
	if (sensor >= 0 && sensor < cfg->sensors->num_sensors)
		return (states[sensor] & S_status) != 0;
	else
		return( false );
}

/**
 * set the last read state of a sensor
 * 
 * @param sensor	number of the sensor to set
 * @param isNormal	current state
 */
void SensorManager::previous( int sensor, bool isNormal ) {
	if (sensor < 0 || sensor >= cfg->sensors->num_sensors)
		return;
	if (isNormal)
		states[sensor] |= S_prev;
	else
		states[sensor] &= ~S_prev;
}

/**
 * @param sensor number
 * @return the last read status of a sensor
 */
bool SensorManager::previous( int sensor ) {
	if (sensor >= 0 && sensor < cfg->sensors->num_sensors)
		return (states[sensor] & S_prev) != 0;
	else
		return( false );
}
