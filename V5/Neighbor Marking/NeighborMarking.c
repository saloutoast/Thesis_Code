#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:NeighborMarking.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

static volatile char distance = 0; // variables for reception ISR
static volatile char rcvd = 0;
static volatile char rcving = 0;
static volatile char bit_time = 0;
static volatile int rcv_sx = 0;
static volatile char lastRcv = 0;

static volatile int bits_sent = 0; // variables for sending ISR
static volatile int new_bit = 0;
static volatile int pause = 0;

static volatile char toSend = 0x93; // message variables/IDs: 0x83, 0x85, 0x87
static volatile char ID1 = 0xAB;
static volatile char ID2 = 0x93;
static volatile char ID3 = 0xD5;
static volatile char ID4 = 0xC9;

static volatile int rcv_time = 0; // neighbor marking variables
static volatile int near = 0;
static volatile int far = 0;

#define NUM_NEIGHBORS 5
static volatile char neighbors[NUM_NEIGHBORS][3]; // array for neighbor information

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs

	DDRB |= (1<<7); // enable EPM control pins
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	DDRC=0; // enable IR LED
	PORTC=0;
	DDRC = (1<<DDC3);

	cli(); // disable interrupts

	// Initialize analog compare pins
    DIDR1 = (1<<AIN1D) | (1<<AIN0D); // Disable the digital input buffers
    ACSR = (1<<ACIE) | (1<<ACIS1) | (1<<ACIS0); // Setup the comparator: enable interrupt, interrupt on rising edge

	// Initialize timer0 for timing sending of messages (1/8 prescaler, 8-bit timer rolls over at ~3.9 kHz Hz)
	TCCR0A |= (1<<WGM01); // // do not change any output pin, clear at compare match with OCR0A
	TCCR0B |= (0<<CS02)|(1<<CS01)|(0<<CS00); // prescaler of 1/8
	OCR0A = 100; // interrupt every 100 counts, for sending a new bit every 2 cycles
	TIMSK0 |= (1<<OCIE0A);

	// Initialize timer1 for neighbor-marking, based on times of received messages
	TCCR1B |= (1<<CS12)|(0<<CS11)|(1<<CS10); // 1/1024 prescaler, counts at ~7.8kHz, 16-bit timer

	// Initialize timer2 for timing receiving of messages (1/128 prescaler, 8-bit timer rolls over at ~250 Hz )
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 200; // compare every 200 counts (every 3.2ms, 2x length of message)
    TCCR2B |= (1<<CS22)|(0<<CS21)|(1<<CS20); // prescaler of 1/128: count every 16us

	// test power by turning on LEDs
	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	PORTC |= (1<<PORTC3); // IR
	
	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	PORTC &= ~(1<<PORTC3);
	
	sei(); // enable interrupts	

	/*
	// tuning period to start spinning robot and tune speed of table to 180rpm
	int ww = 0;
	while (ww<45) { // tune for 15 seconds
		_delay_ms(300);
		PORTB |= (1<<PORTB2); // turn on LED once per rotation
		_delay_ms(33);
		PORTB &= ~(1<<PORTB2);
		ww+=1;
	}
	PORTB |= (1<<PORTB0); // green
	_delay_ms(500);
	PORTB &= ~(1<<PORTB0); // speed tuning should be complete at this point
	*/	
	
	int cur_time = 0;

	// at 140 rpm, period should be about 3344 timer1 counts
	int per140 = 3344;

	near = 836; // initial guess
	far = 3*near;

	while(1) {
		//neighbor marking based on times of messages received, use Timer1

		if (rcv_sx==1) { // new message
			if (lastRcv==ID1) {
				// turn on red LED only
				PORTB |= (1<<PORTB2);
				//PORTB &= ~(1<<PORTB1);
				PORTB &= ~(1<<PORTB0);
			} else if (lastRcv==ID2) {
				// turn on green LED
				PORTB &= ~(1<<PORTB2);
				//PORTB &= ~(1<<PORTB1);
				PORTB |= (1<<PORTB0);
			
			} else if (lastRcv==ID4) {
				// turn on red and green LEDs
				PORTB |= (1<<PORTB2);
				//PORTB &= ~(1<<PORTB1);
				PORTB |= (1<<PORTB0);
			}
		}	
		
		
		
		/*if (rcv_sx==1) { // got a new message

			int rr=0;
			for (rr=0;rr<NUM_NEIGHBORS;rr++) { // check rows in neighbor table
				if (lastRcv == neighbors[rr][0]) { // already received a message from this neighbor
					neighbors[rr][1] = 0;
					neighbors[rr][1] |= rcv_time;
					// update time/heading and recalculate time for LED to turn on?
					break;
				} else if (neighbors[rr][0] != 0) { // that row in the table contains data about a different neighbor
					continue;
				} else { // if the message is from a new neighbor, create a new entry in the table

					neighbors[rr][0] |= lastRcv;
					// store time/heading and calculate time for LED to turn on as well

					break;
				}
			}

		} */

		// check current heading/time

		// check neighbor table to see if any combination of LEDs should be turned on/off

		// send ID message again
	



		/* // two robot communication, based on timing
		if (rcv_sx==1) { // got a new message

			// if the LEDs are in line with the other module
			cur_time = TCNT1;
			if ( ((cur_time < (near+5))&(cur_time > (near-5))) | ((cur_time < (far+5))&(cur_time > (far-5))) ) {
				PORTB |= (1<<PORTB0);
			} else {
				PORTB &= ~(1<<PORTB0);
			}

		} */

	}

}

