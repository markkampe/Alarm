#include "Config.h"
#include "Arduino.h"
#include "Shiftreg.h"

/**
 * Create a new shift register cascade descriptor, 
 * initializing the basic parameters.
 */
Shiftreg::Shiftreg( int regs, int dataP, int clockP, int latchP ) {
	numRegs = regs;
	data = (char *) malloc( numRegs );	// ardunio can't new[]
	for ( int i = 0; i < numRegs; i++ )
		data[i] = 0;

	dataPin = dataP;
	// this pin gets initialized in the subclass

	clockPin = clockP;
	pinMode( clockP, OUTPUT );

	latchPin = latchP;
	pinMode( latchP, OUTPUT );
};

/**
 * initialize a shift register cascade for parallel output
 */
OutShifter::OutShifter( int regs, int dataP, int clockP, int latchP ) :
    Shiftreg( regs, dataP, clockP, latchP ) {
      pinMode( dataP, OUTPUT );

#ifdef	DEBUG_CFG
    extern int debug;
    if (debug) {
	printf("O-shift: regs=%d, data=%d, clock=%d, latch=%d\n",
		regs, dataP, clockP, latchP );
    }
#endif
}
    
/**
 * output the supplied data to the shift register cascade
 *
 * @param data  pointer to data to be output
 * @param numBytes number of data bytes to be output
 *
 * NOTE: the low order bit of the first byte in the
 *	 data array will wind up as the lowest 
 *	 order bit (D0) in the first (directly 
 *	 connected to data write) register of the 
 *	 shift cascade.
 */
void OutShifter::write() {
	// prepare to shift out the data
	digitalWrite(clockPin, LOW);
	digitalWrite(latchPin, LOW);

	// clock out all of our data
	for ( int i = numRegs - 1; i >= 0; i-- )
		shiftOut( dataPin, clockPin, MSBFIRST, data[i] );

	// latch the data for output
	digitalWrite(latchPin, HIGH);
}

/**
 * set the value of the specified bit
 *
 * @param bit number of the desired bit (0...)
 * @param value to set it to
 */
void OutShifter::set( int bit, bool value ) {
	if (bit < 0 || bit >= numRegs*8)
		return;
	
	unsigned char mask = 1 << (bit & 7);

	if (value) 
		data[bit>>3] |= mask;
	else
		data[bit>>3] &= ~mask;
}

/**
 * initialize a shift register cascade for parallel input
 */
InShifter::InShifter( int regs, int dataP, int clockP, int latchP ) :
    Shiftreg( regs, dataP, clockP, latchP ) { 
	pinMode( dataP, INPUT );

#ifdef	DEBUG_CFG
	extern int debug;
	if (debug) {
		printf("I-shift: regs=%d, data=%d, clock=%d, latch=%d\n",
			regs, dataP, clockP, latchP );
	}
#endif
}
    
/**
 * replacement for buggy library shiftIn, which
 * seems to reverse the raising and lowering of 
 * the clock signal.
 *
 * @param dataPin input
 * @param clockPin shift
 * @param bitOrder LSBFIRST/MSBFIRST
 */
int myShiftIn( int dataPin, int clockPin, int bitOrder ) {
	int i;
	int value = 0;
 
 	for (i = 0; i < 8; ++i) {
		digitalWrite(clockPin, LOW);
 		if (bitOrder == LSBFIRST)
 			value |= digitalRead(dataPin) << i;
 		else
 			value |= digitalRead(dataPin) << (7 - i);
		digitalWrite(clockPin, HIGH);
 	}
 	return value;
}

/**
 * read the contents of the shift register cascade
 *
 * @param data  pointer to array to receive the data
 * @param numBytes number of data bytes to be read
 *
 * NOTE: the low order bit of the first byte in the
 *	 data array comes from the lowest order bit 
 *	 in the first register of the cascade.
 */
void InShifter::read() {
        
	// prepare to shift in the data
	digitalWrite(clockPin, LOW);
	digitalWrite(latchPin, LOW);

	// latch the data for input
	digitalWrite(latchPin, HIGH);

	// clock in all of our data
	for ( int i = 0; i < numRegs; i++ )
		data[i] = myShiftIn( dataPin, clockPin, LSBFIRST );
}

/**
 * return the value of a specified bit
 *
 * @param bit number of desired bit (0...)
 */
bool InShifter::get( int bit ) {
	if (bit < 0 || bit >= numRegs*8)
		return( 0 );
	
	unsigned char mask = 1 << (bit & 7);
	return( (data[ bit >> 3 ] & mask) != 0 );
}
