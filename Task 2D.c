#include <stdint.h>
#include "system_stm32f4xx.h"
#include <string.h>

// Register Base Addresses
#define RCC_BASE        0x40023800
#define GPIOA_BASE      0x40020000
#define GPIOB_BASE      0x40020400
#define I2C1_BASE       0x40005400
#define TIM2_BASE       0x40000000
#define ADC1_BASE       0x40012000
#define NVIC_ISER0      (*((volatile uint32_t *)0xE000E100))

// RCC Registers
#define RCC_AHB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x30)))
#define RCC_APB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x40)))
#define RCC_APB2ENR     (*((volatile uint32_t *)(RCC_BASE + 0x44)))

// GPIOA Registers
#define GPIOA_MODER     (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_BSRR      (*((volatile uint32_t *)(GPIOA_BASE + 0x18)))

// GPIOB Registers
#define GPIOB_MODER     (*((volatile uint32_t *)(GPIOB_BASE + 0x00)))
#define GPIOB_OTYPER    (*((volatile uint32_t *)(GPIOB_BASE + 0x04)))
#define GPIOB_OSPEEDR   (*((volatile uint32_t *)(GPIOB_BASE + 0x08)))
#define GPIOB_PUPDR     (*((volatile uint32_t *)(GPIOB_BASE + 0x0C)))
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

// ADC1 Registers
#define ADC1_SR         (*((volatile uint32_t *)(ADC1_BASE + 0x00)))
#define ADC1_CR1        (*((volatile uint32_t *)(ADC1_BASE + 0x04)))
#define ADC1_CR2        (*((volatile uint32_t *)(ADC1_BASE + 0x08)))
#define ADC1_SMPR2      (*((volatile uint32_t *)(ADC1_BASE + 0x10)))
#define ADC1_SQR1       (*((volatile uint32_t *)(ADC1_BASE + 0x2C)))
#define ADC1_SQR3       (*((volatile uint32_t *)(ADC1_BASE + 0x34)))
#define ADC1_DR         (*((volatile uint32_t *)(ADC1_BASE + 0x4C)))

// Constants
#define OLED_ADDR_W     0x78
#define R1_OHM          5600    // R1 = 5.6 kOhm
#define R2_OHM          3300    // R2 = 3.3 kOhm
#define LOW_BATT_THRESHOLD  600 // 6.00V in hundredths
#define NUM_SAMPLES     32      // ADC averaging samples

// Global Variables
volatile uint32_t update_flag = 0;
volatile uint32_t adc_raw = 0;
volatile uint32_t voltage_in_hundredths = 0;    // Input voltage x100
volatile uint32_t adc_voltage_mv = 0;           // ADC pin voltage in mV

// Display Buffer
unsigned char displayBuffer[512];