ISR(ANALOG_COMP_vect) { // essentially the receive_msg() routine

	if (rcving==0) {

		TCNT2=0;
		rcving=1;
		rcvd=0x80;
		ACSR &= ~(1<<ACIS0); // change to falling edge
		rcv_sx = 0; // reset success flag

		//PORTB |= (1<<PORTB0); // clear success LEDs from previous message
		PORTB &= ~(1<<PORTB1);
		//PORTB &= ~(1<<PORTB2);

	} else { // first rising edge has been detected (rcving=1)

		if (!(ACSR&(1<<ACIS0))) { // check for first falling edge

			distance = 0;
			distance |= TCNT2; // use timer value for distance
			ACSR |= (1<<ACIS0); // switch back to rising edge
			//PORTB &= ~(1<<PORTB0);

		} else { // on subsequent rising edges
			//PORTB |= (1<<PORTB0);

			// match rising edges to closest expected time in rcvd
			bit_time = 0;
			bit_time |= TCNT2; // time that rising edge was detected
			
			if ((bit_time>=10)&(bit_time<=17)) { rcvd |= 0x40; }
			else if ((bit_time>=22)&(bit_time<=29)) { rcvd |= 0x20; }
			else if ((bit_time>=35)&(bit_time<=42)) { rcvd |= 0x10; }
			else if ((bit_time>=47)&(bit_time<=54)) { rcvd |= 0x08; }
			else if ((bit_time>=60)&(bit_time<=67)) { rcvd |= 0x04; }
			else if ((bit_time>=72)&(bit_time<=79)) { rcvd |= 0x02; }
			else if ((bit_time>=85)&(bit_time<=92)) { // eighth bit has been received
				rcvd |= 0x01; 
				rcv_sx = 1;
				lastRcv = 0;
				lastRcv |= rcvd; // store message

				// turn on LEDs for success
				//PORTB |= (1<<PORTB0);
				PORTB |= (1<<PORTB1);
				//PORTB |= (1<<PORTB2);
				//PORTB &= ~(1<<PORTB0);
				//if (lastRcv==ID1) { PORTB |= (1<<PORTB2); }
				//if (lastRcv==ID2) { PORTB |= (1<<PORTB0); }

				rcv_time = 0;
				rcv_time |= TCNT1;

				/* if (rcv_time >= 1000) {
					near = rcv_time/4;
					far = 3*near;
				} */

				// TODO: more robust, able to handle multiple neighbors...only reset on first neighbor in table
				TCNT1 = 0; // reset timer1 on received messages (MOVE THIS RESET TO MAIN LOOP)

				rcving = 0; // reset receiving variables
				TCNT2 = 0;
				rcvd = 0;

			}  else { // bad rising edge means message is bad, discard and reset
				rcving = 0; // reset receiving variables
				TCNT2 = 0;
				rcvd = 0;
			}		
		}
	}

	// simple code to follow pulse train
	/* while(ACSR & (1<<ACO)) {
		PORTB |= (1<<PORTB0);
	}
	PORTB &= ~(1<<PORTB0); */

} 

// reset routine for message reception
ISR(TIMER2_COMPA_vect) { // timer2 interrupt routine

	rcving = 0;
	rcvd = 0;
	//PORTB &= ~(1<<PORTB0);
	PORTB &= ~(1<<PORTB1);
	//PORTB &= ~(1<<PORTB2);

}

// routine for timer0 to send messages, pause for a longer time between messages
ISR(TIMER0_COMPA_vect) { // timer0 interrupt routine

	if (bits_sent<8) { // if the whole message has not been sent
		if (pause==0) { // 0,1 half of bit to be sent
			new_bit = (toSend & (1<<(7-bits_sent))) >> (7-bits_sent);
			if(new_bit==1) { // turn on LEDs
				PORTC |= (1<<PORTC3);
				//PORTB |= (1<<PORTB2);
			} else { // turn off LEDs
				PORTC &= ~(1<<PORTC3);
				//PORTB &= ~(1<<PORTB2);
			}
			pause = 1; // pause after sending a bit
		} else { // pausing between bits
			pause = 0; // send new bit on next interrupt
			PORTC &= ~(1<<PORTC3); // ensure LEDs are low for pause
			//PORTB &= ~(1<<PORTB2);
			bits_sent += 1; // increment bits_sent after each pause
		}
	} else { // if bits_sent >= 8, reset variables and pause for a bit
		if (bits_sent>=40) { // wait for 2 messages, send again
			bits_sent = 0; 
		} else {
			bits_sent+=1; // increment bits_sent for timing between messages
		}
		
		// TODO: could disable ISR (clear some bit) here, for send_msg function?
	}

}

// TODO: routine to send messages, especially if messages end up containing more than just static IDs
/* void send_msg(char msg) { // timing values/delays are calibrated with other 

	// TODO: reset sending variables, enable ISR for sending here? yes

} */

