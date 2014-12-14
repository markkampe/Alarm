#include "Arduino.h"
#include <Config.h>
#include <Zone.h>

extern int debug;	// enable debug output
#ifdef	DEBUG_EVT
extern void logTime( unsigned long );
#endif

/**
 * create a managed zone alarm relays
 *
 * @param cfg object
 */
ZoneManager::ZoneManager( Config *config ) {

	cfg = config;		// note configuration object

	// program each relay control pin for output
	for ( int i = 0; i < cfg->zones->num_relays; i++ ) {
		int z = cfg->zones->zone(i);
		if (z < 0)
			continue;
		pinMode( cfg->zones->pin(i), OUTPUT );
		triggerTime[i] = 0;

#ifdef	DEBUG_CFG
		if (debug) {
			printf("ZONE: zone=%d, out=%d, sense=%d, period=%d\n",
				cfg->zones->zone(i),
				cfg->zones->pin(i),
				cfg->zones->normal(i),
				cfg->zones->min_trigger);
		}
#endif
	}

	resetAll();
}

/**
 * set the state of a particular zone 
 *
 *	Once triggered, a zone remains triggered for the configured
 *	minimum trigger time.  Note that this is a trigger and not
 *	a latch.  
 *
 * @param zone		zone to be updated
 * @param normal	set to normal (vs triggered)
 */
void ZoneManager::set( int zone, bool normal ) {
	if (zone < 1 || zone > MAX_ZONES)
		return;

	byte mask = 1 << (zone - 1);
	if (normal)
		zoneStates &= ~mask;
	else {
		zoneStates |= mask;
		// if zone currently triggered, do not re-trigger
		if (triggerTime[zone-1] == 0) {
			// compute when the trigger end time
			unsigned long t = cfg->zones->min_trigger;
			t *= 1000;	// turn it to milliseconds
			t += millis();	// add this to the current time
			if (t == 0)	// zero is not a legal time
				t++;
			triggerTime[zone-1] = t;
		}
	}
#ifdef	DEBUG_EVT
	if (debug > 1) {
		// excuse: strings take up data space
		logTime(millis());
		putchar( normal ? '-' : '!' );
		putchar(' ');
		putchar('Z');
		putchar('=');
		putchar('0' + zone);
		putchar('\n');
	}
#endif
}

/**
 * set the armed/disarmed state of a zone
 *
 */
void ZoneManager::arm( int zone, bool armed ) {
	if (zone < 1 || zone > MAX_ZONES)
		return;

	char mask = 1 << (zone-1);

	if (armed)
		zoneArmed |= mask;
	else {
		zoneArmed &= ~mask;
		zoneStates &= ~mask;
		triggerTime[zone-1] = 0;
	}
#ifdef	DEBUG_EVT
	if (debug > 1) {
		// excuse: strings take up data space
		logTime(millis());
		if (!armed) {
			putchar('D');
		} else {
			putchar('A');
		}
		putchar(' ');
		putchar('Z');
		putchar('=');
		putchar('0' + zone);
		putchar('\n');
	}
#endif
}

/**
 * query the armed state of a zone
 */
bool ZoneManager::armed( int zone ) {
	if (zone < 1 || zone > MAX_ZONES)
		return false;
	unsigned char mask = 1 << (zone - 1);
	return ((zoneArmed & mask) != 0);
}

/**
 * reset all of the zone relays (before a scan)
 */
void ZoneManager::resetAll() {
	zoneStates = 0x00;
	zoneArmed = 0x00;
}

/**
 * update each relay to reflect the state of its zone
 */
void ZoneManager::update() {

	unsigned long now = millis();

	// set each relay to reflect the trigger state of its zone
	for ( int i = 0; i < cfg->zones->num_relays; i++ ) {
		// see if this zone is configured
		int z = cfg->zones->zone(i);
		if (z < 0)
			continue;

		// see if this zone is currently triggered
		bool triggered = (zoneStates & (1 << (z-1))) != 0;
			
		// see if a prior trigger is still running
		if (!triggered && triggerTime[z-1] > 0) {
			unsigned long t = triggerTime[z-1];
			if (now < t)
				triggered = (t - now) < MAX_TIMEOUT;
			else if (t < MAX_TIMEOUT)	// check for wrap
				triggered = (now - t) > MAX_TIMEOUT;
		}
		if (!triggered)
			triggerTime[z-1] = 0;

		// and write the appropriate value to the relay 
		int value = (triggered == cfg->zones->normal(i)) ? LOW : HIGH;
		digitalWrite( cfg->zones->pin(i), value );
	}
}
