#include <stdint.h>
#include "system_stm32f4xx.h"
#include <string.h>

//Register Base Addresses 
#define RCC_BASE        0x40023800
#define GPIOB_BASE      0x40020400
#define I2C1_BASE       0x40005400
#define TIM2_BASE       0x40000000
#define NVIC_ISER0      (*((volatile unsigned int *)0xE000E100))

//RCC Registers
#define RCC_AHB1ENR     (*((volatile unsigned int *)(RCC_BASE + 0x30)))
#define RCC_APB1ENR     (*((volatile unsigned int *)(RCC_BASE + 0x40)))

//GPIOB Registers
#define GPIOB_MODER     (*((volatile unsigned int *)(GPIOB_BASE + 0x00)))
#define GPIOB_OTYPER    (*((volatile unsigned int *)(GPIOB_BASE + 0x04)))
#define GPIOB_OSPEEDR   (*((volatile unsigned int *)(GPIOB_BASE + 0x08)))
#define GPIOB_PUPDR     (*((volatile unsigned int *)(GPIOB_BASE + 0x0C)))
#define GPIOB_AFRH      (*((volatile unsigned int *)(GPIOB_BASE + 0x24)))

//I2C1 Registers
#define I2C1_CR1        (*((volatile unsigned int *)(I2C1_BASE + 0x00)))
#define I2C1_CR2        (*((volatile unsigned int *)(I2C1_BASE + 0x04)))
#define I2C1_DR         (*((volatile unsigned int *)(I2C1_BASE + 0x10)))
#define I2C1_SR1        (*((volatile unsigned int *)(I2C1_BASE + 0x14)))
#define I2C1_SR2        (*((volatile unsigned int *)(I2C1_BASE + 0x18)))
#define I2C1_CCR        (*((volatile unsigned int *)(I2C1_BASE + 0x1C)))
#define I2C1_TRISE      (*((volatile unsigned int *)(I2C1_BASE + 0x20)))

//TIM2 Registers
#define TIM2_CR1        (*((volatile unsigned int *)(TIM2_BASE + 0x00)))
#define TIM2_DIER       (*((volatile unsigned int *)(TIM2_BASE + 0x0C)))
#define TIM2_SR         (*((volatile unsigned int *)(TIM2_BASE + 0x10)))
#define TIM2_CNT        (*((volatile unsigned int *)(TIM2_BASE + 0x24)))
#define TIM2_PSC        (*((volatile unsigned int *)(TIM2_BASE + 0x28)))
#define TIM2_ARR        (*((volatile unsigned int *)(TIM2_BASE + 0x2C)))

//OLED Constants
#define OLED_ADDR_W     0x78    //(0x3C << 1) 

//Time Variables 
volatile unsigned int hours   = 12;  //Initial time: 12:00:00
volatile unsigned int minutes = 0;
volatile unsigned int seconds = 0;
volatile unsigned int tick_flag = 0;  //Set by TIM2 ISR every second
volatile unsigned int colon_on = 1;   //Blinking colon toggle 

/* Display Buffer: 128 x 4 pages = 512 bytes */
unsigned char displayBuffer[512];

