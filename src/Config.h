/**
 * @file Config.h
 * @brief Centralized pin definitions and constants for M5DinMeter
 */
#pragma once

// --- Firmware ---
#define FW_VERSION "v1.0"

// --- Power ---
#define POWER_HOLD_PIN 46

// --- Buzzer ---
#define BUZZ_PIN 3

// --- Encoder ---
#define ENC_PIN_A 40
#define ENC_PIN_B 41

// --- Button ---
#define BTN_PWR_PIN     42
#define BTN_DEBOUNCE_MS 20

// --- Display (ST7789 via SPI) ---
#define LCD_MOSI_PIN 5
#define LCD_MISO_PIN -1
#define LCD_SCLK_PIN 6
#define LCD_DC_PIN   4
#define LCD_CS_PIN   7
#define LCD_RST_PIN  8
#define LCD_BUSY_PIN -1
#define LCD_BL_PIN   9

#define LCD_PANEL_WIDTH  135
#define LCD_PANEL_HEIGHT 240
#define LCD_OFFSET_X     52
#define LCD_OFFSET_Y     40

// --- RTC (BM8563, I2C) ---
#define RTC_SDA_PIN 11
#define RTC_SCL_PIN 12

// --- Battery ---
#define BAT_ADC_PIN 10
#define BAT_DIVIDER 2.0f
