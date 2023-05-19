#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

void setupLEDs();
void setupTimers();
void setupUART();
void blinkLED(int rowPin, int colPin);
void OCRAUpdate();
void turnOnLEDs();
void clearLEDs();
void processUARTByte();
// TESTING ONLY DELETE LATER
void uart_put_string(char string[]);
void uart_putbyte(unsigned char data);

// Bit operations
#define SET_BIT(reg, pin)			(reg) |= (1 << (pin))
#define SET_BITS(reg, mask)			(reg) |= (mask)
#define CLEAR_BIT(reg, pin)			(reg) &= ~(1 << (pin))
#define CLEAR_BITS(reg, mask)		(reg) &= ~(mask)
#define BIT_VALUE(reg, pin)			(((reg) >> (pin)) & 1)
#define BIT_IS_SET(reg, pin)	    (BIT_VALUE((reg),(pin))==1) 

// Device specs
#define BAUD		9600
// Register names
#define ROW_PORT	PORTC
#define COL_PORT	PORTD
#define ROW_DDR		DDRC
#define COL_DDR		DDRD
// Pin numbers
#define ROW1		0
#define ROW2		1
#define ROW3		2
#define COL1		7
#define COL2		6
#define COL3		5

// LED matrix specs
#define rowOffset	0
#define colOffset	0

// Initialise matrix
const int rowPins[] = {ROW1, ROW2, ROW3};
const int colPins[] = {COL1, COL2, COL3};
const int rowSpan = sizeof(rowPins)/sizeof(rowPins[0]);
const int colSpan = sizeof(colPins)/sizeof(colPins[0]);
#define MAX_MTRX_POINTS		rowSpan*colSpan
#define MAX_MTRX_PATTERN_STEPS		20

// Matrix points to flash, each 2 dimensional
// Num of points in matrix = rows * length
volatile int mtrxPoints[MAX_MTRX_PATTERN_STEPS][MAX_MTRX_POINTS][2] = {-1};
volatile int numPoints[MAX_MTRX_PATTERN_STEPS] = {0};

// Matrix settings
volatile int opacityMode = 0; // Default
volatile int patternTime = 0;

/* --------------- LED Matrix --------------- */
void blinkLED(int rowPin, int colPin)
{
	// Row on
	CLEAR_BIT(ROW_PORT, rowPin);
	// Column on
	SET_BIT(COL_PORT, colPin);
	
	_delay_ms(5);

	// Reset to clear
	SET_BIT(ROW_PORT, rowPin);
	CLEAR_BIT(COL_PORT, colPin);
}

void OCRAUpdate()
{
	// OCR0A change counter and direction
	static uint8_t count = 20;
	static uint8_t up = 1;
	
	OCR0A = count; // Set compare value
	
	// Incrementing compare value
	if (up)
	{
		if (count < 255)
		{
			count++;
		}
		else if (count == 255)
		{
			up = 0;
			return;
		}
	}

	// Decrementing compare value
	if (!up)
	{
		if (count > 5)
		{
			count--;
		}
		else if (count == 5)
		{
			up = 1;
		}
	}
}

void turnOnLEDs()
{
	// Display each point in matrix
	for (int index = 0; index <= numPoints[index]; index++)
	{
		// Blink LED at row, column
		blinkLED(mtrxPoints[patternTime][index][0], mtrxPoints[patternTime][index][1]);
	}
}

// PWM For setting opacity
ISR (TIMER0_COMPA_vect)
{
	clearLEDs();
  	TCNT0 = 0;
}

void clearLEDs()
{
	// LED row
	uint8_t mask = (1 << ROW1) | (1 << ROW2) | (1 << ROW3);
	SET_BITS(ROW_PORT, mask);
}

/* --------------- Receiver --------------- */
// Store the max timesteps of this pattern
volatile int maxTimestep = 0;

// 'USART Received' interrupt
ISR(USART_RX_vect)
{
	char ch = UDR0; // Receive byte
	processUARTByte(ch);
}

// Process each character received
void processUARTByte(char byte)
{
	// Matrix point counters
	static int row = 0;
	static int col = 0;
	static int mtrxPointIndex = 0;
	static int timestep = 0;
	
	switch (byte)
	{
		case '1': // LED point - on
			// Check if within bounds of this device's handling of LED matrix
			// based on row/column offset/span of matrix

			if (col >= colOffset && col <= colOffset + colSpan &&
				row >= rowOffset && row <= rowOffset + rowSpan)
			{
				// Found point
				mtrxPoints[timestep][mtrxPointIndex][0] = rowPins[row-rowOffset];
				mtrxPoints[timestep][mtrxPointIndex][1] = colPins[col-colOffset];
			}

			mtrxPointIndex++; // Ready for next point
			col++;
			break;
		case 4: // End of transmission
			maxTimestep = timestep;
			timestep = 0;
			break;
		case ',': // End of row - onto next row
			row++;
			col = 0;
			break;
		case 0: // Last point of this timestep
			numPoints[timestep] = mtrxPointIndex+1;
			
			// Reset counts
			row = 0;
			col = 0;
			mtrxPointIndex = 0;
			// Next timestep
			timestep++;
			break;
		case '0': // LED point - off
			col++;
			break;
		default: // Unrecognised - ignore
			break;
	}
}

/* --------------- Initialise --------------- */
void setupLEDs()
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
}

void setupTimers()
{
	// Timers for opacity PWM
	SET_BIT(TCCR0B, CS02);
	// Waveform - Fast PWM
	SET_BIT(TCCR0A, WGM00);
	SET_BIT(TCCR0A, WGM01);
	// Compare interrupt enable
	SET_BIT(TIMSK0, OCIE0A);
	
	OCR0A = 255; // Default
	
	SET_BIT(TCCR1B, CS12);
	SET_BIT(TIMSK1, TOIE1);
}
ISR(TIMER1_OVF_vect)
{
	patternTime++;
	if (patternTime == maxTimestep)
	{
		patternTime = 0;
	}
}

void setupUART()
{
    UBRR0 = F_CPU / 16 / BAUD - 1;
	
    UCSR0A = 0;
	
	// Enable interrupts
	uint8_t mask = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
    SET_BITS(UCSR0B, mask);
	
	// Character size
	mask = (1 << UCSZ00) | (1 << UCSZ01) | (1 << UCSZ02);
    SET_BITS(UCSR0C, mask);
}

/* ------ Main ------ */
int main() {
	setupLEDs();
	setupTimers();
	setupUART();
	
	sei();
	
	while (1) {
		// Quickly flash matrix of LEDs to simulate matrix LED display
		turnOnLEDs();
		
		// Update compare value to change opacity
		OCRAUpdate();
	}
}