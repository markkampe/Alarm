/**
 * There is a lot of configuration information associated
 * with this system: pins, positions in the shift cascades,
 * normal senses, debounce intervals, and LED duty cycles.
 *
 * Normally I would keep all of this in a configuration file
 * but arduinos have no files and precious little RAM, so I
 * created these classes to encapsulate hard encoded (like
 * in the text segment) configuration data.
 */
#include "Arduino.h"
#include <avr/pgmspace.h>
#include "Config.h"

/*
 * this is the configuration of all of the zone
 * alarm relays ... I would do it in a structure,
 * but ardunio doesn't support struct[] initialization.
 * And because memory is so dear on an Ardunio, I
 * decided to put it in PROGMEM.
 */
#define	Z_dis	0	// disabled
#define	Z_ent	1	// entry
#define	Z_ext	2	// external perimeter
#define	Z_brk	3	// window breakage
#define	Z_int	4	// OK if people are home

const char zonecfg[][3] PROGMEM = {
//  zone  normal  pin
  { Z_ent,  0,	   8  },	// legitimate entrances
  { Z_ext,  0,	   9  },	// obvious break-in points
  { Z_brk,  0,	   10 },	// window breakage sensors
  { Z_int,  0,	   11 },	// OK if people are home
  {     5,  0,     12 },	// unused zone
  {-1,	  -1,	  -1, }
};

/* minimum period for a relay to remain triggered	*/
#define	MINIMUM_TRIGGER	5

/*
 * this is the configuration of all of the sensors
 *
 * I would have done these in a structure, but
 * ardunio doesn't support struct[] initialization.
 * And because memory is so dear on an Ardunio, I
 * decided to put it in PROGMEM.
 */

#define X_zone  0	// column for zone
#define X_info	1	// column for misc config info
#define X_in    2	// column for input shift index
#define X_red   3	// column for red output index
#define X_green 4	// column for green output index
#define X_delay 5	// column for debounce count

/* sense of signal	*/
#define	S_hi	0x80	// signal is normally high
#define	S_lo	0x00	// signal is normally low

/*
 * I went to the trouble of implementing debouncing and
 * then concluded that all the sensors had a debounce
 * period shorter than our scan rate.
 */
#define D_none 0	// digital signal debounce
#define D_reed 0	// magnetic reed debounce
#define D_mech 0	// mechanical switch debounce
#define D_merc 0	// mercury switch debounce
#define D_mot  0	// infra-red motion sensor

/*
 * if the input port is -1, the sensor will not be read
 * if the zone is Z_dis the sensor cannot trigger anything
 */
