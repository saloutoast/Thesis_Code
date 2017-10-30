#define F_CPU 8000000UL
//avrdude -c usbtiny -B 1000 -p m328p -e -U flash:w:IRReceive.hex -F
//write fuses: -U lfuse:w:0xE2:m -F

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

static volatile int last_edge = 0;

int main(void) {
	
	// enable pin C5 and pin C4 as outputs
	DDRC = (1<<DDC5) | (1<<DDC4); // C5 will mimic signal, C4 will be communication bits
	
	// Initialize analog compare pins
    DIDR1 = (1<<AIN1D) | (1<<AIN0D); // Disable the digital input buffers
    ACSR = (1<<ACIE) | (1<<ACIS1) | (1<<ACIS0); // Setup the comparator: enable interrupt, interrupt on rising edge
    
	// Initialize timer2
	TCCR2A |= (1<<WGM21); // do not change any output pin, clear at compare match with OCR2A
	TIMSK2 = (1<<OCIE2A); // compare match on OCR2A
    OCR2A = 50; // compare every 150 counts (every 150us, 1/10 frequency of communication bits)
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
    }
    else
    {
        // Turn off C5 output
		PORTC &= ~(1<<PORTC5);

        // Change to rising edge.
        ACSR |= (1<<ACIS0);
    }
} 

ISR(TIMER2_COMPA_vect) {

	last_edge += 1; // Increment edge counter

	if (last_edge > 10) { // Did not receive a 1 communication bit in over 500us

		PORTC &= ~(1<<PORTC4); // Clear C4 output
		last_edge = 0; // Reset edge counter

	}
	
} 
