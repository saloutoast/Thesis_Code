#define F_CPU 8000000UL
//avrdude -c usbtiny -B 1000 -p m328p -e -U flash:w:IRComms.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>
#include <stdint.h>
#include <inttypes.h>

#include <math.h>
#include "i2c_master.h"

// adresses of accel/gyro in IMU
#define LSM9DS1_WRITE  0xD4
#define LSM9DS1_READ   0xD5

// register addresses here



void imu_init(void);
void whoAmI(void);
/* int get_ax(void);
int get_ay(void);
int get_gz(void); */

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7);
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	imu_init();

	// test power by turning on LEDs
	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	//PORTC |= (1<<PORTC3); // IR
	
	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	//PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	//PORTC &= ~(1<<PORTC3);

	//sei(); // enable interrupts
	
	//int ii=0;
	while(1) {
		// loop
		
		// read IMU linear accelerations

		// turn on LEDs based on magnitude of acceleration

		// read gyroscope

		// integrate gyroscope

		// turn on LEDs based on heading position


		whoAmI(); // test comms

		//_delay_ms(100); // delay to keep loop from running too quickly
	}

}


void imu_init(void) {
	
	i2c_init();

	// set necessary register values
}

void whoAmI(void) {
	
	PORTB &= ~(1<<PORTB1);	

	i2c_start(LSM9DS1_WRITE);
	i2c_write(0x0F); // who am I register 
	i2c_stop();

	i2c_start(LSM9DS1_READ);
	int who = i2c_read_nack();
	i2c_stop();

	if (who == 0b01101000) { // who am I register value
		PORTB |= (1<<PORTB0);
	} else {
		PORTB |= (1<<PORTB2);
	} 

	_delay_ms(200);

	PORTB |= (1<<PORTB1);
}