/*5x7 Font Table (ASCII 32 to 127, 5 bytes per character)*/
static const unsigned char font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, // 32: space
    {0x00,0x00,0x5F,0x00,0x00}, // 33: !
    {0x00,0x07,0x00,0x07,0x00}, // 34: "
    {0x14,0x7F,0x14,0x7F,0x14}, // 35: #
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36: $
    {0x23,0x13,0x08,0x64,0x62}, // 37: %
    {0x36,0x49,0x55,0x22,0x50}, // 38: &
    {0x00,0x05,0x03,0x00,0x00}, // 39: '
    {0x00,0x1C,0x22,0x41,0x00}, // 40: (
    {0x00,0x41,0x22,0x1C,0x00}, // 41: )
    {0x14,0x08,0x3E,0x08,0x14}, // 42: *
    {0x08,0x08,0x3E,0x08,0x08}, // 43: +
    {0x00,0x50,0x30,0x00,0x00}, // 44: ,
    {0x08,0x08,0x08,0x08,0x08}, // 45: -
    {0x00,0x60,0x60,0x00,0x00}, // 46: .
    {0x20,0x10,0x08,0x04,0x02}, // 47: /
    {0x3E,0x51,0x49,0x45,0x3E}, // 48: 0
    {0x00,0x42,0x7F,0x40,0x00}, // 49: 1
    {0x42,0x61,0x51,0x49,0x46}, // 50: 2
    {0x21,0x41,0x45,0x4B,0x31}, // 51: 3
    {0x18,0x14,0x12,0x7F,0x10}, // 52: 4
    {0x27,0x45,0x45,0x45,0x39}, // 53: 5
    {0x3C,0x4A,0x49,0x49,0x30}, // 54: 6
    {0x01,0x71,0x09,0x05,0x03}, // 55: 7
    {0x36,0x49,0x49,0x49,0x36}, // 56: 8
    {0x06,0x49,0x49,0x29,0x1E}, // 57: 9
    {0x00,0x36,0x36,0x00,0x00}, // 58: :
    {0x00,0x56,0x36,0x00,0x00}, // 59: ;
    {0x08,0x14,0x22,0x41,0x00}, // 60: 
    {0x14,0x14,0x14,0x14,0x14}, // 61: =
    {0x00,0x41,0x22,0x14,0x08}, // 62: >
    {0x02,0x01,0x51,0x09,0x06}, // 63: ?
    {0x32,0x49,0x79,0x41,0x3E}, // 64: @
    {0x7E,0x11,0x11,0x11,0x7E}, // 65: A
    {0x7F,0x49,0x49,0x49,0x36}, // 66: B
    {0x3E,0x41,0x41,0x41,0x22}, // 67: C
    {0x7F,0x41,0x41,0x22,0x1C}, // 68: D
    {0x7F,0x49,0x49,0x49,0x41}, // 69: E
    {0x7F,0x09,0x09,0x09,0x01}, // 70: F
    {0x3E,0x41,0x49,0x49,0x7A}, // 71: G
    {0x7F,0x08,0x08,0x08,0x7F}, // 72: H
    {0x00,0x41,0x7F,0x41,0x00}, // 73: I
    {0x20,0x40,0x41,0x3F,0x01}, // 74: J
    {0x7F,0x08,0x14,0x22,0x41}, // 75: K
    {0x7F,0x40,0x40,0x40,0x40}, // 76: L
    {0x7F,0x02,0x0C,0x02,0x7F}, // 77: M
    {0x7F,0x04,0x08,0x10,0x7F}, // 78: N
    {0x3E,0x41,0x41,0x41,0x3E}, // 79: O
    {0x7F,0x09,0x09,0x09,0x06}, // 80: P
    {0x3E,0x41,0x51,0x21,0x5E}, // 81: Q
    {0x7F,0x09,0x19,0x29,0x46}, // 82: R
    {0x46,0x49,0x49,0x49,0x31}, // 83: S
    {0x01,0x01,0x7F,0x01,0x01}, // 84: T
    {0x3F,0x40,0x40,0x40,0x3F}, // 85: U
    {0x1F,0x20,0x40,0x20,0x1F}, // 86: V
    {0x3F,0x40,0x38,0x40,0x3F}, // 87: W
    {0x63,0x14,0x08,0x14,0x63}, // 88: X
    {0x07,0x08,0x70,0x08,0x07}, // 89: Y
    {0x61,0x51,0x49,0x45,0x43}, // 90: Z
    {0x00,0x7F,0x41,0x41,0x00}, // 91: [
    {0x02,0x04,0x08,0x10,0x20}, // 92: \
    {0x00,0x41,0x41,0x7F,0x00}, // 93: ]
    {0x04,0x02,0x01,0x02,0x04}, // 94: ^
    {0x40,0x40,0x40,0x40,0x40}, // 95: _
    {0x00,0x01,0x02,0x04,0x00}, // 96: `
    {0x20,0x54,0x54,0x54,0x78}, // 97: a
    {0x7F,0x48,0x44,0x44,0x38}, // 98: b
    {0x38,0x44,0x44,0x44,0x20}, // 99: c
    {0x38,0x44,0x44,0x48,0x7F}, // 100: d
    {0x38,0x54,0x54,0x54,0x18}, // 101: e
    {0x08,0x7E,0x09,0x01,0x02}, // 102: f
    {0x0C,0x52,0x52,0x52,0x3E}, // 103: g
    {0x7F,0x08,0x04,0x04,0x78}, // 104: h
    {0x00,0x44,0x7D,0x40,0x00}, // 105: i
    {0x20,0x40,0x44,0x3D,0x00}, // 106: j
    {0x7F,0x10,0x28,0x44,0x00}, // 107: k
    {0x00,0x41,0x7F,0x40,0x00}, // 108: l
    {0x7C,0x04,0x78,0x04,0x78}, // 109: m
    {0x7C,0x08,0x04,0x04,0x78}, // 110: n
    {0x38,0x44,0x44,0x44,0x38}, // 111: o
    {0x7C,0x14,0x14,0x14,0x08}, // 112: p
    {0x08,0x14,0x14,0x18,0x7C}, // 113: q
    {0x7C,0x08,0x04,0x04,0x08}, // 114: r
    {0x48,0x54,0x54,0x54,0x20}, // 115: s
    {0x04,0x3F,0x44,0x40,0x20}, // 116: t
    {0x3C,0x40,0x40,0x20,0x7C}, // 117: u
    {0x1C,0x20,0x40,0x20,0x1C}, // 118: v
    {0x3C,0x40,0x30,0x40,0x3C}, // 119: w
    {0x44,0x28,0x10,0x28,0x44}, // 120: x
    {0x0C,0x50,0x50,0x50,0x3C}, // 121: y
    {0x44,0x64,0x54,0x4C,0x44}, // 122: z
    {0x00,0x08,0x36,0x41,0x00}, // 123: {
    {0x00,0x00,0x7F,0x00,0x00}, // 124: |
    {0x00,0x41,0x36,0x08,0x00}, // 125: }
    {0x10,0x08,0x08,0x10,0x08}, // 126: ~
    {0x00,0x00,0x00,0x00,0x00}  // 127: DEL
};

