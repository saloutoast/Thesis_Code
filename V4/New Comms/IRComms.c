#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRComms.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>


static volatile int starting = 0;
static volatile int start_cycles = 0;
static volatile int rcving = 0;
static volatile int rcv_cycles = 0;

static volatile int bits_rcvd = 0;
static volatile int new_bit = 0;
static volatile char rcvd1 = 0;
static volatile char rcvd2 = 0;

static volatile char toSend = 0b00000000; // some sequence of 8 bits here
static volatile int bits_sent = 0;
static volatile int next_bit = 0;
static volatile int sending = 0;
static volatile int pausing = 0;
static volatile int cycle = 0;

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7);
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	DDRC=0;
	PORTC=0;
	DDRC = (1<<DDC3);
		
	//PORTC |= (1<<PORTC3); // turn on IR LED	

	cli(); // disable interrupts

	// Initialize analog compare pins
    DIDR1 = (1<<AIN1D) | (1<<AIN0D); // Disable the digital input buffers
    ACSR = (1<<ACIE) | (1<<ACIS1) | (1<<ACIS0); // Setup the comparator: enable interrupt, interrupt on rising edge

	// Initialize timer1 for transmitting (currently at 2000 bits per second, might be too fast 
    TCCR1B |= (1<<WGM12); // do not change any output pin, clear at compare match with OCR1A
	TIMSK1 = (1<<OCIE1A); // compare match on OCR1A
	OCR1A = 50; // compare every 50 counts (every 50us (20kHz), 10x frequency of communication bits)
	TCCR1B |= (0<<CS12)|(1<<CS11)|(0<<CS10); // prescaler of 1/8: count every 1us 
    
    // Initialize timer2 for receiving
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 50; // compare every 50 counts (every 50us (20kHz), 10x frequency of communication bits)
    TCCR2B |= (0<<CS22)|(1<<CS21)|(0<<CS20); // prescaler of 1/8: count every 1us
	
	sei(); // enable interrupts

	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // turn on middle LED
	PORTB |= (1<<PORTB2); // red
	PORTC |= (1<<PORTC3); // IR

	_delay_ms(100);

	PORTB &= ~(1<<PORTB0); // clear debugging LEDs
	PORTB &= ~(1<<PORTB1);
	PORTB &= ~(1<<PORTB2);
	PORTC &= ~(1<<PORTC3);	

	int ii=0;
	while(1) {
		// loop, blinking LED to show power is on
		
		/*PORTC |= (1<<PORTC3);
		PORTB |= (1<<PORTB2);
		
		while(ii<13) {
			_delay_us(50);
			ii++;
		}
		ii=0;

		PORTC &= ~(1<<PORTC3);
		PORTB &= ~(1<<PORTB2);

		while(ii<10) {
			_delay_ms(10);
			ii++;
		}
		ii=0;*/

		ii++;
	}

}

ISR(ANALOG_COMP_vect) {

    // On rising edge
    if (ACSR & (1<<ACO))
    {
		
		//PORTB |= (1<<PORTB0);

		if ((starting==0)&(rcving==0)) // if start bit not received (AND RCVING==0)
		{
			TCNT2 = 0;
			starting = 1;
			start_cycles = 0;

			ACSR &= ~(1<<ACIE);
			ACSR &= ~(1<<ACIS0); // change to falling edge
			ACSR |= (1<<ACIE);
			

			PORTB |= (1<<PORTB0);
			PORTB &= ~(1<<PORTB2);

		}

		//ACSR &= ~(1<<ACIS0);

    } else { // on falling edge

		//PORTB &= ~(1<<PORTB0);

		if (starting==1) { // start bit rising edge already received
			if ((start_cycles>70)&(start_cycles<=100)) { // test for end of start bit
				rcving = 1;
				PORTB &= ~(1<<PORTB0);
				PORTB &= ~(1<<PORTB1);
				PORTB &= ~(1<<PORTB2);

				ACSR &= ~(1<<ACIE);				
				ACSR |= (1<<ACIS0); // change back to rising edge
				ACSR |= (1<<ACIE);

				TCNT2 = 0; // reset timer2 counter to be insync with the communications
				bits_rcvd=0; // track number of bits received

				start_cycles = 0;
				starting = 0;

			} else if (start_cycles>100) { // not a start bit, reset starting criteria

				PORTB &= ~(1<<PORTB0);
				PORTB &= ~(1<<PORTB1);
				PORTB &= ~(1<<PORTB2);	

				ACSR &= ~(1<<ACIE);
				ACSR |= (1<<ACIS0); // change back to rising edge
				ACSR |= (1<<ACIE);

				start_cycles = 0;
				starting = 0;

			}
		} 

		//ACSR |= (1<<ACIS0);

	} // rest of reception handled by the timer2 interrupt routine
} 

