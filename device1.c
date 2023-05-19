// LCD LIBRARY FROM WEEK 11 NOTES


#include <string.h>
#include <inttypes.h>

//For more information about this library please visit:
//http://www.arduino.cc/en/Reference/LiquidCrystal

//and

//https://github.com/arduino-libraries/LiquidCrystal


// --== WIRING ==--
// LCD GND  -> GND
// LCD VCC  -> 5V
// LCD V0   -> GND
// LCD RW   -> GND
// LCD LED Anode    -> 220 Ohm -> 5V
// LCD LED Cathode  -> GND

//Change the values in these defines to reflect 
//  how you've hooked up the screen
//In 4-pin mode only DATA4:7 are used

#define LCD_USING_4PIN_MODE (1)

#define LCD_DATA4_DDR (DDRD)
#define LCD_DATA5_DDR (DDRD)
#define LCD_DATA6_DDR (DDRD)
#define LCD_DATA7_DDR (DDRD)

#define LCD_DATA4_PORT (PORTD)
#define LCD_DATA5_PORT (PORTD)
#define LCD_DATA6_PORT (PORTD)
#define LCD_DATA7_PORT (PORTD)

#define LCD_DATA4_PIN (2)
#define LCD_DATA5_PIN (3)
#define LCD_DATA6_PIN (4)
#define LCD_DATA7_PIN (5)


#define LCD_RS_DDR (DDRD)
#define LCD_ENABLE_DDR (DDRD)

#define LCD_RS_PORT (PORTD)
#define LCD_ENABLE_PORT (PORTD)

#define LCD_RS_PIN (7)
#define LCD_ENABLE_PIN (6)



//DATASHEET: https://s3-us-west-1.amazonaws.com/123d-circuits-datasheets/uploads%2F1431564901240-mni4g6oo875bfbt9-6492779e35179defaf4482c7ac4f9915%2FLCD-WH1602B-TMI.pdf

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

void lcd_init(void);
void lcd_write_string(uint8_t x, uint8_t y, char string[]);
void lcd_write_char(uint8_t x, uint8_t y, char val);


void lcd_clear(void);
void lcd_home(void);

void lcd_createChar(uint8_t, uint8_t[]);
void lcd_setCursor(uint8_t, uint8_t); 

void lcd_noDisplay(void);
void lcd_display(void);
void lcd_noBlink(void);
void lcd_blink(void);
void lcd_noCursor(void);
void lcd_cursor(void);
void lcd_leftToRight(void);
void lcd_rightToLeft(void);
void lcd_autoscroll(void);
void lcd_noAutoscroll(void);
void scrollDisplayLeft(void);
void scrollDisplayRight(void);

size_t lcd_write(uint8_t);
void lcd_command(uint8_t);


void lcd_send(uint8_t, uint8_t);
void lcd_write4bits(uint8_t);
void lcd_write8bits(uint8_t);
void lcd_pulseEnable(void);

uint8_t _lcd_displayfunction;
uint8_t _lcd_displaycontrol;
uint8_t _lcd_displaymode;

/* ----------------- END OF LCD DEFINITIONS -------------- */



#include <stdint.h>
#include <stdio.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>

void uartTransmit();
void uartTransmitByte(unsigned char byte);
void uartProcess();
void buttonProcess();
void prepareMessage();
void uartSetup();
void timerSetup();
void inputSetup();
void lcdSetup();

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

// Global variables
// UART transmitting
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
	},
	{// Pattern 5
		"Arrow",
		
		"110001,"
		"011000,"
		"110001",
		
		"011000,"
		"001100,"
		"011000",
		
		"001100,"
		"000110,"
		"001100",
		
		"000110,"
		"100011,"
		"000110",
		
		"100011,"
		"110001,"
		"100011",
		
		NULL
	}
};
int numMtrxPatterns = sizeof(mtrxPatterns)/sizeof(mtrxPatterns[0]);

// Inputs variables initialise
// Check if need to transmit
volatile int startTransmit = 0;
// Check if debounced switch is pressed
volatile uint8_t switch_closed = 0;
// Selected pattern
volatile uint16_t patternSelect = 0;

