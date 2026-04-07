#include <stdint.h>
#include "system_stm32f4xx.h"
#include <string.h>

// Register Base Addresses
#define RCC_BASE        0x40023800
#define GPIOA_BASE      0x40020000
#define GPIOB_BASE      0x40020400
#define I2C1_BASE       0x40005400
#define TIM2_BASE       0x40000000
#define TIM3_BASE       0x40000400
#define NVIC_ISER0      (*((volatile uint32_t *)0xE000E100))

// RCC Registers
#define RCC_AHB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x30)))
#define RCC_APB1ENR     (*((volatile uint32_t *)(RCC_BASE + 0x40)))

// GPIOA Registers
#define GPIOA_MODER     (*((volatile uint32_t *)(GPIOA_BASE + 0x00)))
#define GPIOA_PUPDR     (*((volatile uint32_t *)(GPIOA_BASE + 0x0C)))
#define GPIOA_IDR       (*((volatile uint32_t *)(GPIOA_BASE + 0x10)))
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

// TIM3 Registers
#define TIM3_CR1        (*((volatile uint32_t *)(TIM3_BASE + 0x00)))
#define TIM3_DIER       (*((volatile uint32_t *)(TIM3_BASE + 0x0C)))
#define TIM3_SR         (*((volatile uint32_t *)(TIM3_BASE + 0x10)))
#define TIM3_CNT        (*((volatile uint32_t *)(TIM3_BASE + 0x24)))
#define TIM3_PSC        (*((volatile uint32_t *)(TIM3_BASE + 0x28)))
#define TIM3_ARR        (*((volatile uint32_t *)(TIM3_BASE + 0x2C)))

// Constants
#define OLED_ADDR_W     0x78
#define TEETH           20
#define OVERSPEED_THRESHOLD  800  // 80.0 km/h in tenths

// Global Variables
volatile uint32_t pulse_count = 0;
volatile uint32_t measured_freq = 0;
volatile uint32_t measured_rpm = 0;
volatile uint32_t speed_x10 = 0;        // Speed in tenths of km/h
volatile uint32_t update_flag = 0;

const uint32_t test_rpms[] = {10, 500, 3000};
volatile uint32_t rpm_index = 0;
volatile uint32_t target_rpm = 10;

volatile uint32_t btn_cooldown = 0;

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

// Utility
void delay(volatile uint32_t count) { while (count--); }

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
    RCC_AHB1ENR |= 0x03;

    // PA5 = Output (LED1)
    GPIOA_MODER &= ~(0x03 << 10);
    GPIOA_MODER |=  (0x01 << 10);

    // PA7 = Output (LED2)
    GPIOA_MODER &= ~(0x03 << 14);
    GPIOA_MODER |=  (0x01 << 14);

    // PA0 = Input (Button)
    GPIOA_MODER &= ~0x03;
    GPIOA_PUPDR &= ~0x03;

    // LEDs OFF
    GPIOA_BSRR = (1 << 21);
    GPIOA_BSRR = (1 << 23);
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
    I2C_Start(); I2C_SendAddr(OLED_ADDR_W);
    I2C_SendByte(0x00); I2C_SendByte(cmd); I2C_Stop();
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
    uint32_t p, c;
    OLED_ClearBuffer();
    for (p = 0; p < 4; p++) {
        OLED_SendCommand(0xB0 + p);
        OLED_SendCommand(0x00); OLED_SendCommand(0x10);
        I2C_Start(); I2C_SendAddr(OLED_ADDR_W); I2C_SendByte(0x40);
        for (c = 0; c < 128; c++) I2C_SendByte(0x00);
        I2C_Stop();
    }
}

