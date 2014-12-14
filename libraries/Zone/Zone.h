#ifndef ZONE_H
#define ZONE_H

#define	MAX_ZONES 4

/**
 * a managed collection of zone alarm relays
 */
class ZoneManager {

  public:
    /**
     * create a managed zone alarm relays
     *
     * @param config object
     */
    ZoneManager( Config *config );
    
    /**
     * set the state of a particular zone relay
     *
     */
    void set( int zone, bool normal );

    /**
     * reset all of the zone relays (before a scan)
     */
    void resetAll();

    /**
     * update all physical relays to reflect their configured states
     */
    void update();


  private:
    Config *cfg;		// system configuration
    unsigned char zoneStates;	// current triggered state of each zone
    unsigned short triggerTime[MAX_ZONES];	// end of trigger time
};
#endif
