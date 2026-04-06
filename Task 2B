#include <stdint.h>
#include "system_stm32f4xx.h"
#include <string.h>

// Register Base Addresses
#define RCC_BASE        0x40023800
#define GPIOA_BASE      0x40020000
#define GPIOB_BASE      0x40020400
#define I2C1_BASE       0x40005400
#define TIM2_BASE       0x40000000
#define NVIC_ISER0      (*((volatile uint32_t *)0xE000E100))

// RCC Registers
#define RCC_AHB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x30)))
#define RCC_APB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x40)))

// GPIOA Registers (LED1 = PA5)
#define GPIOA_MODER     (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_ODR       (*((volatile uint32_t *)(GPIOA_BASE + 0x14)))
#define GPIOA_BSRR      (*((volatile uint32_t *)(GPIOA_BASE + 0x18)))

// GPIOB Registers (LED2 = PB0, OLED = PB8/PB9)
#define GPIOB_MODER     (*((volatile uint32_t *)(GPIOB_BASE + 0x00)))
#define GPIOB_OTYPER    (*((volatile uint32_t *)(GPIOB_BASE + 0x04)))
#define GPIOB_OSPEEDR   (*((volatile uint32_t *)(GPIOB_BASE + 0x08)))
#define GPIOB_PUPDR     (*((volatile uint32_t *)(GPIOB_BASE + 0x0C)))
#define GPIOB_ODR       (*((volatile uint32_t *)(GPIOB_BASE + 0x14)))
#define GPIOB_BSRR      (*((volatile uint32_t *)(GPIOB_BASE + 0x18)))
#define GPIOB_AFRH      (*((volatile uint32_t *)(GPIOB_BASE + 0x24)))

// I2C1 Registers
#define I2C1_CR1        (*((volatile uint32_t *)(I2C1_BASE + 0x00)))
#define I2C1_CR2        (*((volatile uint32_t *)(I2C1_BASE + 0x04)))
#define I2C1_DR         (*((volatile uint32_t *)(I2C1_BASE + 0x10)))
#define I2C1_SR1        (*((volatile uint32_t *)(I2C1_BASE + 0x14)))
#define I2C1_SR2        (*((volatile uint32_t *)(I2C1_BASE + 0x18)))
#define I2C1_CCR        (*((volatile uint32_t *)(I2C1_BASE + 0x1C)))
#define I2C1_TRISE      (*((volatile uint32_t *)(I2C1_BASE + 0x20)))

// TIM2 Registers
#define TIM2_CR1        (*((volatile uint32_t *)(TIM2_BASE + 0x00)))
#define TIM2_DIER       (*((volatile uint32_t *)(TIM2_BASE + 0x0C)))
#define TIM2_SR         (*((volatile uint32_t *)(TIM2_BASE + 0x10)))
#define TIM2_CNT        (*((volatile uint32_t *)(TIM2_BASE + 0x24)))
#define TIM2_PSC        (*((volatile uint32_t *)(TIM2_BASE + 0x28)))
#define TIM2_ARR        (*((volatile uint32_t *)(TIM2_BASE + 0x2C)))

// OLED Constants
#define OLED_ADDR_W     0x78

// System Status Definitions
#define STATUS_READY      0
#define STATUS_OVERSPEED  1
#define STATUS_SENSOR_FAULT 2

// Global Variables
volatile uint32_t tick_count = 0;       // Seconds counter
volatile uint32_t update_flag = 0;      // Display update flag
volatile uint32_t system_status = STATUS_READY;

// Simulated sensor values
volatile uint32_t sim_speed = 0;        // Simulated vehicle speed km/h
volatile uint32_t sim_pulse_ok = 1;     // Simulated pulse signal present

// Display Buffer
unsigned char displayBuffer[512];

// 5x7 Font Table (ASCII 32 to 127)
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

// Utility
void delay(volatile uint32_t count) {
    while (count--);
}

// LED GPIO Init
// LED1 = PA5 (on-board LD2, active high)
// LED2 = PA6 (external red LED, active high)
void LED_Init(void) {
    // Enable GPIOA clock (bit 0)
    RCC_AHB1ENR |= 0x01;
    
    // PA5 = Output mode (01), bits [11:10]
    GPIOA_MODER &= ~(0x03 << 10);
    GPIOA_MODER |=  (0x01 << 10);
    
    // PA6 = Output mode (01), bits [13:12]
    GPIOA_MODER &= ~(0x03 << 12);
    GPIOA_MODER |=  (0x01 << 12);
    
    // Both LEDs OFF initially
    GPIOA_BSRR = (1 << 21);    // PA5 reset (OFF)
    GPIOA_BSRR = (1 << 22);    // PA6 reset (OFF)
}

void LED1_ON(void)  { GPIOA_BSRR = (1 << 5);  }   // PA5 set
void LED1_OFF(void) { GPIOA_BSRR = (1 << 21); }   // PA5 reset
void LED2_ON(void)  { GPIOA_BSRR = (1 << 6);  }   // PA6 set
void LED2_OFF(void) { GPIOA_BSRR = (1 << 22); }   // PA6 reset