void OLED_UpdateDisplay(void) {
    uint32_t p, c;
    unsigned char *ptr = displayBuffer;
    for (p = 0; p < 4; p++) {
        OLED_SendCommand(0xB0 + p);
        OLED_SendCommand(0x00); OLED_SendCommand(0x10);
        I2C_Start(); I2C_SendAddr(OLED_ADDR_W); I2C_SendByte(0x40);
        for (c = 0; c < 128; c++) I2C_SendByte(*ptr++);
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

// TIM3: Pulse generator (simulated wheel sensor)
void TIM3_SetRPM(uint32_t rpm) {
    uint32_t pulse_freq_x10;
    uint32_t arr_val;

    TIM3_CR1 &= ~0x01;

    pulse_freq_x10 = (rpm * TEETH * 10) / 60;

    if (pulse_freq_x10 == 0) return;

    if (pulse_freq_x10 < 1000) {
        TIM3_PSC = 159;
        arr_val = (1000000 / pulse_freq_x10) - 1;
    } else {
        TIM3_PSC = 15;
        arr_val = (10000000 / pulse_freq_x10) - 1;
    }

    TIM3_ARR = arr_val;
    TIM3_CNT = 0;
    TIM3_CR1 |= 0x01;
}

void TIM3_Init(void) {
    RCC_APB1ENR |= 0x02;
    TIM3_PSC = 15;
    TIM3_ARR = 999;
    TIM3_CNT = 0;
    TIM3_DIER |= 0x01;
    TIM3_CR1 |= 0x01;
    NVIC_ISER0 |= (1 << 29);
}

void TIM3_IRQHandler(void) {
    TIM3_SR &= ~0x01;
    pulse_count++;
}

// TIM2: 1-second measurement gate
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

    // Capture pulse count
    measured_freq = pulse_count;
    pulse_count = 0;

    // Compute RPM
    measured_rpm = (measured_freq * 60) / TEETH;

    // Compute vehicle speed in tenths of km/h
    // speed = RPM * 2 * pi * r * 3.6 / 60
    // speed_x10 = RPM * 1131 / 1000
    //
    // Derivation:
    //   2 * pi * 0.30 * 3.6 = 6.7858
    //   6.7858 / 60 = 0.11310
    //   0.11310 * 10 (for tenths) = 1.1310
    //   * 1000 for integer = 1131
    speed_x10 = (measured_rpm * 1131) / 1000;

    if (btn_cooldown > 0) btn_cooldown--;

    update_flag = 1;
}

// Button check (PA0)
void CheckButton(void) {
    if ((GPIOA_IDR & 0x01) && (btn_cooldown == 0)) {
        rpm_index++;
        if (rpm_index >= 3) rpm_index = 0;
        target_rpm = test_rpms[rpm_index];
        TIM3_SetRPM(target_rpm);
        btn_cooldown = 1;
    }
}

// Format speed string: "XXX.X km/h" or " XX.X km/h"
void FormatSpeed(uint32_t spd_x10, char *buf) {
    uint32_t integer_part = spd_x10 / 10;
    uint32_t frac_part = spd_x10 % 10;
    char numBuf[5];

    uint_to_str(integer_part, numBuf, 3);
    buf[0] = numBuf[0];
    buf[1] = numBuf[1];
    buf[2] = numBuf[2];
    buf[3] = '.';
    buf[4] = '0' + frac_part;
    buf[5] = '\0';
}

// Main Function
int main(void) {
    char line1[22];
    char line2[22];
    char numBuf[6];
    char spdBuf[8];

    SystemInit();

    // Initialize peripherals
    GPIO_Init();
    I2C1_GPIO_Init();
    I2C1_Init();
    OLED_Init();
    OLED_ClearScreen();

    TIM3_Init();
    TIM2_Init();

    // Set initial RPM
    target_rpm = test_rpms[0];
    TIM3_SetRPM(target_rpm);

    LED1_ON();
    LED2_OFF();

    // Initial display
    OLED_ClearBuffer();
    OLED_DrawString(4, 0, "RPM =  ---");
    OLED_DrawString(4, 2, "SPEED= --.- km/h");
    OLED_UpdateDisplay();

    while (1) {
        CheckButton();

        if (update_flag) {
            update_flag = 0;

            OLED_ClearBuffer();

            // Row 1: RPM = XXXX
            strcpy(line1, "RPM = ");
            uint_to_str(measured_rpm, numBuf, 4);
            strcat(line1, numBuf);
            OLED_DrawString(4, 0, line1);

            // Row 2: SPD= XXX.X km/h
            strcpy(line2, "SPD=");
            FormatSpeed(speed_x10, spdBuf);
            strcat(line2, spdBuf);
            strcat(line2, " km/h");
            OLED_DrawString(4, 2, line2);

            OLED_UpdateDisplay();

            // Overspeed warning: LED2 ON if > 80 km/h
            if (speed_x10 > OVERSPEED_THRESHOLD) {
                LED2_ON();
            } else {
                LED2_OFF();
            }
        }
    }
}
