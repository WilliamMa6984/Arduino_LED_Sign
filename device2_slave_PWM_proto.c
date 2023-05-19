#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

// Bit operations
#define SET_BIT(reg, pin)		(reg) |= (1 << (pin))
#define SET_BITS(reg, mask)		(reg) |= (mask)
#define CLEAR_BIT(reg, pin)		(reg) &= ~(1 << (pin))

// Definitions
// USART addressing
#define ADDRESS		'0'

// Register names
#define ROW_PORT	PORTC
#define COL_PORT	PORTD
#define ROW_DDR		DDRC
#define COL_DDR		DDRD

// Pin numbers
#define ROW1	0
#define ROW2	1
#define ROW3	2

#define COL1	7
#define COL2	6
#define COL3	5

// Initialise matrix
int rows[] = {ROW1, ROW2, ROW3};
int cols[] = {COL1, COL2, COL3};

const int rowsLength = sizeof(rows)/sizeof(rows[0]);
const int colsLength = sizeof(cols)/sizeof(cols[0]);

void setup()
{
	// Initialise registers
	
	// LED row
	uint8_t mask = (1 << ROW1) | (1 << ROW2) | (1 << ROW3);
	SET_BITS(ROW_DDR, mask);
	// Turn all LEDs off (all rows off)
	SET_BITS(ROW_PORT, mask);
	
	// LED col
	mask = (1 << COL1) | (1 << COL2) | (1 << COL3);
	SET_BITS(COL_DDR, mask);

	// Timers for overflows (opacity - PWM)
	SET_BIT(TCCR0B, CS00); // Enable
	// Waveform - Fast PWM
	SET_BIT(TCCR0A, WGM00);
	SET_BIT(TCCR0A, WGM01);
	
	SET_BIT(TIMSK0, OCIE0A); // Overflow (TOIE0, default)
	// Enable Compare interrupts later (OCIE0A)
	OCR0A = 128;
	
	Serial.begin(9600);
	Serial.println("Entry");
}

void turnLED(int rowPin, int colPin)
{
	// Row on
	CLEAR_BIT(ROW_PORT, rowPin);
	// Column on
	SET_BIT(COL_PORT, colPin);
	
	_delay_ms(5);

	// Clear for next
	SET_BIT(ROW_PORT, rowPin);
	CLEAR_BIT(COL_PORT, colPin);
}

void turnLEDs()
{
	turnLED(ROW1, COL1);
	turnLED(ROW2, COL2);
	turnLED(ROW3, COL1);
}

//volatile int LEDsOn[][2] = {NULL}; // Max num points (row * col), each 2 dimensional
void rowToPoint(char stream[], int row)
{
	// Find columns to turn on for each row
	
}

void recieve()
{
	// Input
	/*	e.g.
		{1, 0, 1},
		{0, 1, 0},
		{0, 1, 0}
		
		=> 0 1 101 010 010 \0
		=> address mode matrix End
	*/ 
	
	// Select mode
	//char stream[colsLength] = "0";
	/*switch ((int)stream[1])
	{
		case 0: // OVF
			CLEAR_BIT(TIMSK0, OCIE0A); // Compare A off
			SET_BIT(TIMSK0, TOIE0); // Overflow on
			break;
		case 1: // COMPA
			SET_BIT(TIMSK0, OCIE0A); // Compare A off
			CLEAR_BIT(TIMSK0, TOIE0); // Overflow on
			break;
		default: // Out of bounds - ignore
			break;
	}
	
	stream = "101";
	strToPoint(stream, rows[0]);
	stream = "010";
	strToPoint(stream, rows[1]);
	stream = "010";
	strToPoint(stream, rows[2]);*/
}

int main() {
	setup();
	
	//recieve();
	
	// Turn on interrupts
	sei();
	
	Serial.println("After sei");

	uint8_t count = 128;
	uint8_t up = 1;
	
	while (1) {
		OCR0A = count;
		
		if (up && count < 255)
		{
			count++;
		}
		else if (up && count == 255)
		{
			up = 0;
		}
		else if (!up && count > 50)
		{
			count--;
		}
		else if (!up && count == 50)
		{
			up = 1;
		}
	}
}

ISR (TIMER0_OVF_vect)
{
	turnLEDs();
}
ISR (TIMER0_COMPA_vect)
{
	turnLEDs();
	TCNT0 = 0;
}