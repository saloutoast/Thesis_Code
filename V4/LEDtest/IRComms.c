#define F_CPU 8000000UL
//avrdude -c usbtiny -B 10 -p m328p -e -U flash:w:IRComms.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>



static volatile int last_edge = 0;
static volatile int debug_count = 0;
static volatile int bits_rcvd = 0;
static volatile int rcving = 0;
static volatile char rcvd1 = 0;
static volatile char rcvd2 = 0;

static volatile int cycle = 0;
static volatile char toSend = 0b01100110; // some sequence of 8 bits here
static volatile char toRcv = 0b01100110; // desired sequence of 8 bits here
//static volatile char nextSend = 0b00000001;
static volatile int bits_sent = 0;
static volatile int next_bit = 0;
static volatile int sending = 0;
static volatile int pausing = 0;

int main(void) {

	DDRB=0;
	PORTB=0;
	DDRB = (1<<DDB0) | (1<<DDB1) | (1<<DDB2); // enable debugging LEDs
	DDRB |= (1<<7);
	DDRB |= (1<<6);
	PORTB &=~(1<<6);
	PORTB &=~(1<<7);

	PORTB |= (1<<PORTB0); // green
	PORTB |= (1<<PORTB1); // turn on middle LED
	//PORTB |= (1<<PORTB2); // red

	DDRC=0;
	PORTC=0;
	DDRC = (1<<DDC3);
		
	//PORTC |= (1<<PORTC3); // turn on IR LED	

	// Initialize analog compare pins
    DIDR1 = (1<<AIN1D) | (1<<AIN0D); // Disable the digital input buffers
    ACSR = (1<<ACIE) | (1<<ACIS1) | (1<<ACIS0); // Setup the comparator: enable interrupt, interrupt on rising edge

	// Initialize timer1 for transmitting
    TCCR1B |= (1<<WGM12); // do not change any output pin, clear at compare match with OCR1A
	TIMSK1 = (1<<OCIE1A); // compare match on OCR1A
	OCR1A = 50; // compare every 50 counts (every 50us, 1/10 frequency of communication bits)
	TCCR1B |= (0<<CS12)|(1<<CS11)|(0<<CS10); // prescaler of 1/8: count every 1us 
    
    // Initialize timer2 for receiving
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 50; // compare every 50 counts (every 50us, 1/10 frequency of communication bits)
    TCCR2B |= (0<<CS22)|(1<<CS21)|(0<<CS20); // prescaler of 1/8: count every 1us
	
	sei(); // enable interrupts
	
	int ii=0;
	while(1) {
		// loop
		
		/* //switch E.P.M. direction 1 
		PORTB |= (1<<PORTB1); // set middle LED
		PORTB |= (1<<7);//activate E.P.M direction 1
		_delay_us(80);//leave on for 80us
		PORTB &=~(1<<6);//deactivate E.P.M
		PORTB &=~(1<<7);//deactivate E.P.M
		PORTB &= ~(1<<PORTB1); //clear middle LED
		_delay_ms(140);//delay 140 ms
		
		

		PORTB &= ~(1<<PORTB2); // clear outer LED
		PORTB |= (1<<6);//activate E.P.M direction 2
		_delay_us(80);//leave on for 80us
		PORTB &=~(1<<6);//deactivate E.P.M
		PORTB &=~(1<<7);//deactivate E.P.M
		PORTB |= (1<<PORTB2); // set outer LED
		_delay_ms(140);//delay 140ms */

		
		PORTB |= (1<<PORTB1);
		PORTB |= (1<<PORTB2);
		//PORTC |= (1<<PORTC3);
		
		while(ii<10) {
			_delay_ms(30);
			ii++;
		}
		ii=0;

		PORTB &= ~(1<<PORTB1);
		PORTB &= ~(1<<PORTB2);
		//PORTC &= ~(1<<PORTC3);

		while(ii<10) {
			_delay_ms(30);
			ii++;
		}
		ii=0;
	}

}

