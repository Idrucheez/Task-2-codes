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

// TIM2 Registers (1-sec measurement gate)
#define TIM2_CR1        (*((volatile uint32_t *)(TIM2_BASE + 0x00)))
#define TIM2_DIER       (*((volatile uint32_t *)(TIM2_BASE + 0x0C)))
#define TIM2_SR         (*((volatile uint32_t *)(TIM2_BASE + 0x10)))
#define TIM2_CNT        (*((volatile uint32_t *)(TIM2_BASE + 0x24)))
#define TIM2_PSC        (*((volatile uint32_t *)(TIM2_BASE + 0x28)))
#define TIM2_ARR        (*((volatile uint32_t *)(TIM2_BASE + 0x2C)))

// TIM3 Registers (pulse generation)
#define TIM3_CR1        (*((volatile uint32_t *)(TIM3_BASE + 0x00)))
#define TIM3_DIER       (*((volatile uint32_t *)(TIM3_BASE + 0x0C)))
#define TIM3_SR         (*((volatile uint32_t *)(TIM3_BASE + 0x10)))
#define TIM3_CNT        (*((volatile uint32_t *)(TIM3_BASE + 0x24)))
#define TIM3_PSC        (*((volatile uint32_t *)(TIM3_BASE + 0x28)))
#define TIM3_ARR        (*((volatile uint32_t *)(TIM3_BASE + 0x2C)))

// OLED
#define OLED_ADDR_W     0x78

// Constants
#define TEETH           20      // Tone wheel teeth count

// Global Variables
volatile uint32_t pulse_count = 0;      // Pulses counted in current 1-sec window
volatile uint32_t measured_freq = 0;    // Measured pulse frequency (Hz)
volatile uint32_t measured_rpm = 0;     // Computed RPM
volatile uint32_t update_flag = 0;      // Display refresh flag

// RPM test points
const uint32_t test_rpms[] = {10, 500, 3000};
volatile uint32_t rpm_index = 0;        // Current test RPM index
volatile uint32_t target_rpm = 10;      // Current target RPM

// Button debounce
volatile uint32_t btn_pressed = 0;
volatile uint32_t btn_cooldown = 0;

// Display Buffer
unsigned char displayBuffer[512];

// 5x7 Font Table
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
void delay(volatile uint32_t count) {
    while (count--);
}

// Number to string helper (for display)
// Writes up to 5 digits right-justified, no leading zeros
void uint_to_str(uint32_t val, char *buf, int width) {
    int i;
    for (i = width - 1; i >= 0; i--) {
        buf[i] = '0' + (val % 10);
        val /= 10;
    }
    buf[width] = '\0';
    // Replace leading zeros with spaces (except last digit)
    for (i = 0; i < width - 1; i++) {
        if (buf[i] == '0') buf[i] = ' ';
        else break;
    }
}

