#include "Arduino.h"
#include <Config.h>
#include <Zone.h>

extern int debug;	// enable debug output

/**
 * create a managed zone alarm relays
 *
 * @param cfg object
 */
ZoneManager::ZoneManager( Config *config ) {

	cfg = config;		// note configuration object
	zoneStates = 0x00;	// no zones are triggered

	// program each relay control pin for output
	for ( int i = 0; i < cfg->zones->num_relays; i++ ) {
		int z = cfg->zones->zone(i);
		if (z < 0)
			continue;
		pinMode( cfg->zones->pin(i), OUTPUT );
		triggerTime[i] = 0;

#ifdef	DEBUG
		if (debug) {
			printf("Relay: zone=%d, pin=%d, normal=%d, period=%d\n",
				cfg->zones->zone(i),
				cfg->zones->pin(i),
				cfg->zones->normal(i),
				cfg->zones->min_trigger);
		}
#endif
	}
}

void ZoneManager::set( int zone, bool normal ) {
	if (zone >= 0 && zone < 8) {
		byte mask = 1 << zone;
		if (normal)
			zoneStates &= ~mask;
		else {
			zoneStates |= mask;
			if (triggerTime[zone] == 0)
				triggerTime[zone] = millis();
		}
	}
}

/**
 * reset all of the zone relays (before a scan)
 */
void ZoneManager::resetAll() {
	zoneStates = 0x00;
}

/**
 * update each relay to reflect the state of its zone
 */
void ZoneManager::update() {

	// set each relay to reflect the trigger state of its zone
	for ( int i = 0; i < cfg->zones->num_relays; i++ ) {
		int z = cfg->zones->zone(i);
		if (z < 0)
			continue;
		bool triggered = (zoneStates & (1 << z)) != 0;
			
		// we may need to enforce the minimum trigger period
		if (!triggered && triggerTime[z] > 0) {
			if (millis() < triggerTime[z])	// millis wrap
				triggerTime[z] = millis();
			if (millis() >= triggerTime[z] + (1000 * cfg->zones->min_trigger))
				triggerTime[z] = 0;
			else
				triggered = true;
		}

		// and write the appropriate value to the relay 
		int value = (triggered == cfg->zones->normal(i)) ? LOW : HIGH;
		digitalWrite( cfg->zones->pin(i), value );
	}
}