ISR(ANALOG_COMP_vect) {

    // On rising edge
    if (ACSR & (1<<ACO))
    {
		// Reset edge counter
		last_edge = 0;       

        // Change to falling edge.
        ACSR &= ~(1<<ACIS0);

        // Store received "1"
        if (rcving==0) {
			//PORTB |= (1<<PORTB2); // turn on output for start bit
            rcving = 1; // have received a start bit
        } else { // if rcving=1
			//PORTB |= (1<<PORTB1); // turn on for comm bits
            if (bits_rcvd<8) {
                rcvd1 |= (1<<(7-bits_rcvd));
                bits_rcvd += 1;
            } 
            else if (bits_rcvd<16) {
                rcvd2 |= (1<<(15-bits_rcvd));
                bits_rcvd += 1;
            }
            else { // received all 16 bits
                bits_rcvd = 0;
                rcving = 0; // finished receiving
				//PORTB &= ~(1<<PORTB2);	
                if ((rcvd1==toRcv)&&(rcvd2==toRcv)) { // if two messages are the same
                //if (rcvd1==rcvd2) {
					PORTB |= (1<<PORTB0); // message received successfully
					debug_count = 0;
                }
				rcvd1 = 0;
				rcvd2 = 0;
            }
        }
    }
    else 
    {
        // Change to rising edge.
        ACSR |= (1<<ACIS0);
    }
} 

ISR(TIMER2_COMPA_vect) { // timer2 interrupt routine

    if (rcving==1) { // only bother tracking 0 bits if a start bit has been received
        last_edge += 1; // Increment edge counter

        if (last_edge>12) { // Did not receive a rising edge in over 600us (received a 0 communication bit)

            //PORTB &= ~(1<<PORTB1); // Clear output
            
            if (bits_rcvd<16) {
                bits_rcvd += 1; // increment bit_rcvd counter if currently receiving
            } else { // bits_rcvd = 16, done receiving
                bits_rcvd = 0;
                rcving = 0; // finished receiving
				//PORTB &= ~(1<<PORTB2); 
                if ((rcvd1==toSend)&&(rcvd2==toSend)) { // if two messages are the same
                //if (rcvd1==rcvd2) { 
					PORTB |= (1<<PORTB0); // turn on debugging LED
					debug_count = 0; //reset count for debugging LED
                }
				rcvd1 = 0;
				rcvd2 = 0;
            }
            last_edge = 2; // Reset edge counter
        }
    }
	if (rcving==0) {
		debug_count += 1;
		if (debug_count == 100) { // it has been 10ms
			PORTB &= ~(1<<PORTB0); // clear debugging LED
		}
	}
} 

ISR(TIMER1_COMPA_vect) { // timer1 interrupt routine
	
	if (pausing==0) { // if not pausing
		if (sending==0) { // send start bit on C3
			PORTC |= (1<<PORTC3);
			PORTB |= (1<<PORTB2);
			cycle = 0;
			sending = 1;
		}
		else { // sending = 1
			if (cycle==10) {

				if (bits_sent<8) { // first message
					next_bit = (toSend & (1<<(7-bits_sent))) >> (7-bits_sent);

					if (next_bit==1) { 
						PORTC |= (1<<PORTC3); 
						PORTB |= (1<<PORTB2);
					} // if bit is 1, set C3
					else { 
						PORTC &= ~(1<<PORTC3); 
						PORTB &= ~(1<<PORTB2);
					} // if bit is 0, clear C3

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

			} else if (cycle==2) {
				PORTC &= ~(1<<PORTC3); // always clear bit after 2 cycles
				PORTB &= ~(1<<PORTB2);
				cycle += 1;
			} else { 
				cycle += 1; // increment cycle counter
			}
		}	
	}
	else { // if pausing
		
		cycle +=1; 
		if (cycle==200) { // pause for 200*50 us after sending message
			cycle=0;
			pausing=0;
			
			//toSend = nextSend+1; // update character to send from queue
			//nextSend = 0;
		}  
	}
}

