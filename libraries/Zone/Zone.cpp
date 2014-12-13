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

	// program each relay control pin for output
	for ( int i = 0; i < cfg->zones->num_relays; i++ ) {
		int z = cfg->zones->zone(i);
		if (z < 0)
			continue;
		pinMode( cfg->zones->pin(i), OUTPUT );
		triggerTime[i] = 0;

#ifdef	DEBUG
		if (debug) {
			printf("Relay: zone=%d, out=%d, normal=%d, period=%d\n",
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
 * generate a log message for a zone state/arm change
 *
 * @param time (in ms) of incident
 * @param sensor index of sensor
 */
void ZoneManager::logEvent( unsigned long mstime, int zone ) {

#ifdef	DEBUG
	// deconstruct and log the time
	int ms   = mstime % 1000;	mstime /= 1000;
	int secs = mstime % 60;		mstime /= 60;
	int mins = mstime % 60;		mstime /= 60;
	int hours = mstime % 24;
	printf("%02d:%02d:%02d.%03d  ",
		hours, mins, secs, ms );

	// then print out the event
	printf("Zone %d: ", zone);
	char mask = 1 << zone;
	printf( (zoneArmed & mask) ? "armed, " : "disarmed, " );
	printf( (zoneStates & mask) ? "triggered\n" : "normal\n" );
#endif
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
	if (zone < 0 || zone > 8)
		return;

	byte mask = 1 << zone;
	if (normal)
		zoneStates &= ~mask;
	else {
		zoneStates |= mask;
		// if zone currently triggered, do not re-trigger
		if (triggerTime[zone] == 0) {
			// compute when the trigger end time
			unsigned long t = cfg->zones->min_trigger;
			t *= 1000;	// turn it to milliseconds
			t += millis();	// add this to the current time
			if (t == 0)	// zero is not a legal time
				t++;
			triggerTime[zone] = t;
		}
	}
#ifdef	DEBUG
	if (debug > 1) {
		logEvent(millis(), zone);
	}
#endif

}

/**
 * set the armed/disarmed state of a zone
 *
 */
void ZoneManager::arm( int zone, bool armed ) {
	if (zone < 0 || zone > 8)
		return;

	char mask = 1 << zone;

	if (armed)
		zoneArmed != mask;
	else {
		zoneArmed &= ~mask;
		zoneStates &= ~mask;
		triggerTime[zone] = 0;
	}
#ifdef	DEBUG
	if (debug > 1) {
		logEvent(millis(), zone);
	}
#endif
}

/**
 * query the armed state of a zone
 */
bool ZoneManager::armed( int zone ) {
	char mask = 1 << zone;
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
		bool triggered = (zoneStates & (1 << z)) != 0;
			
		// see if a prior trigger is still running
		if (!triggered && triggerTime[z] > 0) {
			unsigned long t = triggerTime[z];
			if (now < t)
				triggered = (t - now) < MAX_TIMEOUT;
			else if (t < MAX_TIMEOUT)	// check for wrap
				triggered = (now - t) > MAX_TIMEOUT;
		}
		if (!triggered)
			triggerTime[z] = 0;

		// and write the appropriate value to the relay 
		int value = (triggered == cfg->zones->normal(i)) ? LOW : HIGH;
		digitalWrite( cfg->zones->pin(i), value );
	}
}
