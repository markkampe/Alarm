#ifndef sensor_h
#define sensor_h

#include <Shiftreg.h>

/**
 * a managed collection of sensors and their status indicators
 */
class SensorManager { 

  public:
    /**
     * a managed collection of LEDs
     *
     * @param config	object
     * @param input	InShifter for the sensors
     * @param output	OutShifter for the indicators
     * @param zones	Zone manager for alarm relays
     */
    SensorManager( Config *config, InShifter *input, OutShifter *output );

    /**
     * read the current status of the input cascade
     */
    void sample();

    /**
     * flush the current LED states to the output cascade
     */
    void update();

    /**
     * lamp-test during the first few start-up cycles
     */
    bool lampTest( bool force );

    enum ledState { 	// LED color values
	led_off = 0, led_red, led_green, led_yellow 
    };

    enum ledBlink {	// LED blink rates
	led_none = 0, led_slow, led_med, led_fast
    };

    /**
     * @param zone to be updated
     * @param armed
     */
    void arm( int zone, bool armed );

    unsigned char zoneArmed;	// zone armed bits
    unsigned char zoneState;	// zone triggered bits

  private:
    /*
     * Memory is so precious on the Arduino that we:
     * (a) don't have individual sensor/individual objects
     *     but merely manage them by indices within the
     *     manager clas.
     * (b) don't copy configuration information into this
     *     object but reference it from the (read only)
     *	   configuration object.
     * (c) encode everything we know about each sensor
     *     in two bytes (one full of state/status bits)
     */
    Config     *cfg;		// configuration object
    InShifter  *inshifter;	// input shift cascade for collection
    OutShifter *outshifter;	// output shift cascade for collection

    unsigned char *debounce;	// debounce counts for each sensor
    unsigned char *states;	// state bytes for each sensor
#ifdef DEFIB
    unsigned char *defib;	// defibrillation counts for each zone
    unsigned nextUpdate;	// time of next defib count update
#endif

// bits in the sensor state bytes
#define S_b_lo	 	0x01	// low order bit of blink rate
#define S_b_hi	 	0x02	// high order bit of blink rate
#define S_red	 	0x04	// red light should be on
#define S_green	 	0x08	// green light should be on
#define S_trigger	0x10	// this sensor has been triggered
#define S_status 	0x20	// currently reported state (1=normal)
#define S_prev	 	0x40	// last read state (1=normal)
#define S_sense	 	0x80	// 1 = high asserted

    /**
     * set the desired state of a LED
     *
     * @param sensor number
     * @param state of LED
     * @param blink rate of LED
     */
    void setLed( int sensor, enum ledState state, enum ledBlink blink );

    bool hasRed( int sensor );		// accessor function for red value
    bool hasGreen( int sensor );	// accessor function for green value
    int blinkRate( int sensor );	// accessor function for blink rate

    /**
     * set the status of a sensor
     *
     * @param sensor number
     * @param isNormal
     */
     void status( int sensor, bool isNormal );

    /**
     * @param sensor number
     * @return the status of a sensor
     */
    bool status( int sensor );

    /**
     * set the previous status of a sensor
     *
     * @param sensor number
     * @param isNormal
     */
     void previous( int sensor, bool isNormal );

    /**
     * @param sensor number
     * @return the previous status of a sensor
     */
    bool previous( int sensor );

    /**
     * set the triggered indication for a sensor
     *
     * @param sensor number
     * @param isTriggered
     */
     void triggered( int sensor, bool isTriggered );

    /**
     * @param sensor number
     * @return the triggered status of a sensor
     */
    bool triggered( int sensor );
};
#endif
