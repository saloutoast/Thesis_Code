#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRComms.hex -F
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
#define WHO_AM_I_REG   0x0F

#define STATUS_REG0 0x17
#define STATUS_REG1 0x27

#define REFERENCE_G 0x0B
#define CTRL_REG1_G 0x10
#define CTRL_REG2_G 0x11
#define CTRL_REG3_G 0x12
#define ORIENT_CFG_G 0x13

#define CTRL_REG4 0x1E
#define CTRL_REG5_XL 0x1F
#define CTRL_REG6_XL 0x20
#define CTRL_REG7_XL 0x21

#define CTRL_REG8 0x22
#define CTRL_REG9 0x23
#define CTRL_REG10 0x24

#define FIFO_CTRL 0x2E
#define FIFO_SRC 0x2F

#define OUT_TEMP_L 0x15
#define OUT_TEMP_H 0x16

#define OUT_X_L_G 0x18
#define OUT_X_H_G 0x19
#define OUT_Y_L_G 0x1A
#define OUT_Y_H_G 0x1B
#define OUT_Z_L_G 0x1C
#define OUT_Z_H_G 0x1D 

#define OUT_X_L_XL 0x28
#define OUT_X_H_XL 0x29
#define OUT_Y_L_XL 0x2A
#define OUT_Y_H_XL 0x2B
#define OUT_Z_L_XL 0x2C
#define OUT_Z_H_XL 0x2D

void imu_init(void);
void whoAmI(void);

static char gyro_ctrl_reg[3] = {0b01111000, 0b00000000, 0b00000000}; 
// 119Hz ODR, 2000dps, 14Hz cutoff; no filtering enabled

static char xl_ctrl_reg[7] = {0b00111000, 0b00111000, 0b01010000, 0b00000000, 0b00000100, 0b00000000, 0b00000000};
// all 3 gyro axes enabled; all 3 xl axes enabled; 50Hz ODR, +-4g, no bandwidth filter;
// no high resolution mode, no filtering; adress incrementation enabled; i2c enabled, FIFO disabled; no self-test

/* int get_ax(void);
int get_ay(void);
int get_gz(void); */

int main(void) {

	cli(); // disable interrupts

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7);
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	// declare i2c pins as inputs
	DDRC=0;
	PORTC=0;
	PORTC |= (1<<PORTC4);
	PORTC |= (1<<PORTC5);

	// test power by turning on LEDs
	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	//PORTC |= (1<<PORTC3); // IR
	
	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	//PORTC &= ~(1<<PORTC3);

	i2c_init();
	
	//imu_init();

	PORTB |= (1<<PORTB0); // turn on one LED for imu init success

	sei(); // enable interrupts
	
	//int ii=0;
	while(1) {
		// loop
		
		// read IMU linear accelerations

		// turn on LEDs based on magnitude of acceleration

		// read gyroscope

		// integrate gyroscope

		// turn on LEDs based on heading position

		/* PORTB |= (1<<PORTB0);

		whoAmI(); // test comms

		PORTB &= ~(1<<PORTB0);

		int ii=0;
		while(ii<3) {
			_delay_ms(100); // delay to keep loop from running too quickly
			ii++;
		} */


		// loop through all i2c addresses to see if the IMU is active

		char ii=0;

		while (ii<128) {
			char test = 0;
			
			test = i2c_start((ii<<1));
			if(test==0) {
				while(1) {
					PORTB |= (1<<PORTB2);
					_delay_ms(ii);
					PORTB &= ~(1<<PORTB2);
					_delay_ms(ii);
				}
			}
			
			i2c_stop();
			ii++;

			/*int jj=0;
			while(ii<3) {
				_delay_ms(100); // delay to keep loop from running too quickly
				ii++;
			}*/
			_delay_ms(100);
			

		}


	}

}


void imu_init(void) {
	
	// set gyro control registers
	//i2c_writeReg(LSM9DS1_WRITE, CTRL_REG1_G, &gyro_ctrl_reg, 3);

	// set accel control registers
	//i2c_writeReg(LSM9DS1_WRITE, CTRL_REG4, &xl_ctrl_reg, 7);

	// any other registers to set up

}

void whoAmI(void) {
	
	//PORTB |= (1<<PORTB2);	// turn on LED to signal transmission
	char who = 0; // stored rcvd value here
	
	
	//i2c_readReg(LSM9DS1_WRITE, WHO_AM_I_REG, &who, 1); //also could be used
	
	char test=0;
	char test2=0;
	char test3=0;

	test = i2c_start(LSM9DS1_WRITE);
	if (test==2) { PORTB |= (1<<PORTB2); }
		
	test2 = i2c_write(WHO_AM_I_REG);
	if (test2==1) { PORTB |= (1<<PORTB1); }

	test3 = i2c_start(LSM9DS1_READ);
	//if (test3==1) { PORTB |= (1<<PORTB1); }
	
	who = i2c_read_nack();
	i2c_stop();

	/* if (who == 0x68) { // who am I register value (0x68)
		PORTB |= (1<<PORTB1);
	} else if (who == 0x00) {
		PORTB |= (1<<PORTB1);
	} else {
		PORTB |= (1<<PORTB1);
	} */
	//PORTB |= (1<<PORTB2);

	_delay_ms(200);

	PORTB &= ~(1<<PORTB1); // turn off LEDs
	PORTB &= ~(1<<PORTB2);
}

