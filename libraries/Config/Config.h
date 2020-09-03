#ifndef CONFIG_H
#define	CONFIG_H

//#define	DEBUG		1	// enable debug output
//#define	DEBUG_CMD	1	// enable debug commands
//#define	DEBUG_EVT	1	// enable event logging
//#define	DEBUG_CFG	1	// enable configuration debug

//#define ACTIVE_HIGH	1	// relay triggers on high

#define	DEFIB		1	// enable zone defibrillation

// maximum timeout interval ... used for wrap detection
#define	MAX_TIMEOUT	(10*60*1000)

/**
 * These classes encapsulate the form in which we package
 * configuration information for the alarm application -
 * which is particularly imporatant given the measures we
 * take to avoid using precious-rare Arduino RAM.
 *
 */

/**
 * configuration of an input sensor, and its associated indicators
 */
class SensorCfg {
    public:
	char num_sensors;	// number of configured sensors

	const char *name(int i);// name of this sensor
	int zone( int i );	// zone it monitors

	bool sense( int i );	// non-tripped value
	int in( int i );	// input cascade index
	int delay( int i );	// debounce delay (us)

	int red( int i );	// output cascade index
	int green( int i );	// output cascade index

	int numZones();		// number of configured zones
	int zonePin( int i );	// output pin for each zone

	/**
	 * @param number of sensors to be configured
	 */
	SensorCfg( int number );
};

/**
 * configuration of an input/output cascade
 */
struct ShiftCfg {
	char num_regs;		// number of registers
	char data;		// in/out pin number
	char clock;		// output pin number
	char latch;		// output pin number
};

/**
 * configurations for LED duty cycles
 */
class LedCfg {
    public:
	int usRed();		// red (us)
	int usGreen();		// green (us)
	int usOff();		// off (us)
	int slow();		// slow blink (ms)
	int med();		// medium blink (ms)
	int fast();		// fast blink (ms)
};

/**
 * configurations for the system control inputs
 */
class CtrlCfg {
    public:
	char num_bits;		// number of valid bits
	int pin(int i);		// analog pin to read
	bool sense(int i);	// high asserted?
	int scale(int i);	// full scale reading
	int minInterval();	// min reasonable inter-trigger (ms)
	int maxTriggers();	// max consecutive triggers

	/**
	 * @param: number of valid /bits
	 */
	CtrlCfg( int bits );
};

/**
 * This class wraps up all of the different types of
 * configuration in a single object.
 */
class Config {
    public:
	
	ShiftCfg *input;
	ShiftCfg *output;
	LedCfg *leds;
	SensorCfg *sensors;
	CtrlCfg *controls;

	Config();
};
#endif
