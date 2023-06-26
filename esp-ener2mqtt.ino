/* Energenie to MQTT for ESP8266
 * Kristian Shaw 2023
 * Portions (c) David Whale (whaleygeek) and Philip Grainger (Achronite)
 */

#define LED       D0 // NodeMCU LED at pin GPIO16 (D0) 
#define SS        D8 // PIN to enable SPI transaction on RFM69
#define RESETRAD  D1 // PIN to reset RFM69
#define RESETBUT  D3 // PIN to start AP

#include <ESP8266WiFi.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include "SPI.h"
#include "esp_ener2mqtt.h"
#include "TimeLib.h"
#include "PubSubClient.h" // MQTT
#include "NTPClient.h"
#include "WiFiUdp.h"
#include <Preferences.h>

// Used to setup each SPI transaction. Run at 10Mhz.
SPISettings RFM69SPI(10000000, MSBFIRST, SPI_MODE0);

WiFiClient espClient;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
PubSubClient mqttClient(espClient);
bool portalRunning = false;
bool startup_ok = false;
bool do_save_param = false;
Preferences prefs;
unsigned long counter_10sec;
unsigned long counter_1sec;

char mqtt_server[CFG_SIZE_MQTT_SERVER] = "";
char mqtt_port[CFG_SIZE_MQTT_PORT] = "1883"; 
char mqtt_user[CFG_SIZE_MQTT_USER] = "";
char mqtt_pass[CFG_SIZE_MQTT_PASS] = "";
char mqtt_pub[CFG_SIZE_MQTT_PUB] = "ener";

// Extra configuration parameters
WiFiManagerParameter custom_mqtt_server("mqtt_server", "MQTT Server", mqtt_server, CFG_SIZE_MQTT_SERVER);
WiFiManagerParameter custom_mqtt_port("mqtt_port", "MQTT Port", mqtt_port, CFG_SIZE_MQTT_PORT);
WiFiManagerParameter custom_mqtt_username("mqtt_user", "MQTT Username", mqtt_user, CFG_SIZE_MQTT_USER);
WiFiManagerParameter custom_mqtt_password("mqtt_pass", "MQTT Password", mqtt_pass, CFG_SIZE_MQTT_PORT);
WiFiManagerParameter custom_mqtt_pub_topic("mqtt_pub", "MQTT Publish Topic", mqtt_pub, CFG_SIZE_MQTT_PUB);

// Global variable for WiFi
WiFiManager wifiManager;

struct openThingsDevice 
{
  uint8_t manufacturer_id;
  uint8_t product_id;
  uint32_t device_id;
  String records;
};

struct OTRecType {
  uint8_t ot_type;
  String ot_name;
};