const char sensorcfg[][6] PROGMEM = {
//   zone   info    in   red   grn   debounce	location	pr#/color	
  {  Z_ent, S_lo,    0,    0,    1,   D_reed },	// front entry	T23/blu
  {  Z_ent, S_lo,    1,    2,    3,   D_reed },	// garage door	T24/orn
  {  Z_ext, S_lo,    2,    4,    5,   D_reed },	// shop door	T25/grn
  {  Z_int, S_lo,    3,    6,    7,   D_reed },	// closet door	T22/blu
  {  Z_ext, S_lo,    4,    8,    9,   D_mech },	// bell tamper	T13/grn
  {  Z_int, S_lo,    5,   10,   11,   D_reed },	// mstr br sld	T21/brn
  {  Z_int, S_lo,    6,   12,   13,   D_reed },	// laundry sld	B16/blu
  {  Z_int, S_lo,    7,   14,   15,   D_reed },	// laundry door	B17/brn
  {  Z_int, S_lo,    8,   16,   17,   D_reed },	// din rm sld	B20/blu
  {  Z_int, S_lo,    9,   18,   19,   D_reed },	// north br	T18/blu	OPEN
  {  Z_brk, S_lo,   10,   20,   21,   D_merc },	// n br brk L	T19/orn
  {  Z_brk, S_lo,   11,   22,   23,   D_merc },	// n br brk R	T20/orn OPEN
  {  Z_ext, S_lo,   12,   24,   25,   D_reed },	// study south	B04/blu
  {  Z_brk, S_lo,   13,   26,   27,   D_merc },	// stdy brk s L	B05/orn
  {  Z_brk, S_lo,   14,   28,   29,   D_merc },	// stdy brk s R	B06/grn
  {  Z_ent, S_lo,   15,   30,   31,   D_reed },	// back entry	B14/blu
  {  Z_ext, S_lo,   16,   32,   33,   D_reed },	// basement dr	B15/grn
  {  Z_int, S_lo,   17,   34,   35,   D_reed },	// play room	B01/blu
  {  Z_brk, S_lo,   18,   36,   37,   D_merc },	// ply brk L	B02/orn
  {  Z_brk, S_lo,   19,   38,   39,   D_merc },	// ply brk R	B03/grn
  {  Z_int, S_lo,   20,   40,   41,   D_reed },	// study north	B07/blu
  {  Z_brk, S_lo,   21,   42,   43,   D_merc },	// stdy brk n L	B08/orn
  {  Z_brk, S_lo,   22,   44,   45,   D_merc },	// stdy brk n R	B09/grn
  {  Z_dis, S_lo,   23,   46,   47,   D_reed },	// stairway 	T17/blu BROKEN
  {  Z_int, S_lo,   24,   48,   49,   D_reed },	// play rm sld	B10/blu
  {  Z_brk, S_lo,   25,   50,   51,   D_merc },	// ply sld brkL	B11/orn
  {  Z_brk, S_lo,   26,   52,   53,   D_merc },	// ply sld brkR	B12/grn
  {  Z_ext, S_lo,   27,   54,   55,   D_reed },	// south office	B13/brn
  {  Z_ext, S_lo,   28,   56,   57,   D_reed },	// front rm lft T14/blu BROKEN
  {  Z_ext, S_lo,   29,   58,   59,   D_reed },	// front rm rt  T15/orn BROKEN
  {  Z_brk, S_lo,   30,   60,   61,   D_merc },	// front brk	T16/grn BROKEN
  {  Z_ext, S_lo,   -1,   62,   63,   D_mech },	// key tamper	back	NOTYET
  { -1,       -1,   -1,   -1,   -1,   -1 }
};

/**
 * accessor routine for sensor config data in text segment
 */
static unsigned char get_sensor_data( int sensor, int x ) {
	const char *p = &sensorcfg[sensor][x];
	return pgm_read_byte_near(p);
}

#define X_normal 1
#define X_pin    2

/**
 * accessor routine for zone config data in text segment
 */
static unsigned char get_zone_data( int zone, int x ) {
	const char *p = &zonecfg[zone][x];
	return pgm_read_byte_near(p);
}

/**
 * construct a sensor name, based on the info byte
 *
 * I had these big plans for a bunch of descriptive
 * stuff in the info byte (e.g. room, item, sensor 
 * type) which I would put in progmem and use to 
 * construct cool names, and if I were generating a
 * a log, these would be important, but at present, 
 * names are only used for debug output, where a 
 * sensor number is actually more useful than a 
 * descriptive name.   So I decided that this was 
 * just a waste of precious RAM for a useless feature.
 *
 * @param	sensor number
 * @return	a descriptive name
 */
const char *SensorCfg::name( int sensor ) {
	return "sensor";
}

/*
 * this is the configuration of the shift register cascades
 * (because they are parameters to a constructor, it was
 *  a skosh more awkward to make them PROGMEM, so here are
 *  8 (of my 1024) bytes pissed away.
 */
//			 #regs	data  clock  latch
struct ShiftCfg inCfg	= { 4,	5,    6,    7 };
struct ShiftCfg outCfg	= { 8,  2,    3,    4 };

/*
 * this is the configuration for the LEDs
 * (duty cycles and blink rates)
 */
const short ledparms[] PROGMEM = 
//	red	green	off	slow	med	fast
{	100,	100,	50,	1000,	500,	250 };

#define	LED_red		0
#define	LED_green	1
#define	LED_off		2
#define	LED_blink_slow	3
#define	LED_blink_med	4
#define	LED_blink_fast	5

/*
 * this is the configuration for the control input signals
 */
const short ctrlcfg[][3] PROGMEM = {
	A0,	0,	1024,
	A1,	0,	1024,
	A2,	0,	1024,
	A3,	0,	1024,
	-1,	-1,	-1
};

/**
 * accessor routine for sensor config data in text segment
 */