/* --------------- Transmitter --------------- */
// Process (looped)
void uartProcess()
{
	static int messageIndex = 0;
	static uint8_t uartCooldown = 0;
	
	// Cycle
	if (uartCooldown) return;
		// Stop if transmission is fully completed (including for the string)
	if (!startTransmit && uartStringIndex == 0) return;
	
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
			startTransmit = 0;
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

/* --------------- Inputs --------------- */
void buttonProcess()
{
	// Prepare
	prepareMessage(patternSelect);
	// And then start transmitting
	startTransmit = 1;
}

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

ISR(ADC_vect)
{
	// Inputs to change settings
	static uint16_t previousSelect = 0;
	
	// ADC Conversion Result (10bit)
	// Ranging from 1 to num of patterns
	patternSelect = ADC / (1023.0/numMtrxPatterns);
	
	if (patternSelect != previousSelect)
	{
		// Pattern change -> Update LCD with pattern name (first string)
		lcd_clear();
		lcd_write_string(0, 0, mtrxPatterns[patternSelect][0]);
		// Save to history so that it does not clear every time
		previousSelect = patternSelect;
	}
	
}

ISR(TIMER0_OVF_vect)
{
	// ADC Start Conversion (or reset)
	SET_BIT(ADCSRA, ADSC);
	
	// And switch debouncing
	/* Code gotten from AMS CAB202 Topic 9, Exercise 3 */
	static uint8_t state_count = 0;
	
	state_count = state_count << 1;
	
	uint8_t mask = 7; // Threshold @ bit 3 (00000111)
	state_count &= mask;
	
	state_count |= BIT_IS_SET(PINC, 1);
	
	if (state_count == mask) { // Debounced on (00000111)
		switch_closed = 1;
	}
	else if (state_count == 0) { // Debounced off (00000000)
		switch_closed = 0;
	}
}

/* --------------- Initialise --------------- */
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

void timerSetup()
{
	// Timer setup
	SET_BIT(TCCR0B, CS00);
	SET_BIT(TCCR0B, CS01);
	
	SET_BIT(TIMSK0, TOIE0); // Timer/Counter0 Overflow Interrupt Enable
}

// Setup timer and enable interrupt
void inputSetup()
{
	// ADC
	SET_BIT(ADMUX, REFS0); // Voltage Reference Selections for ADC (arduino internal ground)
	SET_BIT(ADCSRA, ADEN); // ADEN: ADC Enable
	SET_BIT(ADCSRA, ADIE); // ADC Interrupt Enable
	SET_BIT(ADCSRA, ADSC); // ADC Start Conversion
	
	// Setup switch
	SET_BIT(DDRC, 1);
}


/* --------------- Main --------------- */
int main() {
    uartSetup();
	timerSetup();
	inputSetup();
	lcd_init(); // LCD setup from library (lecture notes)
	
	sei();
	
    while (1)
	{
		uartProcess();
		
		// Debounced button pressed -> start transmitting
		if (switch_closed && !startTransmit)
		{
			// Button event
			buttonProcess();
		}
		
		_delay_ms(10);
    }

    return 0;
}



/* ********************************************/
// START LIBRARY FUNCTIONS - FROM WEEK 11 NOTES

void lcd_init(void){
  //dotsize
  if (LCD_USING_4PIN_MODE){
    _lcd_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
  } else {
    _lcd_displayfunction = LCD_8BITMODE | LCD_1LINE | LCD_5x8DOTS;
  }
  
  _lcd_displayfunction |= LCD_2LINE;

  // RS Pin
  LCD_RS_DDR |= (1 << LCD_RS_PIN);
  // Enable Pin
  LCD_ENABLE_DDR |= (1 << LCD_ENABLE_PIN);
  
  #if LCD_USING_4PIN_MODE
    //Set DDR for all the data pins
    LCD_DATA4_DDR |= (1 << LCD_DATA4_PIN);
    LCD_DATA5_DDR |= (1 << LCD_DATA5_PIN);
    LCD_DATA6_DDR |= (1 << LCD_DATA6_PIN);    
    LCD_DATA7_DDR |= (1 << LCD_DATA7_PIN);


#else
    //Set DDR for all the data pins
    LCD_DATA0_DDR |= (1 << LCD_DATA0_PIN);
    LCD_DATA1_DDR |= (1 << LCD_DATA1_PIN);
    LCD_DATA2_DDR |= (1 << LCD_DATA2_PIN);
    LCD_DATA3_DDR |= (1 << LCD_DATA3_PIN);
    LCD_DATA4_DDR |= (1 << LCD_DATA4_PIN);
    LCD_DATA5_DDR |= (1 << LCD_DATA5_PIN);
    LCD_DATA6_DDR |= (1 << LCD_DATA6_PIN);
    LCD_DATA7_DDR |= (1 << LCD_DATA7_PIN);
  #endif 

  // SEE PAGE 45/46 OF Hitachi HD44780 DATASHEET FOR INITIALIZATION SPECIFICATION!

  // according to datasheet, we need at least 40ms after power rises above 2.7V
  // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
  _delay_us(50000); 
  // Now we pull both RS and Enable low to begin commands (R/W is wired to ground)
  LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
  LCD_ENABLE_PORT &= ~(1 << LCD_ENABLE_PIN);
  
  //put the LCD into 4 bit or 8 bit mode
  if (LCD_USING_4PIN_MODE) {
    // this is according to the hitachi HD44780 datasheet
    // figure 24, pg 46

    // we start in 8bit mode, try to set 4 bit mode
    lcd_write4bits(0b0111);
    _delay_us(4500); // wait min 4.1ms

    // second try
    lcd_write4bits(0b0111);
    _delay_us(4500); // wait min 4.1ms
    
    // third go!
    lcd_write4bits(0b0111); 
    _delay_us(150);

    // finally, set to 4-bit interface
    lcd_write4bits(0b0010); 
  } else {
    // this is according to the hitachi HD44780 datasheet
    // page 45 figure 23

    // Send function set command sequence
    lcd_command(LCD_FUNCTIONSET | _lcd_displayfunction);
    _delay_us(4500);  // wait more than 4.1ms

    // second try
    lcd_command(LCD_FUNCTIONSET | _lcd_displayfunction);
    _delay_us(150);

    // third go
    lcd_command(LCD_FUNCTIONSET | _lcd_displayfunction);
  }

  // finally, set # lines, font size, etc.
  lcd_command(LCD_FUNCTIONSET | _lcd_displayfunction);  

  // turn the display on with no cursor or blinking default
  _lcd_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;  
  lcd_display();

  // clear it off
  lcd_clear();

  // Initialize to default text direction (for romance languages)
  _lcd_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
  // set the entry mode
  lcd_command(LCD_ENTRYMODESET | _lcd_displaymode);
}


/********** high level commands, for the user! */
void lcd_write_string(uint8_t x, uint8_t y, char string[]){
  lcd_setCursor(x,y);
  for(int i=0; string[i]!='\0'; ++i){
    lcd_write(string[i]);
  }
}

void lcd_write_char(uint8_t x, uint8_t y, char val){
  lcd_setCursor(x,y);
  lcd_write(val);
}

void lcd_clear(void){
  lcd_command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  _delay_us(2000);  // this command takes a long time!
}

void lcd_home(void){
  lcd_command(LCD_RETURNHOME);  // set cursor position to zero
  _delay_us(2000);  // this command takes a long time!
}


// Allows us to fill the first 8 CGRAM locations
// with custom characters
void lcd_createChar(uint8_t location, uint8_t charmap[]) {
  location &= 0x7; // we only have 8 locations 0-7
  lcd_command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++) {
    lcd_write(charmap[i]);
  }
}


