#ifndef shiftreg.h
#define shiftreg.h

/**
 * a ShiftReg is the interface to a cascade of input or output
 * shift registers (e.g. 74HC595, 74C165)
 */
class Shiftreg {
  public:
    int  numRegs;	// number of cascaded registers
    int  dataPin;	// data input/output pin
    int  clockPin;	// data clocking pin
    int  latchPin;	// data latching pin
    char *data;		// input/output buffer
  
    /**
     * Create a new shift register cascade descriptor
     *
     * @param	regs, number of chips in cascade
     * @param	data, pin number for data output
     * @param	clock, pin number for shift clock
     * @param	latch, pin number for data latch
     */
    Shiftreg( int regs, int dataP, int clockP, int latchP );
};

/**
 * a cascade of 74HC595 shift registers for parallel output
 */
class OutShifter : public Shiftreg {
  public:
    /**
     * initialize a cascade of shift registers for output
     *
     * @param	regs, number of chips in cascade
     * @param	data, pin number for data input/output
     * @param	clock, pin number for shift clock
     * @param	latch, pin number for data latch
     */
    OutShifter( int regs, int dataP, int clockP, int latchP );
    
    /**
     * set the specified bit to one or zero
     */
    void set( int bit, bool value );

    /**
     * output the supplied data to the shift register cascade
     */
    void write();
};

/**
 * a cascade of 74C165 shift registers for parallel input
 */
class InShifter : public Shiftreg {
  public:
    /**
     * initialize a cascade of shift registers for input
     *
     * @param	regs, number of chips in cascade
     * @param	data, pin number for data input
     * @param	clock, pin number for shift clock
     * @param	latch, pin number for data latch
     */
    InShifter( int regs, int dataP, int clockP, int latchP );
    
    /**
     * set the specified bit to one or zero
     */
    bool get( int bit );

    /**
     * read the contents of the shift register cascade
     *
     * @param data  pointer to array to receive the data
     * @param numBytes number of data bytes to be read
     */
    void read( );
};
#endif