static short get_ctrl_data( int i, int x ) {
	const short *p = &ctrlcfg[i][x];
	return pgm_read_word_near(p);
}

#define	CTRL_PIN	0
#define	CTRL_SENSE	1
#define	CTRL_SCALE	2

/**
 * constructor for the configuration manager
 * (which pulls together all the pieces)
 */
Config::Config() {

	// load up the shift cascade configurations
	input = &inCfg;
	output = &outCfg;

	// load up the LED configuration
	leds = new LedCfg();

	// figure out how many sensors are configured
	int num_sensors = 0;
	for ( int i = 0; i < 256; i++ ) {
		if (get_sensor_data(i, X_zone) == 0xff)
			break;
		num_sensors++;
	}
	sensors = new SensorCfg( num_sensors );

	// figure out how many zones are configured
	int num_relays = 0;
	int num_zones = 0;
	for ( int i = 0; i < 256; i++ ) {
		int z = sensors->zone(i);
		if (z < 0)
			break;
		if (z > num_zones)
			num_zones = z;
		num_relays++;
	}
	zones = new ZoneCfg( num_zones, num_relays );

	int num_controls = 0;
	for( int i = 0; i < 8; i++ ) {
		int p = get_ctrl_data(i, CTRL_PIN);
		if (p < 0)
			break;
		num_controls++;
	}
	controls = new CtrlCfg(num_controls);
}

/*
 * everything beneath this point is trivial constructor/accessor functions
 */
SensorCfg::SensorCfg( int numsensor ) {
	num_sensors = numsensor;
}

ZoneCfg::ZoneCfg( int numzone, int numrelay ) {
	num_zones = numzone;
	num_relays = numrelay;
	min_trigger = MINIMUM_TRIGGER;
}

// accessor functions for LED configuration info
int LedCfg::usRed() {
	return pgm_read_word_near(ledparms + LED_red );
}

int LedCfg::usGreen() {
	return pgm_read_word_near(ledparms + LED_green );
}

int LedCfg::usOff() {
	return pgm_read_word_near(ledparms + LED_off );
}

int LedCfg::slow() {
	return pgm_read_word_near(ledparms + LED_blink_slow );
}

int LedCfg::med() {
	return pgm_read_word_near(ledparms + LED_blink_med );
}

int LedCfg::fast() {
	return pgm_read_word_near(ledparms + LED_blink_fast );
}


// accessor functions for sensor configuration info
bool SensorCfg::sense( int i) {
	if (i < num_sensors)
		return ((get_sensor_data( i, X_info ) & S_hi) != 0);
	return( 0 );
}

int SensorCfg::in( int i) {
	if (i < num_sensors)
		return get_sensor_data( i, X_in );
	return( -1 );
}

int SensorCfg::red( int i) {
	if (i < num_sensors)
		return get_sensor_data( i, X_red );
	return( -1 );
}

int SensorCfg::green( int i) {
	if (i < num_sensors)
		return get_sensor_data( i, X_green );
	return( -1 );
}

int SensorCfg::zone( int i) {
	if (i < num_sensors)
		return get_sensor_data( i, X_zone );
	return( -1 );
}

int SensorCfg::delay( int i) {
	if (i < num_sensors)
		return get_sensor_data( i, X_delay );
	return( 0 );
}

// accessor functions for zone configuration info
int ZoneCfg::zone( int i ) {
    return (i < num_zones) ? get_zone_data(i, X_zone) : -1;
}

int ZoneCfg::normal( int i ) {
    return (i < num_zones) ? get_zone_data(i, X_normal) : 0;
}

int ZoneCfg::pin( int i ) {
    return (i < num_zones) ? get_zone_data(i, X_pin) : -1;
}

// constructor and accessor functions for control configuration ifno
CtrlCfg::CtrlCfg( int num_ctrl ) {
	num_bits = num_ctrl;
}

int CtrlCfg::pin( int i ) {
	return (i < num_bits) ?  get_ctrl_data(i, CTRL_PIN) : -1;
}

bool CtrlCfg::sense( int i ) {
	return (i < num_bits) ?  get_ctrl_data(i, CTRL_SENSE) : -1;
}

int CtrlCfg::scale( int i ) {
	return (i < num_bits) ?  get_ctrl_data(i, CTRL_SCALE) : -1;
}