static struct OTRecType OTRecTypes[] = {
{ OTP_ALARM, "ALARM" },
{ OTP_THERMOSTAT_MODE, "THERMOSTAT_MODE" },
{ OTP_DIAGNOSTICS, "DIAGNOSTICS" },
{ OTP_DEBUG_OUTPUT, "DEBUG_OUTPUT" },
{ OTP_IDENTIFY, "IDENTIFY" },
{ OTP_SOURCE_SELECTOR, "SOURCE_SELECTOR" },
{ OTP_WATER_DETECTOR, "WATER_DETECTOR" },
{ OTP_GLASS_BREAKAGE, "GLASS_BREAKAGE" },
{ OTP_CLOSURES, "CLOSURES" },
{ OTP_DOOR_BELL, "DOOR_BELL" },
{ OTP_ENERGY, "ENERGY" },
{ OTP_FALL_SENSOR, "FALL_SENSOR" },
{ OTP_GAS_VOLUME, "GAS_VOLUME" },
{ OTP_AIR_PRESSURE, "AIR_PRESSURE" },
{ OTP_ILLUMINANCE, "ILLUMINANCE" },
{ OTP_TARGET_TEMP, "TARGET_TEMP" },
{ OTP_LEVEL, "LEVEL" },
{ OTP_RAINFALL, "RAINFALL" },
{ OTP_APPARENT_POWER, "APPARENT_POWER" },
{ OTP_POWER_FACTOR, "POWER_FACTOR" },
{ OTP_REPORT_PERIOD, "REPORT_PERIOD" },
{ OTP_SMOKE_DETECTOR, "SMOKE_DETECTOR" },
{ OTP_TIME_AND_DATE, "TIME_AND_DATE" },
{ OTP_VIBRATION, "VIBRATION" },
{ OTP_WATER_VOLUME, "WATER_VOLUME" },
{ OTP_WIND_SPEED, "WIND_SPEED" },
{ OTP_WAKEUP, "WAKEUP" },
{ OTP_GAS_PRESSURE, "GAS_PRESSURE" },
{ OTP_BATTERY_LEVEL, "BATTERY_LEVEL" },
{ OTP_CO_DETECTOR, "CO_DETECTOR" },
{ OTP_DOOR_SENSOR, "DOOR_SENSOR" },
{ OTP_EMERGENCY, "EMERGENCY" },
{ OTP_FREQUENCY, "FREQUENCY" },
{ OTP_GAS_FLOW_RATE, "GAS_FLOW_RATE" },
{ OTP_REL_HUMIDITY, "REL_HUMIDITY" },
{ OTP_CURRENT, "CURRENT" },
{ OTP_JOIN, "JOIN" },
{ OTP_LIGHT_LEVEL, "LIGHT_LEVEL" },
{ OTP_MOTION_DETECTOR, "MOTION_DETECTOR" },
{ OTP_OCCUPANCY, "OCCUPANCY" },
{ OTP_REAL_POWER, "REAL_POWER" },
{ OTP_REACTIVE_POWER, "REACTIVE_POWER" },
{ OTP_ROTATION_SPEED, "ROTATION_SPEED" },
{ OTP_SWITCH_STATE, "SWITCH_STATE" },
{ OTP_TEMPERATURE, "TEMPERATURE" },
{ OTP_VOLTAGE, "VOLTAGE" },
{ OTP_WATER_FLOW_RATE, "WATER_FLOW_RATE" },
{ OTP_WATER_PRESSURE, "WATER_PRESSURE" },
{ OTP_TEST, "TEST" },
};

#define NUM_OT_PARAMS (sizeof(OTRecTypes)/sizeof(OTRecTypes[0]))

// Convert type to name via lookup
String OTTypeToString(uint8_t type)
{
  for (int i=0; i<NUM_OT_PARAMS; i++)
  {
    //Serial.println("OTTypeToString: type: " + String(type) + ", NUM_OT_PARAMS: " + String(NUM_OT_PARAMS) + ", ot_type: " + String(OTRecTypes[i].ot_type));
    if (OTRecTypes[i].ot_type == type)
    {
      return OTRecTypes[i].ot_name;
    }
  }
  return "Unknown";
}

