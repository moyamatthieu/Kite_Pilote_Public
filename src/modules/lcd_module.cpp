#include "modules/lcd_module.h"

const uint8_t LcdModule::COMMON_LCD_ADDRESSES[4] = {
    0x27, // Adresse la plus courante pour les modules PCF8574
    0x3F, // Deuxième adresse la plus courante pour les modules PCF8574A
    0x20, // Parfois utilisée sur des adaptateurs I2C modifiés
    0x38  // Utilisée par certains fabricants
};