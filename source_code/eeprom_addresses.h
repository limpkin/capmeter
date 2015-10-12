/*
 * eeprom_addresses.h
 *
 * Created: 01/07/2015 16:07:03
 *  Author: limpkin
 */ 


#ifndef EEPROM_ADDRESSES_H_
#define EEPROM_ADDRESSES_H_

// Bool define
#define EEPROM_BOOL_OK_VAL          0xDD

// Address defines
#define EEP_OE_CALIB_DONE_BOOL      0
#define EEP_OE_CALIB_DATA           1
#define EEP_APP_STORED_DATA         50

// Size defines
#define APP_STORED_DATA_MAX_SIZE    (1024-EEP_APP_STORED_DATA)

#endif /* EEPROM_ADDRESSES_H_ */