//Simple delay
 
void delay(volatile unsigned int count) {
    while (count--);
}

//I2C1 Functions
 
void I2C1_GPIO_Init(void) {
    //Enable GPIOB clock 
    RCC_AHB1ENR |= 0x02;
    
    /* PB8, PB9 = Alternate Function (10) */
    GPIOB_MODER &= ~0x000F0000;
    GPIOB_MODER |=  0x000A0000;
    
    //Open-drain
    GPIOB_OTYPER |= 0x0300;
    
    //High speed
    GPIOB_OSPEEDR |= 0x000F0000;
    
    //Pull-up
    GPIOB_PUPDR &= ~0x000F0000;
    GPIOB_PUPDR |=  0x00050000;
    
    //AF4 for PB8 and PB9
    GPIOB_AFRH &= ~0xFF;
    GPIOB_AFRH |=  0x44;
}

void I2C1_Init(void) {
    //Enable I2C1 clock
    RCC_APB1ENR |= 0x00200000;
    
    //Disable I2C1
    I2C1_CR1 = 0;
    
    //Software reset
    I2C1_CR1 = 0x8000;
    I2C1_CR1 = 0;
    
    //APB1 clock = 16 MHz 
    I2C1_CR2 = 16;
    
    //100 kHz standard mode: CCR = 16MHz / (2*100kHz) = 80 
    I2C1_CCR = 0x50;
    
    /* TRISE = 16 + 1 = 17 */
    I2C1_TRISE = 17;
    
    //Enable I2C1
    I2C1_CR1 = 0x01;
}