// I2C1 Functions
void I2C1_GPIO_Init(void) {
    RCC_AHB1ENR |= 0x02;       // Enable GPIOB clock
    
    GPIOB_MODER &= ~0x000F0000;
    GPIOB_MODER |=  0x000A0000; // PB8, PB9 = AF
    GPIOB_OTYPER |= 0x0300;    // Open-drain
    GPIOB_OSPEEDR |= 0x000F0000; // High speed
    GPIOB_PUPDR &= ~0x000F0000;
    GPIOB_PUPDR |=  0x00050000; // Pull-up
    GPIOB_AFRH &= ~0xFF;
    GPIOB_AFRH |=  0x44;       // AF4 for I2C1
}

void I2C1_Init(void) {
    RCC_APB1ENR |= 0x00200000; // Enable I2C1 clock
    I2C1_CR1 = 0;
    I2C1_CR1 = 0x8000;         // Software reset
    I2C1_CR1 = 0;
    I2C1_CR2 = 16;             // 16 MHz APB1
    I2C1_CCR = 0x50;           // 100 kHz
    I2C1_TRISE = 17;
    I2C1_CR1 = 0x01;           // Enable
}

void I2C_Start(void) {
    I2C1_CR1 |= 0x0100;
    while (!(I2C1_SR1 & 0x01));
}

void I2C_SendAddr(unsigned char addr) {
    I2C1_DR = addr;
    while (!(I2C1_SR1 & 0x02));
    (void)I2C1_SR1;
    (void)I2C1_SR2;
}

void I2C_SendByte(unsigned char data) {
    while (!(I2C1_SR1 & 0x80));
    I2C1_DR = data;
    while (!(I2C1_SR1 & 0x04));
}

void I2C_Stop(void) {
    I2C1_CR1 |= 0x0200;
    delay(100);
}

// OLED Functions
void OLED_SendCommand(unsigned char cmd) {
    I2C_Start();
    I2C_SendAddr(OLED_ADDR_W);
    I2C_SendByte(0x00);
    I2C_SendByte(cmd);
    I2C_Stop();
}

void OLED_Init(void) {
    delay(800000);
    OLED_SendCommand(0xAE);
    OLED_SendCommand(0xD5); OLED_SendCommand(0x80);
    OLED_SendCommand(0xA8); OLED_SendCommand(0x1F);
    OLED_SendCommand(0xD3); OLED_SendCommand(0x00);
    OLED_SendCommand(0x40);
    OLED_SendCommand(0x8D); OLED_SendCommand(0x14);
    OLED_SendCommand(0x20); OLED_SendCommand(0x00);
    OLED_SendCommand(0xA1);
    OLED_SendCommand(0xC8);
    OLED_SendCommand(0xDA); OLED_SendCommand(0x02);
    OLED_SendCommand(0x81); OLED_SendCommand(0x8F);
    OLED_SendCommand(0xD9); OLED_SendCommand(0xF1);
    OLED_SendCommand(0xDB); OLED_SendCommand(0x40);
    OLED_SendCommand(0xA4);
    OLED_SendCommand(0xA6);
    OLED_SendCommand(0xAF);
    delay(200000);
}

void OLED_ClearBuffer(void) {
    memset(displayBuffer, 0, 512);
}

void OLED_ClearScreen(void) {
    uint32_t page, col;
    OLED_ClearBuffer();
    for (page = 0; page < 4; page++) {
        OLED_SendCommand(0xB0 + page);
        OLED_SendCommand(0x00);
        OLED_SendCommand(0x10);
        I2C_Start();
        I2C_SendAddr(OLED_ADDR_W);
        I2C_SendByte(0x40);
        for (col = 0; col < 128; col++)
            I2C_SendByte(0x00);
        I2C_Stop();
    }
}

void OLED_UpdateDisplay(void) {
    uint32_t page, col;
    unsigned char *ptr = displayBuffer;
    for (page = 0; page < 4; page++) {
        OLED_SendCommand(0xB0 + page);
        OLED_SendCommand(0x00);
        OLED_SendCommand(0x10);
        I2C_Start();
        I2C_SendAddr(OLED_ADDR_W);
        I2C_SendByte(0x40);
        for (col = 0; col < 128; col++)
            I2C_SendByte(*ptr++);
        I2C_Stop();
    }
}

void OLED_DrawChar(unsigned char x, unsigned char page, char c) {
    uint32_t idx, i;
    if (c < 32 || c > 127) return;
    idx = c - 32;
    for (i = 0; i < 5; i++) {
        if ((x + i) < 128)
            displayBuffer[page * 128 + x + i] = font5x7[idx][i];
    }
}

void OLED_DrawString(unsigned char x, unsigned char page, const char *str) {
    while (*str) {
        OLED_DrawChar(x, page, *str);
        x += 6;
        str++;
    }
}

