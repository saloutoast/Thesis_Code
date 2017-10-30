#define F_CPU 8000000UL
//avrdude -c usbtiny -B 1000 -p m328p -e -U flash:w:IRReceive.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

static volatile int last_edge = 0;

static volatile int bits_rcvd = 0;
static volatile int rcving = 0;
static volatile char rcvd1 = 0;
static volatile char rcvd2 = 0;

int main(void) {
	
	// enable pin C5 and pin C4 as outputs
	DDRC = (1<<DDC5) | (1<<DDC4) | (1<<DDC3); // C5: mimic signal, C4: communication bits, C3: message received correctly
	
	// Initialize analog compare pins
    DIDR1 = (1<<AIN1D) | (1<<AIN0D); // Disable the digital input buffers
    ACSR = (1<<ACIE) | (1<<ACIS1) | (1<<ACIS0); // Setup the comparator: enable interrupt, interrupt on rising edge
    
	// Initialize timer2
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 50; // compare every 50 counts (every 50us, 1/10 frequency of communication bits)
    TCCR2B |= (0<<CS22)|(1<<CS21)|(0<<CS20); // prescaler of 1/8: count every 1us

    sei(); // Enable interrupts

	while(1) {
		// loop
	}

}

ISR(ANALOG_COMP_vect) {

    // On rising edge
    if (ACSR & (1<<ACO))
    {
        // Turn on C5 and C4 output
        PORTC = (1<<PORTC5) | (1<<PORTC4); 
			
		// Reset edge counter
		last_edge = 0;       

        // Change to falling edge.
        ACSR &= ~(1<<ACIS0);

        // Store received "1"
        if (rcving==0) {
            rcving = 1; // have received a start bit
        } else { // if rcving=1
            if (bits_rcvd<8) {
                rcvd1 &= (1<<(7-bits_rcvd));
                bits_rcvd += 1;
            } 
            else if (bits_rcvd<16) {
                rcvd2 &= (1<<(15-bits_rcvd));
                bits_rcvd += 1;
            }
            else { // received all 16 bits
                bits_rcvd = 0;
                rcving = 0; // finished receiving
                if ((rcvd1-rcvd2)==0) { // if two messages are the same
                    PORTC = (1<<PORTC3); // turn on C3 output
                }
            }
        }
    }
    else // only necessary if the C5 output is still being used to mimic the signal
    {
        // Turn off C5 output
        PORTC &= ~(1<<PORTC5);

        // Change to rising edge.
        ACSR |= (1<<ACIS0);
    }
} 

ISR(TIMER2_COMPA_vect) {

    if (rcving=1) { // only bother tracking 0 bits if a start bit has been received
        last_edge += 1; // Increment edge counter

        if (last_edge > 10) { // Did not receive a 1 communication bit in over 500us (received a 0 communication bit)

            PORTC &= ~(1<<PORTC4); // Clear C4 output
            
            if (bits_rcvd<16) {
                bits_rcvd += 1; // increment bit_rcvd counter if currently receiving
            } else { // bits_rcvd = 16, done receiving
                bits_rcvd = 0;
                rcving = 0; // finished receiving
                if ((rcvd1-rcvd2)==0) { // if two messages are the same
                    PORTC = (1<<PORTC3); // turn on C3 output
                }
            }

            last_edge = 0; // Reset edge counter

        }
    }
} 
