#ifndef CONTROL_H
#define CONTROL_H

#define	MAX_CONTROL 8

/**
 * a managed collection of input control bits
 */
class ControlManager {

  public:
    /**
     * create a managed set of controls
     *
     * @param config object
     */
    ControlManager( Config *config );
    
    /**
     * return the status of the input control bits
     */
    char read();

  private:
    Config	*cfg;		// configuration object
};
#endif
