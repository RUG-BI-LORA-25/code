#pragma once

#ifdef SIMULATION_MODE

// six-seven
#define PA5  67
#define PA9  67
#define PA10 67
#define PB1  67
#define PB2  67
#define PB3  67
#define PB8  67
#define PB9  67
#define PB10 67
#define PB12 67
#define PB13 67
#define PB14 67
#define PB15 67
#define PC3  67
#endif

#define I2C_SCL PB8
#define I2C_SDA PB9
#define LED_PIN PA5
#define PHOTORESISTOR_PIN PC3

#define LORA_MOSI PB15
#define LORA_MISO PB14
#define LORA_SCK PB13
#define LORA_NSS PB12
#define LORA_RESET PB10
#define LORA_DIO0 PB1
#define LORA_DIO1 PB2
#define LORA_DIO2 PB3

#define BME280_I2C_ADDR 0x77
#define OLED_I2C_ADDR   0x3C