// GPIO Init
void GPIO_Init(void) {
    // Enable GPIOA and GPIOB clocks
    RCC_AHB1ENR |= 0x03;
    
    // PA5 = Output (LED1 System Ready)
    GPIOA_MODER &= ~(0x03 << 10);
    GPIOA_MODER |=  (0x01 << 10);
    
    // PA7 = Output (LED2 Fault)
    GPIOA_MODER &= ~(0x03 << 14);
    GPIOA_MODER |=  (0x01 << 14);
    
    // PA0 = Input (Button), no pull (external pull-down)
    GPIOA_MODER &= ~0x03;
    GPIOA_PUPDR &= ~0x03;
    
    // LEDs OFF initially
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
        I2C_Start(); I2C_SendAddr(OLED_ADDR_W);
        I2C_SendByte(0x40);
        for (col = 0; col < 128; col++) I2C_SendByte(0x00);
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
        I2C_Start(); I2C_SendAddr(OLED_ADDR_W);
        I2C_SendByte(0x40);
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

// Reconfigure TIM3 for the target RPM
//
// Pulse frequency = (RPM * TEETH) / 60
// TIM3 clock = 16 MHz / (PSC+1)
// TIM3 interrupt rate = TIM3_clock / (ARR+1) = desired pulse freq
//
// Strategy: Use PSC=15 -> TIM3 clock = 1 MHz
//   ARR = (1000000 / pulse_freq) - 1
//
// For very low frequencies (10 RPM -> 3.33 Hz):
//   Use PSC=159 -> TIM3 clock = 100 kHz
//   ARR = (100000 / 3.33) - 1 = 29999
void TIM3_SetRPM(uint32_t rpm) {
    uint32_t pulse_freq_x10;    // Pulse freq * 10 for precision
    uint32_t arr_val;
    
    // Disable TIM3 while reconfiguring
    TIM3_CR1 &= ~0x01;
    
    // pulse_freq = (rpm * TEETH) / 60
    // Use x10 for precision: pulse_freq_x10 = (rpm * TEETH * 10) / 60
    pulse_freq_x10 = (rpm * TEETH * 10) / 60;
    
    if (pulse_freq_x10 == 0) {
        // RPM = 0, don't start timer
        return;
    }
    
    if (pulse_freq_x10 < 1000) {
        // Low frequency: use PSC=159 -> 100 kHz timer clock
        // ARR = (100000 * 10) / pulse_freq_x10 - 1
        TIM3_PSC = 159;
        arr_val = (1000000 / pulse_freq_x10) - 1;
    } else {
        // Higher frequency: use PSC=15 -> 1 MHz timer clock
        // ARR = (1000000 * 10) / pulse_freq_x10 - 1
        TIM3_PSC = 15;
        arr_val = (10000000 / pulse_freq_x10) - 1;
    }
    
    TIM3_ARR = arr_val;
    TIM3_CNT = 0;
    
    // Re-enable TIM3
    TIM3_CR1 |= 0x01;
}

// TIM3 Init: Pulse generator
void TIM3_Init(void) {
    // Enable TIM3 clock (bit 1 of APB1ENR)
    RCC_APB1ENR |= 0x02;
    
    // Initial config - will be set by TIM3_SetRPM
    TIM3_PSC = 15;
    TIM3_ARR = 999;
    TIM3_CNT = 0;
    TIM3_DIER |= 0x01;         // Enable update interrupt
    TIM3_CR1 |= 0x01;          // Enable timer
    
    // TIM3 IRQ = IRQ #29 -> bit 29 of NVIC_ISER0
    NVIC_ISER0 |= (1 << 29);
}

// TIM3 ISR: Called at the pulse frequency rate
// Each call = one simulated sensor pulse
void TIM3_IRQHandler(void) {
    TIM3_SR &= ~0x01;          // Clear update flag
    pulse_count++;              // Count this pulse
}

// TIM2 Init: 1-second measurement gate
void TIM2_Init(void) {
    RCC_APB1ENR |= 0x01;       // Enable TIM2 clock
    TIM2_PSC = 15999;           // 16MHz / 16000 = 1kHz
    TIM2_ARR = 999;             // 1000 ticks = 1 second
    TIM2_CNT = 0;
    TIM2_DIER |= 0x01;
    TIM2_CR1 |= 0x01;
    NVIC_ISER0 |= (1 << 28);   // TIM2 IRQ #28
}

// TIM2 ISR: Runs every 1 second
// Captures pulse count, computes frequency and RPM
void TIM2_IRQHandler(void) {
    TIM2_SR &= ~0x01;
    
    // Capture and reset pulse count
    measured_freq = pulse_count;
    pulse_count = 0;
    
    // Compute RPM = (frequency * 60) / TEETH
    measured_rpm = (measured_freq * 60) / TEETH;
    
    // Debounce cooldown for button
    if (btn_cooldown > 0) btn_cooldown--;
    
    update_flag = 1;
}

// Check button (PA0) with debounce
void CheckButton(void) {
    if ((GPIOA_IDR & 0x01) && (btn_cooldown == 0)) {
        // Button pressed - cycle to next RPM
        rpm_index++;
        if (rpm_index >= 3) rpm_index = 0;
        target_rpm = test_rpms[rpm_index];
        
        // Reconfigure TIM3 for new RPM
        TIM3_SetRPM(target_rpm);
        
        // Debounce: ignore button for 1 second
        btn_cooldown = 1;
    }
}

// Main Function
int main(void) {
    char line1[22];
    char line2[22];
    char numBuf[6];
    
    SystemInit();
    
    // Initialize all peripherals
    GPIO_Init();
    I2C1_GPIO_Init();
    I2C1_Init();
    OLED_Init();
    OLED_ClearScreen();
    
    // Init timers
    TIM3_Init();
    TIM2_Init();
    
    // Set initial RPM target
    target_rpm = test_rpms[0];  // Start at 10 RPM
    TIM3_SetRPM(target_rpm);
    
    // System ready
    LED1_ON();
    LED2_OFF();
    
    // Show initial display
    OLED_ClearBuffer();
    OLED_DrawString(4, 0, "RPM = ----");
    OLED_DrawString(4, 2, "FREQ= ---- Hz");
    OLED_UpdateDisplay();
    
    while (1) {
        // Check button for RPM change
        CheckButton();
        
        if (update_flag) {
            update_flag = 0;
            
            OLED_ClearBuffer();
            
            // Row 1: RPM = XXXX
            strcpy(line1, "RPM = ");
            uint_to_str(measured_rpm, numBuf, 4);
            strcat(line1, numBuf);
            OLED_DrawString(4, 0, line1);
            
            // Row 2: FREQ= XXXX Hz
            strcpy(line2, "FREQ= ");
            uint_to_str(measured_freq, numBuf, 4);
            strcat(line2, numBuf);
            strcat(line2, " Hz");
            OLED_DrawString(4, 2, line2);
            
            OLED_UpdateDisplay();
            
            // LED2: fault if no pulses detected
            if (measured_freq == 0) {
                LED2_ON();
            } else {
                LED2_OFF();
            }
        }
    }
}