void setupRFM69RX() {
 Serial.println("setupRFM69RX: Resetting HRF69");
 digitalWrite(RESETRAD, HIGH);
 delayMicroseconds(100);
 digitalWrite(RESETRAD, LOW);
 delay(5);
 Serial.println("setupRFM69RX: Setting HRF69 mode to FSK");
 setReg(HRF_ADDR_REGDATAMODUL, HRF_VAL_REGDATAMODUL_FSK);      // modulation scheme FSK
 setReg(HRF_ADDR_FDEVMSB, HRF_VAL_FDEVMSB30);                  // frequency deviation 5kHz 0x0052 -> 30kHz 0x01EC
 setReg(HRF_ADDR_FDEVLSB, HRF_VAL_FDEVLSB30);                  // frequency deviation 5kHz 0x0052 -> 30kHz 0x01EC
 setReg(HRF_ADDR_FRMSB, HRF_VAL_FRMSB434);                     // carrier freq -> 434.3MHz 0x6C9333
 setReg(HRF_ADDR_FRMID, HRF_VAL_FRMID434);                     // carrier freq -> 434.3MHz 0x6C9333
 setReg(HRF_ADDR_FRLSB, HRF_VAL_FRLSB434);                     // carrier freq -> 434.3MHz 0x6C9333
 setReg(HRF_ADDR_AFCCTRL, HRF_VAL_AFCCTRLS);                   // standard AFC routine
 setReg(HRF_ADDR_LNA, HRF_VAL_LNA50);                          // 200ohms, gain by AGC loop -> 50ohms
 setReg(HRF_ADDR_RXBW, HRF_VAL_RXBW60);                        // channel filter bandwidth 10kHz -> 60kHz  page:26
 setReg(HRF_ADDR_BITRATEMSB, 0x1A);                            // 4800b/s
 setReg(HRF_ADDR_BITRATELSB, 0x0B);                            // 4800b/s
 setReg(HRF_ADDR_SYNCCONFIG, HRF_VAL_SYNCCONFIG2);             // Size of the Synch word = 2 (SyncSize + 1)
 setReg(HRF_ADDR_SYNCVALUE1, RADIO_VAL_SYNCVALUE1FSK);         // 1st byte of Sync word
 setReg(HRF_ADDR_SYNCVALUE2, RADIO_VAL_SYNCVALUE2FSK);         // 2nd byte of Sync word
 setReg(HRF_ADDR_PACKETCONFIG1, RADIO_VAL_PACKETCONFIG1FSK);   // Variable length, Manchester coding, NodeAddress filtering
 setReg(HRF_ADDR_PAYLOADLEN, HRF_VAL_PAYLOADLEN66);            // max Length in RX, not used in Tx
 setReg(HRF_ADDR_NODEADDRESS, 0x04);                           // Node address used in address filtering (not used) - PTG was 0x06 gpbenton uses 0x04
 setReg(HRF_ADDR_OPMODE, HRF_MODE_RECEIVER);                   // RX
 // Output radio info
 Serial.printf("HRF_ADDR_OPMODE:0x%02X\n",getReg(HRF_ADDR_OPMODE));
 Serial.printf("HRF_ADDR_REGDATAMODUL:0x%02X\n",getReg(HRF_ADDR_REGDATAMODUL));
 Serial.printf("HRF_ADDR_VERSION:0x%02X\n",getReg(HRF_ADDR_VERSION));
 Serial.printf("HRF_ADDR_PAYLOADLEN:0x%02X\n",getReg(HRF_ADDR_PAYLOADLEN));
}

/* 
 * Start reading from FRM69
 * Toggle SS on/off to indicate a transaction
 */
void startSPI()
{
  SPI.beginTransaction(RFM69SPI);
  digitalWrite(SS, LOW);
  digitalWrite(LED, LOW); //turn the led on
}

/* 
 * End reading from FRM69
 * Toggle SS on/off to indicate a transaction
 */
void endSPI()
 {
   digitalWrite(SS, HIGH);
   SPI.endTransaction();
   digitalWrite(LED, HIGH); //turn the led off
 }

/* 
 * Get a register value from the RFM69
 * Toggle SS on/off to indicate a transaction
 */
byte getReg(byte reg) 
{
 byte returnedValue = 0;
 startSPI();
 SPI.transfer(reg);
 returnedValue = SPI.transfer(0x00);
 endSPI();
 return returnedValue;
}

/* 
 * Get a register value from the RFM69
 */
void setReg(byte reg, byte val) 
{
 startSPI();
 uint8_t txbuf[2];
 Serial.printf("setReg: Setting register 0x%02X to 0x%02X\n",reg,val);
 txbuf[0] = reg|HRF_MASK_WRITE_DATA;
 txbuf[1] = val;
 SPI.transfer(txbuf,2);
 endSPI();
}