void lcd_setCursor(uint8_t col, uint8_t row){
  if ( row >= 2 ) {
    row = 1;
  }
  
  lcd_command(LCD_SETDDRAMADDR | (col + row*0x40));
}

// Turn the display on/off (quickly)
void lcd_noDisplay(void) {
  _lcd_displaycontrol &= ~LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _lcd_displaycontrol);
}
void lcd_display(void) {
  _lcd_displaycontrol |= LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | _lcd_displaycontrol);
}

// Turns the underline cursor on/off
void lcd_noCursor(void) {
  _lcd_displaycontrol &= ~LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _lcd_displaycontrol);
}
void lcd_cursor(void) {
  _lcd_displaycontrol |= LCD_CURSORON;
  lcd_command(LCD_DISPLAYCONTROL | _lcd_displaycontrol);
}

// Turn on and off the blinking cursor
void lcd_noBlink(void) {
  _lcd_displaycontrol &= ~LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _lcd_displaycontrol);
}
void lcd_blink(void) {
  _lcd_displaycontrol |= LCD_BLINKON;
  lcd_command(LCD_DISPLAYCONTROL | _lcd_displaycontrol);
}

// These commands scroll the display without changing the RAM
void scrollDisplayLeft(void) {
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void scrollDisplayRight(void) {
  lcd_command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void lcd_leftToRight(void) {
  _lcd_displaymode |= LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _lcd_displaymode);
}

// This is for text that flows Right to Left
void lcd_rightToLeft(void) {
  _lcd_displaymode &= ~LCD_ENTRYLEFT;
  lcd_command(LCD_ENTRYMODESET | _lcd_displaymode);
}

// This will 'right justify' text from the cursor
void lcd_autoscroll(void) {
  _lcd_displaymode |= LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _lcd_displaymode);
}

