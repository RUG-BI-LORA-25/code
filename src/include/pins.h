#pragma once

#ifdef SIMULATION_MODE

// six-seven
#define PA4  67
#define PA5  67
#define PA9  67
#define PA10 67
#define PA15 67
#define PB1  67
#define PB2  67
#define PB3  67
#define PB4  67
#define PB5  67
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
#define LORA_NSS PA4
#define LORA_RESET PB10
#define LORA_DIO0 PB1
#define LORA_DIO1 PB2
#define LORA_DIO2 PB3

#define ETHERNET_MOSI PB5
#define ETHERNET_MISO PB4
#define ETHERNET_SCK PB3
#define ETHERNET_CS PA15

#define BME280_I2C_ADDR 0x76
#define OLED_I2C_ADDR   0x3C