uint8_t cryptByte(uint8_t data, uint16_t& g_ran)
{
  for (int i = 0; i < 5; i++) {
    if ((g_ran & 0x01) != 0) {
      g_ran = ((g_ran >> 1) ^ 62965); // bit0 set
    } else {
      g_ran = g_ran >> 1; // bit0 clear
    }
  }
  return (g_ran ^ data ^ 90);
}

uint16_t calculateCRC(uint8_t msg[], uint8_t length)
{
  uint8_t ch, bit;
  uint16_t rem = 0;

  for (int i = OTH_INDEX_DEVICEID; i <= length; i++) {
    ch = msg[i];
    rem = rem ^ (ch << 8);
    for (bit = 0; bit < 8; bit++) {
      if ((rem & (1 << 15)) != 0) {
        // bit is set
        rem = ((rem << 1) ^ 0x1021);
      } else {
        // bit is clear
        rem = (rem << 1);
       }
      }
    }
    return rem;
}

/* 
 * Dump the payload data in a screen printable format
 */
int payloadToChar(uint8_t payload[], uint8_t payload_start, uint8_t payload_len, char* buf, int buf_len) 
{
  int i;
  if ((payload_len - payload_start) > buf_len) return 0; // too large

  for(i = 0; i < payload_len-payload_start; i++) {
    sprintf(buf + i * 3, "%02X ", payload[i+payload_start]);
  }
  // Last byte
  sprintf(buf + i * 3, "%02X ", payload[i+payload_start]);
  return 1;
}

/* 
 * Payload contains n records (voltage, current, frequency etc). Each 
 * record is variable length with a speccific variable type.
 */
String decodeRecord(uint8_t payload[], int &pos, uint8_t payload_len, bool retfl)
{
  //Serial.println("pos in dr:" + String(pos));
  if (pos >= (payload_len-1)) return ""; // About to go off the end of the buffer, we are in the CRC
  
  int datapos = 0;  // Position in local data array
  uint8_t data[4] = {0, 0, 0, 0}; // Used to store record data
  uint8_t var_type = payload[pos] & 0xF0;
  uint8_t var_len = payload[pos] & 0x0F;
  uint32_t val = 0;
  float fval = 0;
  
  pos++;  // Move payload to next byte
  
  for (datapos = 0; ((datapos<var_len) && (pos<=payload_len-2)); datapos++) // make sure we don't read past the end, 2 byte CRC 
  {
    //Serial.println("Reading record value count: " + (String)var_len + ", datapos: " + (String)datapos + ", pos: " + (String)pos);
    data[datapos] = payload[pos];
    pos++;
  }

  switch (var_type) {
    case OT_UINT:
    {
      for (int j=0; j<var_len; j++)
        val = (val << 8) + data[j];
      return String(val);
    }
    case OT_UINT4:
    case OT_UINT8:
    case OT_UINT12:
    case OT_UINT16:
    case OT_UINT20:
    {
      for (int j=0; j<var_len; j++)
      {
        val = (val << 8) + data[j];
      }
      break;
    }
    case OT_SINT:
    case OT_SINT8:
    case OT_SINT16:
    case OT_SINT24:
    {
      //Serial.println("OT_SINTx");
      for (int j=0; j<var_len; j++)
      {
        val = (val << 8) + data[j];
      }
      if ((data[0] & 0x80) == 0x80)
                    {
                        // negative
                        val = -(((!val) & ((2 ^ (var_len * 8)) - 1)) + 1);
                    }
      return String((int32_t)val); //cast to signed int
      break;
    }
    default:
      // Unhandled type
      Serial.printf("Unhandled record value type: 0x%02X\n", var_type);
      break;
  } // end switch

    switch (var_type) {
    case OT_UINT4:
      return String((float)val / 16,6); // 2^4
    case OT_UINT8:
      return String((float)val / 256,6); // 2^8
    case OT_UINT12:
      return String((float)val / 4096,6); // 2^12
    case OT_UINT16:
      return String((float)val / 65536,6); // 2^16
    case OT_UINT20:
      return String((float)val / 1048576,6); // 2^20
    case OT_UINT24:
      return String((float)val / 16777216,6); // 2^24
    default:
      break;
  } // end switch for float type
  
  return "0"; //default return

} // end decodeRecord