// This will 'left justify' text from the cursor
void lcd_noAutoscroll(void) {
  _lcd_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  lcd_command(LCD_ENTRYMODESET | _lcd_displaymode);
}

/*********** mid level commands, for sending data/cmds */

inline void lcd_command(uint8_t value) {
  //
  lcd_send(value, 0);
}

inline size_t lcd_write(uint8_t value) {
  lcd_send(value, 1);
  return 1; // assume sucess
}

/************ low level data pushing commands **********/

// write either command or data, with automatic 4/8-bit selection
void lcd_send(uint8_t value, uint8_t mode) {
  //RS Pin
  LCD_RS_PORT &= ~(1 << LCD_RS_PIN);
  LCD_RS_PORT |= (!!mode << LCD_RS_PIN);

  if (LCD_USING_4PIN_MODE) {
    lcd_write4bits(value>>4);
    lcd_write4bits(value);
  } else {
    lcd_write8bits(value); 
  } 
}

void lcd_pulseEnable(void) {
  //Enable Pin
  LCD_ENABLE_PORT &= ~(1 << LCD_ENABLE_PIN);
  _delay_us(1);    
  LCD_ENABLE_PORT |= (1 << LCD_ENABLE_PIN);
  _delay_us(1);    // enable pulse must be >450ns
  LCD_ENABLE_PORT &= ~(1 << LCD_ENABLE_PIN);
  _delay_us(100);   // commands need > 37us to settle
}

void lcd_write4bits(uint8_t value) {
  //Set each wire one at a time

  LCD_DATA4_PORT &= ~(1 << LCD_DATA4_PIN);
  LCD_DATA4_PORT |= ((value & 1) << LCD_DATA4_PIN);
  value >>= 1;

  LCD_DATA5_PORT &= ~(1 << LCD_DATA5_PIN);
  LCD_DATA5_PORT |= ((value & 1) << LCD_DATA5_PIN);
  value >>= 1;

  LCD_DATA6_PORT &= ~(1 << LCD_DATA6_PIN);
  LCD_DATA6_PORT |= ((value & 1) << LCD_DATA6_PIN);
  value >>= 1;

  LCD_DATA7_PORT &= ~(1 << LCD_DATA7_PIN);
  LCD_DATA7_PORT |= ((value & 1) << LCD_DATA7_PIN);

  lcd_pulseEnable();
}

void lcd_write8bits(uint8_t value) {
  //Set each wire one at a time

  #if !LCD_USING_4PIN_MODE
    LCD_DATA0_PORT &= ~(1 << LCD_DATA0_PIN);
    LCD_DATA0_PORT |= ((value & 1) << LCD_DATA0_PIN);
    value >>= 1;

    LCD_DATA1_PORT &= ~(1 << LCD_DATA1_PIN);
    LCD_DATA1_PORT |= ((value & 1) << LCD_DATA1_PIN);
    value >>= 1;

    LCD_DATA2_PORT &= ~(1 << LCD_DATA2_PIN);
    LCD_DATA2_PORT |= ((value & 1) << LCD_DATA2_PIN);
    value >>= 1;

    LCD_DATA3_PORT &= ~(1 << LCD_DATA3_PIN);
    LCD_DATA3_PORT |= ((value & 1) << LCD_DATA3_PIN);
    value >>= 1;

    LCD_DATA4_PORT &= ~(1 << LCD_DATA4_PIN);
    LCD_DATA4_PORT |= ((value & 1) << LCD_DATA4_PIN);
    value >>= 1;

    LCD_DATA5_PORT &= ~(1 << LCD_DATA5_PIN);
    LCD_DATA5_PORT |= ((value & 1) << LCD_DATA5_PIN);
    value >>= 1;

    LCD_DATA6_PORT &= ~(1 << LCD_DATA6_PIN);
    LCD_DATA6_PORT |= ((value & 1) << LCD_DATA6_PIN);
    value >>= 1;

    LCD_DATA7_PORT &= ~(1 << LCD_DATA7_PIN);
    LCD_DATA7_PORT |= ((value & 1) << LCD_DATA7_PIN);
    
    lcd_pulseEnable();
  #endif
}