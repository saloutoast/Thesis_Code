#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:TriCentering.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

void detach(double);

static volatile char distance = 0; // variables for reception ISR
static volatile char rcvd = 0;
static volatile char rcving = 0;
static volatile char bit_time = 0;
static volatile int rcv_sx = 0;
static volatile char lastRcv = 0;
static volatile char rcv_ct = 0;

static volatile int bits_sent = 0; // variables for sending ISR
static volatile int new_bit = 0;
static volatile int pause = 0;

static volatile char toSend = 0xC9; // message variables
static volatile char beaconID1 = 0xAB;
static volatile char beaconID2 = 0x93;
static volatile char beaconID3 = 0xD5;
static volatile char mobileID = 0xC9;

static volatile int rcv_time = 0; // neighbor variables
static volatile int beaconID1_time = 0;
static volatile int beaconID2_time = 0;
static volatile int beaconID3_time = 0;
static volatile char beacons_rcvd = 0;
static volatile char desired_beacon = 0;
static volatile int center_threshold = 10;

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7); // enable EPM pins
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
	TCCR1B |= (1<<CS12)|(0<<CS11)|(1<<CS10); // 1/1024 prescaler, counts at ~7.8kHz (1 count is 0.128 ms), 16-bit timer

	// Initialize timer2 for timing receiving of messages (1/128 prescaler, 8-bit timer rolls over at ~250 Hz )
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 200; // compare every 200 counts (every 3.2ms, 2x length of message)
    TCCR2B |= (1<<CS22)|(0<<CS21)|(1<<CS20); // prescaler of 1/128: count every 16us

	// test power by turning on LEDs
	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	PORTC |= (1<<PORTC3);
	
	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	PORTC &= ~(1<<PORTC3);

	// wait here for a time (~20s) until all modules are spinning, then blink LEDs again
	int ww=0;
	while (ww<300) {

		_delay_ms(100);
		ww+=1;

	}

	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // yellow
	PORTB |= (1<<PORTB2); // red
	
	_delay_ms(200);  

	PORTB &= ~(1<<PORTB0); // turn off LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);

	sei(); // enable interrupts

	// at 140 rpm, period should be about 3344 timer1 counts
	int per = 0;
	//int cur_time = 0;
	int detach_time = 0;
	int dd = 0;
	
	while(1) { // main loop

		if (toSend==mobileID) { // don't bother trying to track beacons if you are not the mobile robot
		
			// take 10 messages to calculate period		
			if ((rcv_sx==1) && (rcv_ct<10)) {
				if (lastRcv==beaconID1) { // only messages from beacon 1 for calculating period
					PORTB |= (1<<PORTB2); // turn on LED to indicate calibration
					if (rcv_time>700) {
						per = (per+rcv_time)/2;
						if (rcv_ct==9) {
							detach_time = per/5; // time after receiving a message that it will detach the EPM
							//detach_time = detach_time/8; // convert roughly to ms
							PORTB &= ~(1<<PORTB2); // clear LED to indicate end of calibration
						}
						rcv_ct+=1;	
					}
					rcv_sx=0;
				}
			}

			// calculate angles based on times between beacon messages, then pick beacon to move towards
			// rotation A: take in three messages, calculate "angles" (times between receptions)
			// rotation B: move towards selected beacon (towards beacon not asociated with the largest angle)
		
			/*if ((rcv_sx==1) && (rcv_ct==10)) { // got a new message and already calibrated
				if (beacons_rcvd < 3) { // store times from the three beacons in a row
					if (lastRcv==beaconID1) { // if other two times are 0, store time and add to beacons rcvd; else reset
						if ((beacons_rcvd==0) && (beaconID2_time==0) && (beaconID3_time==0)) {
							beaconID1_time |= rcv_time;
							beacons_rcvd = 1;
							PORTB |= (1<<PORTB0);

						} else {
							if (rcv_time > 100) {
								beaconID1_time = 0;
								beaconID2_time = 0;
								beaconID3_time = 0;
								beacons_rcvd = 0;
								PORTB &= ~(1<<PORTB0);
							}
						}
					}
					else if (lastRcv==beaconID2) { // if time3 is zero and time1 is not 0, store time and add to beacons rcvd; else ignore
						if ((beacons_rcvd==1) && (beaconID3_time==0) && (beaconID1_time>0)) {
							beaconID2_time |= rcv_time;
							beacons_rcvd = 2;
						} else {
							if (rcv_time > 100) {
								beaconID1_time = 0;
								beaconID2_time = 0;
								beaconID3_time = 0;
								beacons_rcvd = 0;
								PORTB &= ~(1<<PORTB0);
							}
						}
					}
					else if (lastRcv==beaconID3) { // if other two times are not zero, store time and add to beacons rcvd; else ignore
						if ((beacons_rcvd==2) && (beaconID1_time>0) && (beaconID2_time>0)) {
							beaconID3_time |= rcv_time;
							beacons_rcvd = 3;
						} else {
							if (rcv_time > 100) {
								beaconID1_time = 0;
								beaconID2_time = 0;
								beaconID3_time = 0;
								beacons_rcvd = 0;
								PORTB &= ~(1<<PORTB0);
							}
						}
					}
				}
				rcv_sx = 0;
			} */

			// if 3->1 (beaconID1_time) is the longest time, move to beacon 2
			// if 1->2 (beaconID2_time) is the longest time, move to beacon 3
			// if 2->3 (beaconID3_time) is the longest time, move to beacon 1

			// calculate movement
			if ((rcv_sx==1)&&(rcv_ct==10)) {

				beacons_rcvd = 3;

				if (beacons_rcvd==3) {

					beaconID1_time = 1000; // move towards beaconID2, if this is largest
					beaconID2_time = 1000; // move towards beaconID3, if this is largest
					beaconID3_time = 1000; // move towards beaconID1, if this is largest
						

					if ((beaconID1_time>(beaconID2_time+center_threshold)) && (beaconID1_time>(beaconID2_time+center_threshold))) {
						desired_beacon |= beaconID2;
					} else if ((beaconID2_time>(beaconID1_time+center_threshold)) && (beaconID2_time>(beaconID3_time+center_threshold))) {
						desired_beacon |= beaconID3;
					} else if ((beaconID3_time>(beaconID1_time+center_threshold)) && (beaconID3_time>(beaconID1_time+center_threshold))) {
						desired_beacon |= beaconID1;
					} else { // within centering threshold, end of program
						while(1) { 
							PORTB |= (1<<PORTB0) | (1<<PORTB1) | (1<<PORTB2); 
						}
					}

					beacons_rcvd=4; // indicated that direction of motion has been decided
					
				}
				// execute movement
				if (beacons_rcvd==4) {
					if(lastRcv==desired_beacon) { // if last message is from desired beacon -> start movement sequence
						cli(); // disable all interrupts so that movement can be executed
						PORTB |= (1<<PORTB2);

						// delay for detach time
						dd = 0;
						while (dd<detach_time) {
							_delay_us(140);
							dd+=1;
						}					
						detach(100);
						// reset movement variables
						beaconID1_time = 0;
						beaconID2_time = 0;
						beaconID3_time = 0;
						beacons_rcvd = 0;
						desired_beacon = 0;
						PORTB &= ~(1<<PORTB2);
						sei(); // re-enable interrupts again to plan next movement
					}
				}
				rcv_sx=0;
			}
		
			/*	if(lastRcv==beaconID) { // if the beacon is sensed
					// if the weight is perpendicular to the desired line of motion, detach
					cur_time = 0;
					cur_time |= TCNT1;
					if ( (cur_time<detach_time)&(cur_time>detach_time-20) ) { // within a small window of the detach time
						
						detach(100);

					}
				} else {
					// do nothing?
				}
			
			} */

			/* if (rcv_sx==1) {
				if ( ((cur_time < (near+5))&(cur_time > (near-5))) | ((cur_time < (far+5))&(cur_time > (far-5))) ) {
					PORTB |= (1<<PORTB0);
				} else {
					PORTB &= ~(1<<PORTB0);
				}
			}  */
		
			// each cycle hould take 400ms total (~160 rpm)
			//detach(100); //move for 100ms
			//_delay_ms(300);
		} 

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
			// TODO: add LED debugging for each bit
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
				PORTB |= (1<<PORTB1);
				//PORTB &= ~(1<<PORTB0);
				//if (lastRcv==toRcv1) { PORTB |= (1<<PORTB2); }
				//if (lastRcv==toRcv2) { PORTB |= (1<<PORTB0); }

				rcv_time = 0;
				rcv_time |= TCNT1;
				TCNT1 = 0; // reset timer1 on received messages
				
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
		
		// TODO: could disable ISR here, for send_msg function?
	}

}

// "move" by detaching magnet for a specified time in ms
void detach(double time) {

	//switch E.P.M. direction 1 (detach)
	PORTB |= (1<<PORTB0); // set inner LED, indicating direction 1
	PORTB |= (1<<6);//activate E.P.M direction 1
	_delay_us(120);//leave on for 120us
	PORTB &=~(1<<6);//deactivate E.P.M
	PORTB &=~(1<<7);//deactivate E.P.M

	_delay_ms(time); // stay detached for desired time

	//switch E.P.M. direction 2 (re-attach)
	PORTB &= ~(1<<PORTB0); // clear inner LED, indicating direction 2
	PORTB |= (1<<7);//activate E.P.M direction 2
	_delay_us(120);//leave on for 120us
	PORTB &=~(1<<6);//deactivate E.P.M
	PORTB &=~(1<<7);//deactivate E.P.M

	return;
}
