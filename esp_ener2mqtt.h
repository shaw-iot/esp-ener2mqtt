/* Energenie to MQTT for ESP8266
 * Kristian Shaw 2023
 * Portions (c) David Whale (whaleygeek) and Philip Grainger (Achronite)
 */

//Parameter sizes
#define CFG_SIZE_MQTT_SERVER  40
#define CFG_SIZE_MQTT_PORT    6
#define CFG_SIZE_MQTT_USER    32
#define CFG_SIZE_MQTT_PASS    32
#define CFG_SIZE_MQTT_PUB     16

// Register addresses
#define HRF_ADDR_FIFO                  0x00
#define HRF_ADDR_OPMODE                0x01
#define HRF_ADDR_REGDATAMODUL          0x02
#define HRF_ADDR_BITRATEMSB            0x03
#define HRF_ADDR_BITRATELSB            0x04
#define HRF_ADDR_FDEVMSB               0x05
#define HRF_ADDR_FDEVLSB               0x06
#define HRF_ADDR_FRMSB                 0x07
#define HRF_ADDR_FRMID                 0x08
#define HRF_ADDR_FRLSB                 0x09
#define HRF_ADDR_AFCCTRL               0x0B
#define HRF_ADDR_VERSION               0x10
#define HRF_ADDR_LNA                   0x18
#define HRF_ADDR_RXBW                  0x19
#define HRF_ADDR_AFCFEI                0x1E
#define HRF_ADDR_IRQFLAGS1             0x27
#define HRF_ADDR_IRQFLAGS2             0x28
#define HRF_ADDR_RSSITHRESH            0x29
#define HRF_ADDR_PREAMBLELSB           0x2D
#define HRF_ADDR_SYNCCONFIG            0x2E
#define HRF_ADDR_SYNCVALUE1            0x2F
#define HRF_ADDR_SYNCVALUE2            0x30
#define HRF_ADDR_SYNCVALUE3            0x31
#define HRF_ADDR_SYNCVALUE4            0x32
#define HRF_ADDR_SYNCVALUE5            0x33
#define HRF_ADDR_SYNCVALUE6            0x34
#define HRF_ADDR_SYNCVALUE7            0x35
#define HRF_ADDR_SYNCVALUE8            0x36
#define HRF_ADDR_PACKETCONFIG1         0x37
#define HRF_ADDR_PAYLOADLEN            0x38
#define HRF_ADDR_NODEADDRESS           0x39
#define HRF_ADDR_FIFOTHRESH            0x3C


// Radio modes
#define HRF_MODE_STANDBY             0x04 // Standby
#define HRF_MODE_TRANSMITTER         0x0C // Transmiter
#define HRF_MODE_RECEIVER            0x10 // Receiver