/* 
 * Decrypt the payload and read out the records
 * Perform basic checks that CRC is correct and it's recognised brand.
 */
int decodePayload(uint8_t payload[], uint8_t payload_start, uint8_t payload_len, openThingsDevice &dev)
{
  uint8_t num_recs = 0;
  // First check CRC in last 2 bytes of packet footer matches computed CRC
  uint16_t crc = (payload[payload_len - 1] << 8) + payload[payload_len];
  uint16_t crc_calc = calculateCRC(payload, payload_len-2); // Skip last 2 bytes as these are the CRC
  Serial.printf("crc_calc:%d, crc:%d\n",crc_calc, crc);
  if (crc != crc_calc) return 0;
  
  dev.manufacturer_id = payload[OTH_INDEX_MFRID];
  //if (dev.manufacturer_id != 0x04) return 0;      // 0x04 = Energene
  
  dev.product_id = payload[OTH_INDEX_PRODUCTID];
  dev.device_id = (uint32_t)(payload[OTH_INDEX_DEVICEID] << 16) + (payload[OTH_INDEX_DEVICEID + 1] << 8) + payload[OTH_INDEX_DEVICEID + 2];
  
  for (int i=OT_INDEX_R1_CMD; (i<=payload_len-2) && payload[i]!=0; ) // ignore last 2 bytes for CRC, ignore record type 0x0
  { 
    //Serial.println("Decoding record type: " + String(payload[i],HEX));
    dev.records = dev.records + ",\"" + OTTypeToString(payload[i]) + "\":" + decodeRecord(payload, ++i, payload_len, true);
  }
  return 1;
}

void decryptPayload(uint8_t payload[], uint8_t payload_start, uint8_t payload_len)
{
  uint16_t pip = (uint16_t)((payload[OTH_INDEX_PIP] << 8) | payload[OTH_INDEX_PIP + 1]); // Encryption
  uint16_t ran = ((((uint16_t)CRYPT_PID) << 8) ^ pip);
  for (int i = payload_start; i <= payload_len; i++)
    {
      payload[i] = cryptByte(payload[i], ran);
    }
}

String oTToJson(openThingsDevice &dev)
{
  time_t ts;
  if(timeClient.isTimeSet()) {
    ts = timeClient.getEpochTime();
  } else {
    ts = now(); // Fallback to time since boot
  }
  //
  String ot_json = "{";
  ot_json = ot_json + "\"deviceId\":" + (String)dev.device_id + ","; 
  ot_json = ot_json + "\"mfrId\":" + (String)dev.manufacturer_id + ",";
  ot_json = ot_json + "\"productId\":" + (String)dev.product_id + ",";
  ot_json = ot_json + "\"timestamp\":" + ts;
  ot_json = ot_json + dev.records;
  ot_json = ot_json + "}";
  return ot_json;
}

int getBufferedPacket(uint8_t buf[], uint8_t &payload_len)
{
  if ((getReg(HRF_ADDR_IRQFLAGS2) & HRF_MASK_PAYLOADRDY) ==  HRF_MASK_PAYLOADRDY) {
    //Serial.println("Data ready in FIFO");
    startSPI();
    SPI.transfer(HRF_ADDR_FIFO);
    buf[0] = SPI.transfer(0x00);

    payload_len = buf[0];
    Serial.printf("Payload len: %d\n",payload_len);

    if (payload_len > MAX_FIFO_BUFFER) payload_len = MAX_FIFO_BUFFER; // clamp the read to the radio FIFO size

    if (payload_len == 0) {
      endSPI(); // Finish SPI transaction
      return 0; // Invalid FIFO
    }

    for (int i=1; i<=payload_len; i++ ) {
      buf[i] = SPI.transfer(0x00);
    }

    endSPI();
    char printbuf[(MAX_FIFO_BUFFER *3) + 1] = ""; // 66*3 + 1 = max FIFO + terminator
    payloadToChar(buf, 1, payload_len, printbuf, sizeof(printbuf)); // skip first byte as that's the payload size
    Serial.printf("Payload: %s\n",printbuf);
    return 1;
  }
  return 0;
}

