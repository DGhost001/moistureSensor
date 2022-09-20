#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

enum {
    SETTINGS_VERSION = 1
};

struct Settings {
    uint8_t address;
};


/**
 * @brief loadSettings will load the settings from the EEPROM
 * @return if loading was successful a pointer to the settings structure is returned. If loading failed a NULL pointer is returned
 */
struct Settings * loadSettings( void );


/**
 * @brief newSettings will reset the settings
 * @return a pointer to the erased settings structure is returned
 */
struct Settings * newSettings( void );

/**
 * @brief saveSettings will save the settings to the EEPROM. This may take a while.
 *
 * This function will save the settings to the EEPROM. The settings need to be filled into the pointer returned by the
 * load / new Settings functions.
 */
void saveSettings( void );

#endif