void I2C_Start(void) {
    I2C1_CR1 |= 0x0100;        //Generate START 
    while (!(I2C1_SR1 & 0x01)); //Wait for SB 
}

void I2C_SendAddr(unsigned char addr) {
    I2C1_DR = addr;
    while (!(I2C1_SR1 & 0x02)); //Wait for ADDR 
    (void)I2C1_SR1;             //Clear ADDR by reading SR1+SR2
    (void)I2C1_SR2;
}

void I2C_SendByte(unsigned char data) {
    while (!(I2C1_SR1 & 0x80)); //Wait for TXE 
    I2C1_DR = data;
    while (!(I2C1_SR1 & 0x04)); //Wait for BTF
}

void I2C_Stop(void) {
    I2C1_CR1 |= 0x0200;        //Generate STOP
    delay(100);
}

//OLED (SSD1306) Functions
 
void OLED_SendCommand(unsigned char cmd) {
    I2C_Start();
    I2C_SendAddr(OLED_ADDR_W);
    I2C_SendByte(0x00);         //Control byte: command
    I2C_SendByte(cmd);
    I2C_Stop();
}

void OLED_Init(void) {
    delay(800000);  //Power-up delay
    
    OLED_SendCommand(0xAE);     //Display OFF 
    OLED_SendCommand(0xD5);     //Clock divide
    OLED_SendCommand(0x80);
    OLED_SendCommand(0xA8);     //Multiplex ratio
    OLED_SendCommand(0x1F);     //32 rows 
    OLED_SendCommand(0xD3);     //Display offset 
    OLED_SendCommand(0x00);
    OLED_SendCommand(0x40);     //Start line 
    OLED_SendCommand(0x8D);     //Charge pump 
    OLED_SendCommand(0x14);
    OLED_SendCommand(0x20);     //Memory mode
    OLED_SendCommand(0x00);     //Horizontal 
    OLED_SendCommand(0xA1);     //Segment remap 
    OLED_SendCommand(0xC8);     //COM scan dir 
    OLED_SendCommand(0xDA);     //COM pins 
    OLED_SendCommand(0x02);
    OLED_SendCommand(0x81);     //Contrast 
    OLED_SendCommand(0x8F);
    OLED_SendCommand(0xD9);     //Pre-charge 
    OLED_SendCommand(0xF1);
    OLED_SendCommand(0xDB);     //VCOMH 
    OLED_SendCommand(0x40);
    OLED_SendCommand(0xA4);     //Display from RAM
    OLED_SendCommand(0xA6);     //Normal display
    OLED_SendCommand(0xAF);     //Display ON 
    
    delay(200000);
}

void OLED_ClearBuffer(void) {
    memset(displayBuffer, 0, 512);
}

void OLED_ClearScreen(void) {
    unsigned int page, col;
    OLED_ClearBuffer();
    for (page = 0; page < 4; page++) {
        OLED_SendCommand(0xB0 + page);
        OLED_SendCommand(0x00);
        OLED_SendCommand(0x10);
        I2C_Start();
        I2C_SendAddr(OLED_ADDR_W);
        I2C_SendByte(0x40);    //Data mode
        for (col = 0; col < 128; col++) {
            I2C_SendByte(0x00);
        }
        I2C_Stop();
    }
}

void OLED_UpdateDisplay(void) {
    unsigned int page, col;
    unsigned char *ptr = displayBuffer;
    for (page = 0; page < 4; page++) {
        OLED_SendCommand(0xB0 + page);
        OLED_SendCommand(0x00);
        OLED_SendCommand(0x10);
        I2C_Start();
        I2C_SendAddr(OLED_ADDR_W);
        I2C_SendByte(0x40);
        for (col = 0; col < 128; col++) {
            I2C_SendByte(*ptr++);
        }
        I2C_Stop();
    }
}

