#include "settings.h"
#include "crc16.h"

#include <avr/eeprom.h>
#include <stddef.h> //For NULL definition
#include <string.h>

struct EepromSettings {
    uint16_t version;
    struct Settings settings;
    uint16_t crc; //Keep the CRC always at the END. The code expects it to be there!
};

static struct EepromSettings eepromSettings EEMEM;
static struct EepromSettings ramSettings;

static uint16_t getRamCrc_( void )
{
    return computeCrc((uint8_t const*)&ramSettings, sizeof (struct EepromSettings) - sizeof (ramSettings.crc));
}

struct Settings * loadSettings( void )
{
    struct Settings  * settings = NULL; //In case of Error we return a NULL (default)

    eeprom_read_block(&ramSettings, &eepromSettings, sizeof(struct EepromSettings)); //Load the settings from the EEPROM

    if( getRamCrc_() == ramSettings.crc && //Check the CRC (CRC is at the end ==> if correct
            SETTINGS_VERSION == ramSettings.version //the CRC will always be 0 over the complete block) and the Version
            )
    {
        settings = &ramSettings.settings;  //If CRC is OK and Version is OK ... return the Settings
    }

    return settings;
}

struct Settings * newSettings( void )
{
    memset(&ramSettings,0, sizeof (struct EepromSettings)); //Clearout the Settings
    ramSettings.version = SETTINGS_VERSION;                 //Set the Version correct

    return &ramSettings.settings; //Let the user fill in the rest
}


void saveSettings( void )
{
    ramSettings.crc = getRamCrc_();
    eeprom_update_block(&ramSettings, &eepromSettings, sizeof (struct EepromSettings)); //Save !
}