String getParam(String name)
{
  //read parameter from server, for customhmtl input
  String value;
  if(wifiManager.server->hasArg(name)) {
    value = wifiManager.server->arg(name);
  }
  return value;
}

// Copy configuration from flash
void loadParam() {
  prefs.getString("mqtt_server", mqtt_server, CFG_SIZE_MQTT_SERVER);
  prefs.getString("mqtt_port", mqtt_port, CFG_SIZE_MQTT_PORT);
  prefs.getString("mqtt_user", mqtt_user, CFG_SIZE_MQTT_USER);
  prefs.getString("mqtt_pass", mqtt_pass, CFG_SIZE_MQTT_PASS);
  prefs.getString("mqtt_pub", mqtt_pub, CFG_SIZE_MQTT_PUB);
}

// Get configuration from WiFiManager Parameters
void getWMParam() {
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_username.getValue());
  strcpy(mqtt_pass, custom_mqtt_password.getValue());
  strcpy(mqtt_pub, custom_mqtt_pub_topic.getValue());
}

// Copy configuration to flash
void saveParam() {
  Serial.println("Saving details from WiFiManager to Preferences");
  prefs.putString("mqtt_server", custom_mqtt_server.getValue());
  prefs.putString("mqtt_port", custom_mqtt_port.getValue());
  prefs.putString("mqtt_user", custom_mqtt_username.getValue());
  prefs.putString("mqtt_pass", custom_mqtt_password.getValue());
  prefs.putString("mqtt_pub", custom_mqtt_pub_topic.getValue());
  prefs.putInt("boot_count", 0); // clear bootup counter
  do_save_param = false;
  Serial.printf("Save complete, restarting. Params: mqtt_server:%s\n", mqtt_server);
  ESP.restart();
}

void saveParamCallback()
{
  do_save_param = true;
}

int connectMQTT()
{
  //Serial.printf("Trying to reconnect MQTT server:%s:%s, user:%s, pass:%s\n",mqtt_server, mqtt_port, mqtt_user, mqtt_pass);
  if (!strcmp(custom_mqtt_server.getValue(),"")) {
    Serial.println("MQTT Server not set");
    return 0;
  }
  mqttClient.setServer(custom_mqtt_server.getValue(), (uint16_t)strtol(custom_mqtt_port.getValue(), NULL, 0));
  if (mqttClient.connect("esp_ener2mqtt", custom_mqtt_username.getValue(), custom_mqtt_password.getValue())) {
    Serial.println("MQTT connected");
    return 1;
  } else {
    Serial.println("Failed to connect to MQTT");
    return 0;
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT); //LED pin as output
  pinMode(SS, OUTPUT);
  pinMode(RESETRAD, OUTPUT);
  pinMode(RESETBUT, INPUT_PULLUP);
  digitalWrite(RESETRAD, LOW);
  Serial.println();
  Serial.println("Initialising preferences");
  prefs.begin("esp_ener2mqtt");
  int boot_count = prefs.getInt("boot_count", 1); // default to 1
  loadParam(); // Load configuration from flash
  Serial.printf("Reboot count: %d\n", boot_count);

  if (boot_count >= 7) {
    Serial.println("Resetting WiFi setting");
    wifiManager.resetSettings();
    ESP.eraseConfig();
    delay(100);
  } else {
    boot_count++;
    prefs.putInt("boot_count", boot_count);
    // Copy flash configuration into web portal
    custom_mqtt_server.setValue(mqtt_server, CFG_SIZE_MQTT_SERVER);
    custom_mqtt_port.setValue(mqtt_port, CFG_SIZE_MQTT_PORT);
    custom_mqtt_username.setValue(mqtt_user, CFG_SIZE_MQTT_USER);
    custom_mqtt_password.setValue(mqtt_pass, CFG_SIZE_MQTT_PASS);
    custom_mqtt_pub_topic.setValue(mqtt_pub, CFG_SIZE_MQTT_PUB);
    //Serial.printf("Loaded config: mqtt_server:%s, mqtt_port:%s, mqtt_user:%s, mqtt_pass:%s, mqtt_pub:%s\n",mqtt_server, mqtt_port, mqtt_user, mqtt_pass, mqtt_pub);
  }

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_username);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_pub_topic);
  wifiManager.setSaveParamsCallback(saveParamCallback);

  if (!wifiManager.autoConnect("ESP_ENER2MQTT")) {
    Serial.println("Failed to configure WiFi, restarting");
    delay(3000);
    ESP.restart();
  }
  // Now get any changed parameters
  getWMParam();
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.startWebPortal();
  portalRunning = true;
  
  timeClient.begin();
  SPI.begin();
  setupRFM69RX();
  counter_10sec = millis(); // 10 second loop counter
  counter_1sec = counter_10sec; // 1 second loop counter
  Serial.println("setup: Done setup");
}