void OLED_DrawChar(unsigned char x, unsigned char page, char c) {
    unsigned int idx, i;
    if (c < 32 || c > 127) return;
    idx = c - 32;
    for (i = 0; i < 5; i++) {
        if ((x + i) < 128) {
            displayBuffer[page * 128 + x + i] = font5x7[idx][i];
        }
    }
}

void OLED_DrawString(unsigned char x, unsigned char page, const char *str) {
    while (*str) {
        OLED_DrawChar(x, page, *str);
        x += 6;
        str++;
    }
}

/*===================================================================
 * TIM2 Configuration: 1-second interrupt
 * System clock = 16 MHz (HSI default)
 * PSC = 15999 -> timer clock = 16MHz / 16000 = 1000 Hz
 * ARR = 999  -> interrupt every 1000 ticks = 1 second
 *===================================================================*/
void TIM2_Init(void) {
    /* Enable TIM2 clock (bit 0 of APB1ENR) */
    RCC_APB1ENR |= 0x01;
    
    /* Set prescaler: 16MHz / 16000 = 1kHz */
    TIM2_PSC = 15999;
    
    /* Auto-reload: count to 999 -> 1 second */
    TIM2_ARR = 999;
    
    //Reset counter
    TIM2_CNT = 0;
    
    /* Enable update interrupt (UIE, bit 0 of DIER) */
    TIM2_DIER |= 0x01;
    
    /* Enable TIM2 (CEN, bit 0 of CR1) */
    TIM2_CR1 |= 0x01;
    
    /* Enable TIM2 interrupt in NVIC
     * TIM2 = IRQ #28, which is in ISER0 (IRQs 0-31)
     * Bit 28 of NVIC_ISER0 */
    NVIC_ISER0 |= (1 << 28);
}

/* TIM2 Interrupt Handler
 * Called every 1 second */
void TIM2_IRQHandler(void) {
    /* Clear update interrupt flag (UIF, bit 0 of SR) */
    TIM2_SR &= ~0x01;
    
    /* Toggle colon for blinking effect */
    colon_on = !colon_on;
    
    //Increment time
    seconds++;
    if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
            minutes = 0;
            hours++;
            if (hours >= 24) {
                hours = 0;
            }
        }
    }
    
    /* Set flag so main loop knows to update display */
    tick_flag = 1;
}

/*Format time string as "HH:MM:SS" or "HH MM SS" (blink colon) */
void FormatTime(char *buf) {
    buf[0] = '0' + (hours / 10);
    buf[1] = '0' + (hours % 10);
    buf[2] = colon_on ? ':' : ' ';
    buf[3] = '0' + (minutes / 10);
    buf[4] = '0' + (minutes % 10);
    buf[5] = colon_on ? ':' : ' ';
    buf[6] = '0' + (seconds / 10);
    buf[7] = '0' + (seconds % 10);
    buf[8] = '\0';
}

/*Main Function*/
int main(void) {
    char timeBuf[12];
    
    /* Initialize system clock (from system_stm32f4xx.c) */
    SystemInit();
    
    /* Initialize peripherals */
    I2C1_GPIO_Init();
    I2C1_Init();
    OLED_Init();
    OLED_ClearScreen();
    
    /* Initialize Timer for 1-second ticks */
    TIM2_Init();
    
    /* Draw static title on Row 1 (page 0) */
    OLED_ClearBuffer();
    OLED_DrawString(10, 0, "DIGITAL CLOCK");
    
    /* Initial time display */
    FormatTime(timeBuf);
    OLED_DrawString(28, 2, timeBuf);  /* Centered on page 2 */
    OLED_UpdateDisplay();
    
    while (1) {
        if (tick_flag) {
            tick_flag = 0;
            
            /* Clear buffer and redraw */
            OLED_ClearBuffer();
            
            /* Row 1: Title */
            OLED_DrawString(10, 0, "DIGITAL CLOCK");
            
            /* Row 2: Time with blinking colon */
            FormatTime(timeBuf);
            OLED_DrawString(28, 2, timeBuf);
            
            //Send to OLED 
            OLED_UpdateDisplay();
        }
    }
}
