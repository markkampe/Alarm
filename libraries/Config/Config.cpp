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
#define	Z_int	3	// OK if people are home
#define	Z_tmp	4	// tamper sensors

const char zonecfg[][3] PROGMEM = {
//  zone  normal  pin
  { Z_ent,  0,	   8  },	// legitimate entrances
  { Z_ext,  0,	   9  },	// obvious break-in points
  { Z_int,  0,	   10 },	// OK if people are home
  { Z_tmp,  0,	   11 },	// tamper sensors
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
 *
 * I was thinking I might want to encode other
 * information in the info byte besides the sense
 * of the signal, but sans names, I didn't need more
 */
#define	S_hi		0x80

#define X_zone  0	// column for zone
#define X_info	1	// column for misc config info
#define X_in    2	// column for input shift index
#define X_green 3	// column for green output index
#define X_red   4	// column for red output index
#define X_delay 5	// column for debounce count

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

const char sensorcfg[][6] PROGMEM = {
//   zone   info    in   grn   red   debounce	location	pr#/color	
  {  Z_tmp, S_hi,    0,    1,    0,   D_mech },	// bell tamper	T13/grn
  {  Z_dis, S_hi,    1,    3,    2,   D_reed },	// front rm lft	T14/blu
  {  Z_dis, S_hi,    2,    5,    4,   D_reed },	// front rm rt	T15/orn
  // old wire, short & break, sensors removed	// front brk	T16/grn
  // old wire, sensor broken?			// stairway	T17/blu
  {  Z_ext, S_hi,    3,    7,    6,   D_reed }, // north br	T18/blu
  {  Z_ext, S_hi,    4,    9,    8,   D_merc },	// n br brk L	T19/orn
//{  Z_int, S_hi,    x,    x,    x,   D_merc },	// n br brk R	T20/grn
  {  Z_int, S_hi,    5,   11,   10,   D_reed },	// mstr br sld	T21/brn
  {  Z_int, S_hi,    6,   13,   12,   D_reed },	// closet door	T22/blu
  {  Z_ent, S_hi,    7,   15,   14,   D_reed },	// front entry	T23/blu
  {  Z_ent, S_hi,    8,   17,   16,   D_reed },	// garage door	T24/orn
  {  Z_ext, S_hi,    9,   19,   18,   D_reed },	// shop door	T25/grn
  {  Z_ext, S_hi,   10,   21,   20,   D_reed },	// play room	B01/blu
  {  Z_ext, S_hi,   11,   23,   22,   D_merc },	// ply brk L	B02/orn
  {  Z_int, S_hi,   12,   25,   24,   D_merc },	// ply brk R	B03/grn
  {  Z_ext, S_hi,   13,   27,   26,   D_reed },	// study south	B04/blu
  {  Z_ext, S_hi,   14,   29,   28,   D_merc },	// stdy brk s L	B05/orn
  {  Z_int, S_hi,   15,   31,   30,   D_merc },	// stdy brk s R	B06/grn
  {  Z_ext, S_hi,   16,   33,   32,   D_reed },	// study north	B07/blu
  {  Z_ext, S_hi,   17,   35,   34,   D_merc },	// stdy brk n L	B08/orn
  {  Z_int, S_hi,   18,   37,   36,   D_merc },	// stdy brk n R	B09/grn
  {  Z_ext, S_hi,   19,   39,   38,   D_reed },	// play rm sld	B10/blu
  {  Z_ext, S_hi,   20,   41,   40,   D_merc },	// ply sld brkL	B11/orn
  {  Z_int, S_hi,   21,   43,   42,   D_merc },	// ply sld brkR	B12/grn
  {  Z_ext, S_hi,   22,   45,   44,   D_reed },	// south office	B13/brn
  {  Z_ent, S_hi,   23,   47,   46,   D_reed },	// back entry	B14/blu
  {  Z_ext, S_hi,   24,   49,   48,   D_reed },	// basement dr	B15/grn
  {  Z_int, S_hi,   25,   51,   50,   D_reed },	// laundry sld	B16/blu
  {  Z_int, S_hi,   26,   53,   52,   D_reed },	// laundry door	B17/brn
  // intented use, sensor never installed	// lnd sld brkL	B18/orn
  // intented use, sensor never installed	// lnd sld brkR	B19/grn
  {  Z_int, S_hi,   27,   55,   54,   D_reed },	// din rm sld	B20/blu
  // intented use, sensor never installed	// din sld brkL	B21/orn
  // intented use, sensor never installed	// din sld brkR	B22/grn
  // old wire, no sensor installed		// kitchen	B23/blu
  // old wire, no sensor installed		// kit brk L	B24/orn
  // old wire, no sensor installed		// kit brk R	B25/grn
  {  Z_int, S_hi,   28,   57,   56,   D_mot  },	// frnt rm PIR	someday
  {  Z_int, S_hi,   29,   59,   58,   D_mot  },	// din rm PIR	someday
  {  Z_int, S_hi,   30,   61,   60,   D_mot  },	// ply rm PIR	someday
  {  Z_int, S_hi,   31,   63,   62,   D_mot  },	// hallway PIR	someday
  { -1,      -1,   -1,   -1,   -1,   -1 }
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
 * system armed indicator
 */
struct ArmCfg armCfg = { A0, 0, 1024 };

/*
 * this is the configuration for the LEDs
 * (duty cycles and blink rates)
 */
const short ledparms[] PROGMEM = 
//	red	green	off	slow	med	fast
{	100,	100,	200,	1000,	500,	250 };

#define	LED_red		0
#define	LED_green	1
#define	LED_off		2
#define	LED_blink_slow	3
#define	LED_blink_med	4
#define	LED_blink_fast	5

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

	// load up the system armed indicator configuration
	arm = &armCfg;

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