// 5x7 Font Table (compact)
static const unsigned char font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, {0x00,0x00,0x5F,0x00,0x00},
    {0x00,0x07,0x00,0x07,0x00}, {0x14,0x7F,0x14,0x7F,0x14},
    {0x24,0x2A,0x7F,0x2A,0x12}, {0x23,0x13,0x08,0x64,0x62},
    {0x36,0x49,0x55,0x22,0x50}, {0x00,0x05,0x03,0x00,0x00},
    {0x00,0x1C,0x22,0x41,0x00}, {0x00,0x41,0x22,0x1C,0x00},
    {0x14,0x08,0x3E,0x08,0x14}, {0x08,0x08,0x3E,0x08,0x08},
    {0x00,0x50,0x30,0x00,0x00}, {0x08,0x08,0x08,0x08,0x08},
    {0x00,0x60,0x60,0x00,0x00}, {0x20,0x10,0x08,0x04,0x02},
    {0x3E,0x51,0x49,0x45,0x3E}, {0x00,0x42,0x7F,0x40,0x00},
    {0x42,0x61,0x51,0x49,0x46}, {0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10}, {0x27,0x45,0x45,0x45,0x39},
    {0x3C,0x4A,0x49,0x49,0x30}, {0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36}, {0x06,0x49,0x49,0x29,0x1E},
    {0x00,0x36,0x36,0x00,0x00}, {0x00,0x56,0x36,0x00,0x00},
    {0x08,0x14,0x22,0x41,0x00}, {0x14,0x14,0x14,0x14,0x14},
    {0x00,0x41,0x22,0x14,0x08}, {0x02,0x01,0x51,0x09,0x06},
    {0x32,0x49,0x79,0x41,0x3E}, {0x7E,0x11,0x11,0x11,0x7E},
    {0x7F,0x49,0x49,0x49,0x36}, {0x3E,0x41,0x41,0x41,0x22},
    {0x7F,0x41,0x41,0x22,0x1C}, {0x7F,0x49,0x49,0x49,0x41},
    {0x7F,0x09,0x09,0x09,0x01}, {0x3E,0x41,0x49,0x49,0x7A},
    {0x7F,0x08,0x08,0x08,0x7F}, {0x00,0x41,0x7F,0x41,0x00},
    {0x20,0x40,0x41,0x3F,0x01}, {0x7F,0x08,0x14,0x22,0x41},
    {0x7F,0x40,0x40,0x40,0x40}, {0x7F,0x02,0x0C,0x02,0x7F},
    {0x7F,0x04,0x08,0x10,0x7F}, {0x3E,0x41,0x41,0x41,0x3E},
    {0x7F,0x09,0x09,0x09,0x06}, {0x3E,0x41,0x51,0x21,0x5E},
    {0x7F,0x09,0x19,0x29,0x46}, {0x46,0x49,0x49,0x49,0x31},
    {0x01,0x01,0x7F,0x01,0x01}, {0x3F,0x40,0x40,0x40,0x3F},
    {0x1F,0x20,0x40,0x20,0x1F}, {0x3F,0x40,0x38,0x40,0x3F},
    {0x63,0x14,0x08,0x14,0x63}, {0x07,0x08,0x70,0x08,0x07},
    {0x61,0x51,0x49,0x45,0x43}, {0x00,0x7F,0x41,0x41,0x00},
    {0x02,0x04,0x08,0x10,0x20}, {0x00,0x41,0x41,0x7F,0x00},
    {0x04,0x02,0x01,0x02,0x04}, {0x40,0x40,0x40,0x40,0x40},
    {0x00,0x01,0x02,0x04,0x00}, {0x20,0x54,0x54,0x54,0x78},
    {0x7F,0x48,0x44,0x44,0x38}, {0x38,0x44,0x44,0x44,0x20},
    {0x38,0x44,0x44,0x48,0x7F}, {0x38,0x54,0x54,0x54,0x18},
    {0x08,0x7E,0x09,0x01,0x02}, {0x0C,0x52,0x52,0x52,0x3E},
    {0x7F,0x08,0x04,0x04,0x78}, {0x00,0x44,0x7D,0x40,0x00},
    {0x20,0x40,0x44,0x3D,0x00}, {0x7F,0x10,0x28,0x44,0x00},
    {0x00,0x41,0x7F,0x40,0x00}, {0x7C,0x04,0x78,0x04,0x78},
    {0x7C,0x08,0x04,0x04,0x78}, {0x38,0x44,0x44,0x44,0x38},
    {0x7C,0x14,0x14,0x14,0x08}, {0x08,0x14,0x14,0x18,0x7C},
    {0x7C,0x08,0x04,0x04,0x08}, {0x48,0x54,0x54,0x54,0x20},
    {0x04,0x3F,0x44,0x40,0x20}, {0x3C,0x40,0x40,0x20,0x7C},
    {0x1C,0x20,0x40,0x20,0x1C}, {0x3C,0x40,0x30,0x40,0x3C},
    {0x44,0x28,0x10,0x28,0x44}, {0x0C,0x50,0x50,0x50,0x3C},
    {0x44,0x64,0x54,0x4C,0x44}, {0x00,0x08,0x36,0x41,0x00},
    {0x00,0x00,0x7F,0x00,0x00}, {0x00,0x41,0x36,0x08,0x00},
    {0x10,0x08,0x08,0x10,0x08}, {0x00,0x00,0x00,0x00,0x00}
};

// Utility Functions
void delay(volatile uint32_t count) {
    while (count--);
}