void loop()
{
  uint8_t buf[MAX_FIFO_BUFFER];
  uint8_t payload_len;
  
  timeClient.update();
  wifiManager.process();
  mqttClient.loop();
  
  // Start AP configuration portal?
  if(digitalRead(RESETBUT) == LOW) {
    if(!portalRunning) {
      wifiManager.stopWebPortal();
    }
    Serial.println("Config button pressed, starting AP");
    wifiManager.setConfigPortalBlocking(true); 
    wifiManager.startConfigPortal();
    portalRunning = true;
  }
  
  // Save parameters for MQTT to flash
  if (do_save_param) { 
    saveParam();
  }

  if ((millis() - counter_10sec) > 10000) {
      //Serial.println("10 second loop tasks");
      // If we've been running for 10 secs then we must have started up OK
      if (!startup_ok) {
        startup_ok =  true;
        prefs.putInt("boot_count", 0); // clear bootup counter
      }
      counter_10sec = millis();
  }

  if ((millis() - counter_1sec) > 1000) {
      //Serial.println("1 second loop tasks");
      counter_1sec = millis();
      
      if (!mqttClient.connected()) {
        connectMQTT();
      }
  }
  
  if (getBufferedPacket(buf, payload_len)) {
    openThingsDevice device;
    decryptPayload(buf, OTH_INDEX_DEVICEID, payload_len);  // Start decrypting from the device ID byte
    if (decodePayload(buf, 1, payload_len, device)) {
      char printbuf[(MAX_FIFO_BUFFER *3) + 1] = ""; // 66*3 + 1 = max FIFO + terminator
      payloadToChar(buf, 1, payload_len, printbuf, sizeof(printbuf)); // skip first byte as that's the payload size
      Serial.printf("Decrypted: %s\n",printbuf);
      Serial.println(oTToJson(device));
      String pubTopic = (String(custom_mqtt_pub_topic.getValue()) + "/" + String(device.product_id) + "/" + String(device.device_id) + "/stat");
      String pubValue = oTToJson(device);
      bool retain_msg = false;
      if (device.product_id = PRODUCTID_MIHO033) { // Open sensor
        retain_msg = true;
      }
      if (mqttClient.connected() & timeClient.isTimeSet()) {
        mqttClient.publish(pubTopic.c_str(),pubValue.c_str(),retain_msg);
      }
    } else {
      Serial.println("CRC error, ignoring packet");
    }
  }
  // Main loop end
  delay(150);
}