// TIM2: 1-second interrupt for fault simulation
// HSI = 16 MHz, PSC = 15999, ARR = 999 -> 1 sec
void TIM2_Init(void) {
    RCC_APB1ENR |= 0x01;       // Enable TIM2 clock
    TIM2_PSC = 15999;
    TIM2_ARR = 999;
    TIM2_CNT = 0;
    TIM2_DIER |= 0x01;         // Enable update interrupt
    TIM2_CR1 |= 0x01;          // Enable timer
    NVIC_ISER0 |= (1 << 28);   // Enable TIM2 IRQ in NVIC
}

// TIM2 ISR: Runs every 1 second
// Simulates fault conditions by cycling through states
void TIM2_IRQHandler(void) {
    TIM2_SR &= ~0x01;          // Clear update flag
    
    tick_count++;
    update_flag = 1;
    
    // Fault Simulation Logic:
    //  0-4 sec:  Normal operation (speed = 60 km/h, pulse OK)
    //  5-9 sec:  Overspeed (speed = 120 km/h)
    // 10-14 sec: Sensor fault (pulse lost)
    // 15 sec:    Reset cycle
    if (tick_count < 5) {
        sim_speed = 60;
        sim_pulse_ok = 1;
    } else if (tick_count < 10) {
        sim_speed = 120;
        sim_pulse_ok = 1;
    } else if (tick_count < 15) {
        sim_speed = 0;
        sim_pulse_ok = 0;
    } else {
        tick_count = 0;         // Reset cycle
        sim_speed = 60;
        sim_pulse_ok = 1;
    }
}

// Evaluate system status based on simulated sensor data
uint32_t EvaluateStatus(void) {
    // Check for sensor fault (no pulses)
    if (!sim_pulse_ok) {
        return STATUS_SENSOR_FAULT;
    }
    // Check for overspeed (> 80 km/h threshold)
    if (sim_speed > 80) {
        return STATUS_OVERSPEED;
    }
    // Normal operation
    return STATUS_READY;
}

// Update LEDs based on system status
void UpdateLEDs(uint32_t status) {
    if (status == STATUS_READY) {
        LED1_ON();      // System ready
        LED2_OFF();     // No fault
    } else {
        LED1_ON();      // System still running
        LED2_ON();      // Fault detected!
    }
}

// Update OLED with status information
void UpdateDisplay(uint32_t status) {
    char speedBuf[20];
    
    OLED_ClearBuffer();
    
    // Row 1: System status message
    switch (status) {
        case STATUS_READY:
            OLED_DrawString(16, 0, "SYSTEM READY");
            break;
        case STATUS_OVERSPEED:
            OLED_DrawString(4, 0, "OVERSPEED WARN!");
            break;
        case STATUS_SENSOR_FAULT:
            OLED_DrawString(10, 0, "SENSOR FAULT");
            break;
    }
    
    // Row 2: Speed or fault detail
    switch (status) {
        case STATUS_READY:
            // Show speed
            speedBuf[0] = 'S';
            speedBuf[1] = 'P';
            speedBuf[2] = 'D';
            speedBuf[3] = '=';
            speedBuf[4] = '0' + (sim_speed / 100);
            speedBuf[5] = '0' + ((sim_speed / 10) % 10);
            speedBuf[6] = '0' + (sim_speed % 10);
            speedBuf[7] = ' ';
            speedBuf[8] = 'k';
            speedBuf[9] = 'm';
            speedBuf[10] = '/';
            speedBuf[11] = 'h';
            speedBuf[12] = '\0';
            OLED_DrawString(16, 2, speedBuf);
            break;
        case STATUS_OVERSPEED:
            speedBuf[0] = 'S';
            speedBuf[1] = 'P';
            speedBuf[2] = 'D';
            speedBuf[3] = '=';
            speedBuf[4] = '0' + (sim_speed / 100);
            speedBuf[5] = '0' + ((sim_speed / 10) % 10);
            speedBuf[6] = '0' + (sim_speed % 10);
            speedBuf[7] = ' ';
            speedBuf[8] = 'k';
            speedBuf[9] = 'm';
            speedBuf[10] = '/';
            speedBuf[11] = 'h';
            speedBuf[12] = '\0';
            OLED_DrawString(16, 2, speedBuf);
            break;
        case STATUS_SENSOR_FAULT:
            OLED_DrawString(4, 2, "NO PULSE DETECT");
            break;
    }
    
    OLED_UpdateDisplay();
}

// Main Function
int main(void) {
    uint32_t status;
    
    SystemInit();
    
    // Initialize peripherals
    LED_Init();
    I2C1_GPIO_Init();
    I2C1_Init();
    OLED_Init();
    OLED_ClearScreen();
    
    // Initialize timer for fault simulation
    TIM2_Init();
    
    // System initialization complete - turn on LED1
    LED1_ON();
    
    // Show initial status
    OLED_ClearBuffer();
    OLED_DrawString(16, 0, "SYSTEM READY");
    OLED_DrawString(16, 2, "SPD=060 km/h");
    OLED_UpdateDisplay();
    
    while (1) {
        if (update_flag) {
            update_flag = 0;
            
            // Evaluate system status
            status = EvaluateStatus();
            
            // Update LEDs
            UpdateLEDs(status);
            
            // Update OLED display
            UpdateDisplay(status);
        }
    }
}