void uint_to_str(uint32_t val, char *buf, int width) {
    int i;
    for (i = width - 1; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    buf[width] = '\0';
    for (i = 0; i < width - 1; i++) {
        if (buf[i] == '0') buf[i] = ' ';
        else break;
    }
}

// GPIO Init
void GPIO_Init(void) {
    RCC_AHB1ENR |= 0x03;       // Enable GPIOA and GPIOB

    // PA5 = Output (LED1)
    GPIOA_MODER &= ~(0x03 << 10);
    GPIOA_MODER |=  (0x01 << 10);

    // PA7 = Output (LED2)
    GPIOA_MODER &= ~(0x03 << 14);
    GPIOA_MODER |=  (0x01 << 14);

    // PA1 = Analog (ADC input) - bits [3:2] = 11
    GPIOA_MODER |= (0x03 << 2);

    // LEDs OFF
    GPIOA_BSRR = (1 << 21);    // PA5 OFF
    GPIOA_BSRR = (1 << 23);    // PA7 OFF
}

void LED1_ON(void)  { GPIOA_BSRR = (1 << 5);  }
void LED1_OFF(void) { GPIOA_BSRR = (1 << 21); }
void LED2_ON(void)  { GPIOA_BSRR = (1 << 7);  }
void LED2_OFF(void) { GPIOA_BSRR = (1 << 23); }

// I2C1 Functions
void I2C1_GPIO_Init(void) {
    RCC_AHB1ENR |= 0x02;
    GPIOB_MODER &= ~0x000F0000;
    GPIOB_MODER |=  0x000A0000;
    GPIOB_OTYPER |= 0x0300;
    GPIOB_OSPEEDR |= 0x000F0000;
    GPIOB_PUPDR &= ~0x000F0000;
    GPIOB_PUPDR |=  0x00050000;
    GPIOB_AFRH &= ~0xFF;
    GPIOB_AFRH |=  0x44;
}

void I2C1_Init(void) {
    RCC_APB1ENR |= 0x00200000;
    I2C1_CR1 = 0;
    I2C1_CR1 = 0x8000;
    I2C1_CR1 = 0;
    I2C1_CR2 = 16;
    I2C1_CCR = 0x50;
    I2C1_TRISE = 17;
    I2C1_CR1 = 0x01;
}

void I2C_Start(void) {
    I2C1_CR1 |= 0x0100;
    while (!(I2C1_SR1 & 0x01));
}

void I2C_SendAddr(unsigned char addr) {
    I2C1_DR = addr;
    while (!(I2C1_SR1 & 0x02));
    (void)I2C1_SR1; (void)I2C1_SR2;
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
    OLED_SendCommand(0xA1); OLED_SendCommand(0xC8);
    OLED_SendCommand(0xDA); OLED_SendCommand(0x02);
    OLED_SendCommand(0x81); OLED_SendCommand(0x8F);
    OLED_SendCommand(0xD9); OLED_SendCommand(0xF1);
    OLED_SendCommand(0xDB); OLED_SendCommand(0x40);
    OLED_SendCommand(0xA4); OLED_SendCommand(0xA6);
    OLED_SendCommand(0xAF);
    delay(200000);
}

void OLED_ClearBuffer(void) { memset(displayBuffer, 0, 512); }

void OLED_ClearScreen(void) {
    uint32_t page, col;
    OLED_ClearBuffer();
    for (page = 0; page < 4; page++) {
        OLED_SendCommand(0xB0 + page);
        OLED_SendCommand(0x00); OLED_SendCommand(0x10);
        I2C_Start(); I2C_SendAddr(OLED_ADDR_W); I2C_SendByte(0x40);
        for (col = 0; col < 128; col++) I2C_SendByte(0x00);
        I2C_Stop();
    }
}

void OLED_UpdateDisplay(void) {
    uint32_t page, col;
    unsigned char *ptr = displayBuffer;
    for (page = 0; page < 4; page++) {
        OLED_SendCommand(0xB0 + page);
        OLED_SendCommand(0x00); OLED_SendCommand(0x10);
        I2C_Start(); I2C_SendAddr(OLED_ADDR_W); I2C_SendByte(0x40);
        for (col = 0; col < 128; col++) I2C_SendByte(*ptr++);
        I2C_Stop();
    }
}

void OLED_DrawChar(unsigned char x, unsigned char page, char c) {
    uint32_t idx, i;
    if (c < 32 || c > 127) return;
    idx = c - 32;
    for (i = 0; i < 5; i++)
        if ((x + i) < 128)
            displayBuffer[page * 128 + x + i] = font5x7[idx][i];
}

void OLED_DrawString(unsigned char x, unsigned char page, const char *str) {
    while (*str) { OLED_DrawChar(x, page, *str); x += 6; str++; }
}

// ADC1 Init: Channel 1 (PA1), 12-bit, single conversion
void ADC1_Init(void) {
    RCC_APB2ENR |= 0x0100;     // Enable ADC1 clock

    // Sample time for ch1: 84 cycles (100 in bits [5:3])
    ADC1_SMPR2 &= ~0x38;
    ADC1_SMPR2 |= 0x20;

    // Sequence length = 1
    ADC1_SQR1 &= ~0x00F00000;

    // First conversion = channel 1
    ADC1_SQR3 &= ~0x1F;
    ADC1_SQR3 |= 0x01;

    // 12-bit resolution
    ADC1_CR1 = 0x00;

    // Enable ADC, EOCS=1
    ADC1_CR2 = 0x00000401;

    delay(10000);               // Stabilization
}

// Read ADC with averaging
uint32_t ADC1_ReadAvg(uint32_t samples) {
    uint32_t sum = 0;
    uint32_t i;

    for (i = 0; i < samples; i++) {
        // Start conversion
        ADC1_CR2 |= 0x40000000;

        // Wait for EOC
        while (!(ADC1_SR & 0x02));

        // Read and accumulate
        sum += (ADC1_DR & 0x0FFF);

        delay(100);
    }

    return sum / samples;
}

// TIM2: 1-second update interval
void TIM2_Init(void) {
    RCC_APB1ENR |= 0x01;
    TIM2_PSC = 15999;
    TIM2_ARR = 999;
    TIM2_CNT = 0;
    TIM2_DIER |= 0x01;
    TIM2_CR1 |= 0x01;
    NVIC_ISER0 |= (1 << 28);
}

void TIM2_IRQHandler(void) {
    TIM2_SR &= ~0x01;
    update_flag = 1;
}

// Format voltage string: "X.XX V"
// Input: voltage in hundredths of a volt (e.g., 900 = 9.00V)
void FormatVoltage(uint32_t hundredths, char *buf) {
    // Integer part (ones digit for 0-9V)
    buf[0] = '0' + (hundredths / 100);
    buf[1] = '.';
    buf[2] = '0' + ((hundredths / 10) % 10);
    buf[3] = '0' + (hundredths % 10);
    buf[4] = ' ';
    buf[5] = 'V';
    buf[6] = '\0';
}

// Main Function
int main(void) {
    char line1[22];
    char line2[22];
    char numBuf[6];
    char voltBuf[8];
    uint32_t raw;
    uint32_t v_in;

    SystemInit();

    // Initialize peripherals
    GPIO_Init();
    I2C1_GPIO_Init();
    I2C1_Init();
    OLED_Init();
    OLED_ClearScreen();
    ADC1_Init();
    TIM2_Init();

    // System ready
    LED1_ON();
    LED2_OFF();

    // Initial display
    OLED_ClearBuffer();
    OLED_DrawString(4, 0, "ADC = ----");
    OLED_DrawString(4, 2, "BATT= -.-- V");
    OLED_UpdateDisplay();

    while (1) {
        if (update_flag) {
            update_flag = 0;

            // Read ADC (averaged)
            raw = ADC1_ReadAvg(NUM_SAMPLES);
            adc_raw = raw;

            // Compute original battery voltage (before divider)
            //
            // V_ADC_mV = (raw * 3300) / 4095
            // V_in_mV  = V_ADC_mV * (R1 + R2) / R2
            //          = (raw * 3300 * 8900) / (4095 * 3300)
            //          = (raw * 8900) / 4095
            //
            // For hundredths of a volt:
            // V_in_hundredths = (raw * 890) / 4095
            v_in = (raw * 890 + 2047) / 4095;   // +2047 for rounding
            voltage_in_hundredths = v_in;

            // Also compute ADC pin voltage for reference
            adc_voltage_mv = (raw * 3300) / 4095;

            // Build display
            OLED_ClearBuffer();

            // Row 1: ADC VALUE = XXXX
            strcpy(line1, "ADC = ");
            uint_to_str(raw, numBuf, 4);
            strcat(line1, numBuf);
            OLED_DrawString(4, 0, line1);

            // Row 2: BATT= X.XX V  STATUS
            strcpy(line2, "BATT= ");
            FormatVoltage(v_in, voltBuf);
            strcat(line2, voltBuf);

            // Append status
            if (v_in < LOW_BATT_THRESHOLD) {
                strcat(line2, " LOW");
            }
            OLED_DrawString(4, 2, line2);

            OLED_UpdateDisplay();

            // LED2: Low battery warning
            if (v_in < LOW_BATT_THRESHOLD) {
                LED2_ON();
            } else {
                LED2_OFF();
            }
        }
    }
}