// RECEIVING
ISR(TIMER2_COMPA_vect) { // timer2 interrupt routine

    if (starting==1) {
		start_cycles+=1; // count length of start bit
		
		/*if (start_cycles>100) // glitch out, missed falling edge
		{ 			
			PORTB |= (1<<PORTB2);			
			PORTB &= ~(1<<PORTB0);
			ACSR |= (1<<ACIS0);			
			start_cycles = 0;
			starting = 0;
		}*/		

	}
	if (rcving==1) { // track pulse timing from start bit onward

		rcv_cycles+=1; // start tracking when to sample ACO

		if ((bits_rcvd==0)&(rcv_cycles==50)) { // first sample is at half of a cycle, plus the delay at the end of the start signal
			
			// get ACO value
			new_bit = ((ACSR&(1<<ACO)));
			//if (new_bit==1) { PORTB |= (1<<PORTB2); }
			
			
			// store first bit
			rcvd1 |= (new_bit<<(7-bits_rcvd));

			bits_rcvd+=1; 
			rcv_cycles=0; 
		}

		if (rcv_cycles==50) { // next samples are exaclty out of phase with sent bits

			// get ACO value
			new_bit = ((ACSR&(1<<ACO))>>5);
			//if (new_bit==1) { PORTB |= (1<<PORTB2); }
			
			// store bit
			if (bits_rcvd<8) { rcvd1 |= (new_bit<<(7-bits_rcvd)); }
			else if (bits_rcvd<16) { rcvd2 |= (new_bit<<(15-bits_rcvd)); }

			bits_rcvd+=1;
			rcv_cycles=0;
		}


		if (bits_rcvd==16) { // both messages have been received
			
			// compare messages, take action (light LED or ignore)
			PORTB |= (1<<PORTB1);
			if ( rcvd1 == rcvd2 ) {
				//PORTB |= (1<<PORTB1); // messages are the same, light up an LED
				if (rcvd1 == toSend) { 
					PORTB |= (1<<PORTB2);
					PORTB &= ~(1<<PORTB0);
				}				
			}

			// reset everything for next message to be received			
			starting = 0;
			start_cycles = 0;
			rcving = 0;
			rcv_cycles = 0;	
			rcvd1 = 0;
			rcvd2 = 0;

		}
    } 
} 

// TRANSMISSION
ISR(TIMER1_COMPA_vect) { // timer1 interrupt routine

	
	/* if (cycle==0) {
		PORTC |= (1<<PORTC3); // turn on start bit
		PORTB |= (1<<PORTB2);
		cycle++;
	}
	else if (cycle==65) {
		PORTC &= ~(1<<PORTC3);
		PORTB &= ~(1<<PORTB2);
		cycle++;
	}
	else if (cycle==10000) {
		cycle=0;
	}
	else {cycle++;} */

	if ((rcving==0)&(starting==0)) {
	
		if (pausing==0) { // if not pausing
			if (sending==0) { // send start bit on C3 
				
				PORTC |= (1<<PORTC3);
				PORTB |= (1<<PORTB2);
				cycle+=1; // start incrementing cycle to determine length of start bit
				
				if (cycle==75) {
					PORTC &= ~(1<<PORTC3); // clear start bits
					PORTB &= ~(1<<PORTB2);
				}
				if (cycle==100) { // short low signal for end of start bit
					cycle = 50;	// to start sending message right away at next ISR call
					sending = 1; // proceed with sending the rest of the message
				}

				
			}
			else { // sending = 1
				if (cycle==50) {

					if (bits_sent<8) { // first message
						next_bit = (toSend & (1<<(7-bits_sent))) >> (7-bits_sent);

						if (next_bit==1) { 
							PORTC |= (1<<PORTC3); 
							PORTB |= (1<<PORTB2);
						} // if bit is 1, set C3
						else { 
							PORTC &= ~(1<<PORTC3); 
							PORTB &= ~(1<<PORTB2);
						} //if bit is 0, clear C3

						bits_sent += 1;

					} else if (bits_sent < 16) { // second message
						next_bit = (toSend & (1<<(15-bits_sent))) >> (15-bits_sent);

						if (next_bit==1) { 
							PORTC |= (1<<PORTC3); 
							PORTB |= (1<<PORTB2);
						} // if bit is 1, set C3
						else { 
							PORTC &= ~(1<<PORTC3); 
							PORTB &= ~(1<<PORTB2);	
						} // if bit is 0, clear C3

						bits_sent += 1;

					} else { // two messages have been sent, bits_sent=16
						sending = 0;
						pausing = 1;
						bits_sent = 0;
						//toSend = 0;
					}
					cycle = 0;

				} 
				cycle += 1; // increment cycle counter

			}	
		}
		else { // if pausing
	
			cycle +=1; 
			PORTB &= ~(1<<PORTB2);

			if (cycle==10000) { // pause for 0.5s after sending message
				cycle=0;
				pausing=0;

			}  
		}
	}
}