// Values to store in registers
#define HRF_VAL_REGDATAMODUL_FSK       0x00  // Modulation scheme FSK
#define HRF_VAL_REGDATAMODUL_OOK       0x08 // Modulation scheme OOK
#define HRF_VAL_FDEVMSB30              0x01 // frequency deviation 5kHz 0x0052 -> 30kHz 0x01EC
#define HRF_VAL_FDEVLSB30              0xEC // frequency deviation 5kHz 0x0052 -> 30kHz 0x01EC
#define HRF_VAL_FRMSB434               0x6C // carrier freq -> 434.3MHz 0x6C9333
#define HRF_VAL_FRMID434               0x93 // carrier freq -> 434.3MHz 0x6C9333
#define HRF_VAL_FRLSB434               0x33 // carrier freq -> 434.3MHz 0x6C9333
#define HRF_VAL_FRMSB433               0x6C // carrier freq -> 433.92MHz 0x6C7AE1
#define HRF_VAL_FRMID433               0x7A // carrier freq -> 433.92MHz 0x6C7AE1
#define HRF_VAL_FRLSB433               0xE1 // carrier freq -> 433.92MHz 0x6C7AE1
#define HRF_VAL_AFCCTRLS               0x00 // standard AFC routine
#define HRF_VAL_AFCCTRLI               0x20 // improved AFC routine
#define HRF_VAL_LNA50                  0x08 // LNA input impedance 50 ohms
#define HRF_VAL_LNA50G                 0x0E // LNA input impedance 50 ohms, LNA gain -> 48db
#define HRF_VAL_LNA200                 0x88 // LNA input impedance 200 ohms
#define HRF_VAL_RXBW60                 0x43 // channel filter bandwidth 10kHz -> 60kHz  page:26
#define HRF_VAL_RXBW120                0x41 // channel filter bandwidth 120kHz
#define HRF_VAL_AFCFEIRX               0x04 // AFC is performed each time RX mode is entered
#define HRF_VAL_RSSITHRESH220          0xDC // RSSI threshold 0xE4 -> 0xDC (220)
#define HRF_VAL_PREAMBLELSB3           0x03 // preamble size LSB 3
#define HRF_VAL_PREAMBLELSB5           0x05 // preamble size LSB 5
#define HRF_VAL_SYNCCONFIG0            0x00 // sync word disabled
#define HRF_VAL_SYNCCONFIG1            0x80 // 1 byte  of tx sync
#define HRF_VAL_SYNCCONFIG2            0x88 // 2 bytes of tx sync
#define HRF_VAL_SYNCCONFIG3            0x90 // 3 bytes of tx sync
#define HRF_VAL_SYNCCONFIG4            0x98 // 4 bytes of tx sync
#define HRF_VAL_PAYLOADLEN255          0xFF // max Length in RX, not used in Tx
#define HRF_VAL_PAYLOADLEN66           66   // max Length in RX, not used in Tx
#define HRF_VAL_FIFOTHRESH1            0x81 // Condition to start packet transmission: at least one byte in FIFO
#define HRF_VAL_FIFOTHRESH30           0x1E // Condition to start packet transmission: wait for 30 bytes in FIFO

// Energenie specific radio config values
#define RADIO_VAL_SYNCVALUE1FSK       0x2D // 1st byte of Sync word
#define RADIO_VAL_SYNCVALUE2FSK       0xD4 // 2nd byte of Sync word
#define RADIO_VAL_SYNCVALUE1OOK       0x80 // 1nd byte of Sync word
#define RADIO_VAL_PACKETCONFIG1FSK    0xA2 // Variable length, Manchester coding, Addr must match NodeAddress
#define RADIO_VAL_PACKETCONFIG1FSKNO  0xA0 // Variable length, Manchester coding

// Masks to set and clear bits
#define HRF_MASK_REGDATAMODUL_OOK      0x08
#define HRF_MASK_REGDATAMODUL_FSK      0x00
#define HRF_MASK_WRITE_DATA            0x80
#define HRF_MASK_MODEREADY             0x80
#define HRF_MASK_FIFONOTEMPTY          0x40
#define HRF_MASK_FIFOLEVEL             0x20
#define HRF_MASK_FIFOOVERRUN           0x10
#define HRF_MASK_PACKETSENT            0x08
#define HRF_MASK_TXREADY               0x20
#define HRF_MASK_PACKETMODE            0x60
#define HRF_MASK_MODULATION            0x18
#define HRF_MASK_PAYLOADRDY            0x04

#define EXPECTED_RADIOVER 36
#define MAX_FIFO_BUFFER 66

// OT Msg lengths and positions
#define MIN_R1_MSGLEN 13
#define MAX_R1_MSGLEN 15
#define OTS_MSGLEN 14       // Switch command - Length with 1 command 1 byte sent  (3)
#define OTA_MSGLEN 13       // ACK command    - Length with 1 command 0 bytes sent (2)
#define OTH_INDEX_MFRID     1
#define OTH_INDEX_PRODUCTID 2
#define OTH_INDEX_PIP       3
#define OTH_INDEX_DEVICEID  5
#define OT_INDEX_R1_CMD     8
#define OT_INDEX_R1_TYPE    9
#define OT_INDEX_R1_VALUE  10

