#include <Arduino.h>
#include <Config.h>
#include <Control.h>

extern int debug;	// enable debug output

/**
 * create a managed set of control bits
 *
 * @param cfg object
 */
ControlManager::ControlManager( Config *config ) {

	cfg = config;		// note configuration object

	// program each relay control pin for output
	for (int i = 0; i < cfg->controls->num_bits; i++ ) {
#ifdef	DEBUG_CFG
		if (debug) {
			printf("Arm: zone=%d, pin=A%d, sense=%d, thresh=%d\n",
				i,
				cfg->controls->pin(i)-A0,
				cfg->controls->sense(i),
				cfg->controls->scale(i));
		}
#endif
	}
}


/*
 * read the status of the control bits and return a mask full of them
 */
char ControlManager::read() {
	char ret = 0;
	for (int i = 0; i < cfg->controls->num_bits; i++ ) {
		unsigned value = analogRead( cfg->controls->pin(i) );
		bool high = value > cfg->controls->scale(i) ;
		if (cfg->controls->sense(i) == high)
			ret |= 1 << i;

	}
	return( ret );
}
