#include <stdint.h>
#include <stdio.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

void uartTransmit();
void uartTransmitByte(unsigned char byte);
void uartProcess();
void prepareMessage();
void uartSetup();
void timerSetup();

// Bit operations
#define SET_BIT(reg, pin)			(reg) |= (1 << (pin))
#define SET_BITS(reg, mask)			(reg) |= (mask)
#define CLEAR_BIT(reg, pin)			(reg) &= ~(1 << (pin))
#define CLEAR_BITS(reg, mask)		(reg) &= ~(mask)
#define BIT_VALUE(reg, pin)			(((reg) >> (pin)) & 1)
#define BIT_IS_SET(reg, pin)		(BIT_VALUE((reg),(pin))==1)

// Device specs
#define BAUD	9600
// LED Matrix display limits
#define MAX_REFRESH_RATE			1
#define MIN_REFRESH_RATE			3
#define MAX_MTRX_PATTERN_STEPS		20+1+1 // +1 for header, +1 for NULL
#define SEC_TO_OCR1A(sec)			F_CPU/1024*sec // OCR = frequency / prescalar * target_time

static char * messagesToSend[MAX_MTRX_PATTERN_STEPS];
static int uartStringIndex = 0;
volatile uint8_t byteToTransmit = 0;

//Matrix array patterns to display
/*
	Dimensions: pattern number x timestep
	For each pattern, contains header, then a array of strings,
	ending with NULL. Cells of each row are represented by bit values
	
	Header: Name/mode (appended later from reading potentiometer input)
*/
char* mtrxPatterns[][MAX_MTRX_PATTERN_STEPS] = {
	{// Pattern 1
		"Border Snake",
		
		"100000,"
		"000000,"
		"000001",
		
		"000000,"
		"100001,"
		"000000",
		
		"000001,"
		"000000,"
		"100000",
		
		"000010,"
		"000000,"
		"010000",
		
		"000100,"
		"000000,"
		"001000",
		
		"001000,"
		"000000,"
		"000100",
		
		"010000,"
		"000000,"
		"000010",
		
		NULL
	},
	{// Pattern 2
		"Cross",
		
		"000100,"
		"100010,"
		"010001",
		
		"100010,"
		"010001,"
		"001000",
		
		"010001,"
		"001000,"
		"000100",
		
		"001000,"
		"000100,"
		"100010",
		
		NULL
	},
	{// Pattern 3
		"Wipe",
		
		"111111,"
		"000000,"
		"000000",
		
		"000000,"
		"111111,"
		"000000",
		
		"000000,"
		"000000,"
		"111111",
		
		"000000,"
		"000000,"
		"000000",
		
		NULL
	},
	{// Pattern 4
		"Wipe Horizontal",
		
		"100000,"
		"100000,"
		"100000",
		
		"010000,"
		"010000,"
		"010000",
		
		"001000,"
		"001000,"
		"001000",
		
		"000100,"
		"000100,"
		"000100",
		
		"000010,"
		"000010,"
		"000010",
		
		"000001,"
		"000001,"
		"000001",
		
		NULL
	}
};
int numMtrxPatterns = sizeof(mtrxPatterns)/sizeof(mtrxPatterns[0]);

/* ------ Transmitter ------ */
// Process (loop)
void uartProcess()
{
	static int messageIndex = 0;
	static uint8_t uartCooldown = 0;
	static int transmitComplete = 0;
	
	// Cycle
	if (uartCooldown) return;
		// Stop if transmission is fully completed (including for the string)
	if (transmitComplete && uartStringIndex == 0) return;
	
	// String to write to Transmit Buffer
	static char* uartString;
	
	// Check previous string is not still transmitting
	if (uartStringIndex == 0)
	{

		// Transmit next timestep/string in matrix display
		uartString = messagesToSend[messageIndex];
		
		if (messagesToSend[messageIndex] == NULL)
		{
			// End of pattern - Reset back to start of array
			messageIndex = 0;
			transmitComplete = 1;
			uartString[0] = 4; // End of transmission character
			uartString[1] = 0; // End of string
		}
		else
		{
			// Get ready to transmit next string
			messageIndex++;
		}
		
	}
	
	// Continue transmission
	uartTransmit(uartString);
}

void uartTransmit(char* dataToTransmit)
{
    if (BIT_IS_SET(UCSR0A, UDRE0)) { // Check if buffer space is free
		if (dataToTransmit[uartStringIndex] != 0) // If not end of string
		{
			// Transmit next byte
			uartTransmitByte((uint8_t)(dataToTransmit[uartStringIndex]));
			uartStringIndex++;
		}
		else
		{
			// String transmission complete
			uartTransmitByte(0);
			uartStringIndex = 0; // -> Reset to start of string
		}
    }
}

// Prepare byte to transmit
void uartTransmitByte(uint8_t byte)
{
	// Prepare to transmit
	byteToTransmit = byte;
	
	// Wait for space in data registry (USART_UDRE_vect)
	SET_BIT(UCSR0B, UDRIE0);
}

// Transmit when USART data registry empty
ISR(USART_UDRE_vect)
{
    // Send data to transmit buffer (when free)
	UDR0 = byteToTransmit;
	CLEAR_BIT(UCSR0B, UDRIE0); // Transmitted -> clear interrupt trigger so it does not loop
}

/* ------ Inputs ------ */
void prepareMessage(int patternNo)
{
	// Get and set message to transmit
	int timestep = 0;
	char* stringAtTime;
		// Loop through selected pattern array and
		// put each timestep in messagesToSend
	do
	{
		// Skip first string in pattern: Pattern name
		// mtrxPatterns offset by 1
		stringAtTime = mtrxPatterns[patternNo][timestep+1];
		messagesToSend[timestep] = stringAtTime;
		timestep++;
	} while (stringAtTime != NULL);
	
	// End with EOT (end of transmission)
	messagesToSend[timestep] = NULL;
}

/* ------ Initialise ------ */
void uartSetup()
{
    UBRR0 = F_CPU / 16 / BAUD - 1;
    UCSR0A = 0;
	
	// Interrupts
    SET_BIT(UCSR0B, TXEN0);
	
	// Character size
	uint8_t mask = (1 << UCSZ00) | (1 << UCSZ01) | (1 << UCSZ02);
    SET_BITS(UCSR0C, mask);
}

/* ------ Main ------ */
int main() {
    uartSetup();
	//timerSetup();
	sei();
	
	// Select pattern 
	int patternNo = 3;
	
	prepareMessage(patternNo);
	
	
    while (1)
	{
		uartProcess();
		
		_delay_ms(10);
    }

    return 0;
}