// Default keys for OpenThings encryption and decryption
#define CRYPT_PID 242

/* OpenThings Product IDs */
#define PRODUCTID_MIHO004 0x01   // Monitor only
#define PRODUCTID_MIHO005 0x02   // Adaptor Plus
#define PRODUCTID_MIHO006 0x05   // House Monitor
#define PRODUCTID_MIHO013 0x03   // eTRV
#define PRODUCTID_MIHO032 0x0C   // FSK motion sensor
#define PRODUCTID_MIHO033 0x0D   // FSK open sensor
#define PRODUCTID_MIHO069 0x12   // Room Thermostat

// OpenThings Rx parameters
#define OTP_ALARM           0x21
#define OTP_THERMOSTAT_MODE 0x2A // Added for MIHO069
#define OTP_DIAGNOSTICS     0x26
#define OTP_DEBUG_OUTPUT    0x2D
#define OTP_IDENTIFY        0x3F
#define OTP_SOURCE_SELECTOR 0x40 // write only
#define OTP_WATER_DETECTOR  0x41
#define OTP_GLASS_BREAKAGE  0x42
#define OTP_CLOSURES        0x43
#define OTP_DOOR_BELL       0x44
#define OTP_ENERGY          0x45
#define OTP_FALL_SENSOR     0x46
#define OTP_GAS_VOLUME      0x47
#define OTP_AIR_PRESSURE    0x48
#define OTP_ILLUMINANCE     0x49
#define OTP_TARGET_TEMP     0x4B
#define OTP_LEVEL           0x4C
#define OTP_RAINFALL        0x4D
#define OTP_APPARENT_POWER  0x50
#define OTP_POWER_FACTOR    0x51
#define OTP_REPORT_PERIOD   0x52
#define OTP_SMOKE_DETECTOR  0x53
#define OTP_TIME_AND_DATE   0x54
#define OTP_VIBRATION       0x56
#define OTP_WATER_VOLUME    0x57
#define OTP_WIND_SPEED      0x58
#define OTP_WAKEUP          0x59
#define OTP_GAS_PRESSURE    0x61
#define OTP_BATTERY_LEVEL   0x62
#define OTP_CO_DETECTOR     0x63
#define OTP_DOOR_SENSOR     0x64
#define OTP_EMERGENCY       0x65
#define OTP_FREQUENCY       0x66
#define OTP_GAS_FLOW_RATE   0x67
#define OTP_REL_HUMIDITY    0x68
#define OTP_CURRENT         0x69
#define OTP_JOIN            0x6A
#define OTP_LIGHT_LEVEL     0x6C
#define OTP_MOTION_DETECTOR 0x6D
#define OTP_OCCUPANCY       0x6F
#define OTP_REAL_POWER      0x70
#define OTP_REACTIVE_POWER  0x71
#define OTP_ROTATION_SPEED  0x72
#define OTP_SWITCH_STATE    0x73
#define OTP_TEMPERATURE     0x74
#define OTP_VOLTAGE         0x76
#define OTP_WATER_FLOW_RATE 0x77
#define OTP_WATER_PRESSURE  0x78
#define OTP_TEST            0xAA

// OpenThings record data types
#define OT_UINT   0x00
#define OT_UINT4  0x10    // 4
#define OT_UINT8  0x20    // 8
#define OT_UINT12 0x30    // 12
#define OT_UINT16 0x40    // 16
#define OT_UINT20 0x50    // 20
#define OT_UINT24 0x60    // 24
#define OT_CHAR   0x70
#define OT_SINT   0x80    // dec=128
#define OT_SINT8  0x90    // 8
#define OT_SINT16 0xA0    // 16
#define OT_SINT24 0xB0    // 24
#define OT_FLOAT  0xF0    // Not implemented yet
