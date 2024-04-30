/********************************************************************************************************************************************
* Code to run once at beginning
********************************************************************************************************************************************/

#include "definitions.h" 

void setup() 
{
  Serial.begin(115200);
  wait(500);
  wait(1000);

  display.init();
  display.setBrightness(255);
  
  display.clear();
  header();  
  display.drawString(10, 21, "Setup");
  display.drawString(10, 32, "in progress..."); 
  display.display();

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(ledArray, 5);
  wait(100);
  
  // Display version on serial monitor
  //Serial.print("\nNTP Ship's Bell Clock v");
  //Serial.print(VERSION_MAJOR); Serial.print("."); Serial.print(VERSION_MINOR); Serial.println(VERSION_BUILD);

  Serial.printf("\nNTP Ship's Bell Clock v%i.%i%i\r\n", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

  WiFi.hostname("Ship_Bell_Clock");

  Serial.print("Chip ID: ");     Serial.println(ESP.getChipId(), HEX);

  DF1201SSerial.begin(115200);
  wait(100);  

  // Display orange  while connecting 
  fourLeds(ORANGE); 

  while(!DF1201S.begin(DF1201SSerial))
  {
    Serial.println("Init failed, please check the connection!");
    delay(1000);
  }
  DF1201S.setVol(0);                           // set volume so music mode isn't announced 
  DF1201S.setLED(true);                        // turn off onboard LED
  DF1201S.setPrompt(false);
  DF1201S.switchFunction(DF1201S.MUSIC);       // set music mode     
  //wait(2000);                                  // wait for the end of the prompt tone  
  DF1201S.setPlayMode(DF1201S.SINGLE);         // set playback mode 
  Serial.print("Player initialized - Play Mode: ");
  Serial.println(DF1201S.getPlayMode());

  EEPROM.begin(sizeof(uint8_t));     
  wait(100);

  fileSetsOnPlayer = DF1201S.getTotalFile() / 8;                // there are 8 files per set

  EEPROM.get(0, chimeFileSet);  
  if (chimeFileSet > fileSetsOnPlayer || chimeFileSet < 1)      // if gibberish comes in, fix it
  {
    chimeFileSet = 1;
    EEPROM.put(0, chimeFileSet);
    EEPROM.commit();
  } 

  functionButton.attachClick(buttonClick);                     // advance chime file set
  functionButton.attachDoubleClick(buttonDoubleClick);         // not used
  functionButton.attachLongPressStop(buttonLongPressStop);
  functionButton.attachLongPressStart(buttonLongPressStart);   // advance manual UTC offset
  functionButton.setDebounceMs(50);
  functionButton.setClickMs(200);                           // single click time
  functionButton.setPressMs(650);                           // long press time   
      
  // Pin settings - LEDs on Pin 0 (NodeMCU D3)
  // pinMode for function button was set when its object was created 
  pinMode(DISPLAY_PIN, INPUT_PULLUP);
  pinMode(BUILT_IN_LED, OUTPUT);  
  digitalWrite(BUILT_IN_LED, HIGH);         // start with the LED off 
   
  Serial.println("Connecting to WiFi ...");
  wait(1500);  

  /* 
  Create WiFiManager object 'wifiManager' and connect to the previously used WiFi network.  
  If the previous network is not available or there is no previous network, WiFiManager 
  starts a blocking loop and sets up a WiFi access point on the ESP8266 called 'Ship_Bell_Clock.'
  The access point can be logged onto using a phone or computer with no password required.   
  A page is displayed on the device where the WiFi network the clock will use can be chosen  
  and the network's password can be entered. 
  The SSID and password of the chosen network will be saved in the ESP8266's EEPROM for next 
  time. WiFiManager will then shut down the 'Ship_Bell_Clock' access point, connect to the chosen  
  network and return control to this program.
  */   
  
  WiFiManager wifiManager; 
  wifiManager.setConfigPortalTimeout(180);
  wifiManager.setAPCallback(configModeCallback);            // display red while wating for user input
  //wifiManager.setSTAStaticIPConfig(IPAddress(192, 168, 1, 210), IPAddress(192, 168, 1, 254), IPAddress(255, 255, 255, 0)); // IP, gateway, subnet

  if (!wifiManager.autoConnect("Ship_Bell_Clock")) 
  {
    Serial.println("Failed to connect and reached timeout");
    wait(3000);
    ESP.reset();                                            // reset and try again
    wait(5000);
  }                       
 
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // If we get here, control has been returned from WiFiManager 
  
  // Display magenta when connected  
  fourLeds(MAGENTA);            
  Serial.print("Connected to ");          Serial.println(WiFi.SSID());
  Serial.print("IP number assigned: ");   Serial.println(WiFi.localIP());  
  Serial.print("Gateway: ");              Serial.println(WiFi.gatewayIP()); 
  Serial.print("Netmask: ");              Serial.println(WiFi.subnetMask()); 
  Serial.print("MAC address: ");          Serial.println(WiFi.macAddress());
  Serial.print("Host name: ");            Serial.println(WiFi.hostname());

  Serial.println("Starting NTP");
  wait(1500);  

  // Get UTC time from NTP server
  Serial.println("Starting UDP");                  
  Udp.begin(localPort);
  Serial.print("Local port: "); Serial.println(Udp.localPort());  
  Serial.println("Waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300); 

  // Display cyan when time is set
  fourLeds(CYAN);   
  Serial.println("UTC time set.");
  Serial.println("Setting time zone ...");
  wait(1500); 
     
  // Connect to server and get time zone and geographic information  
  // This must be done after WiFi connection is established 

  if (setTimeZone()) fourLeds(GREEN);       // display green if auto timezone was successful
  else fourLeds(YELLOW);                    // display yellow if default timezone had to be used
  wait(1000);

  // Set up OTA for software updates
  ArduinoOTA.setHostname("Ship_Bell_Clock");
  ArduinoOTA.setPassword("74177");
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");   
  wait(1500); 

  char buff[24];
  display.clear();
  header();  
  display.drawString(10, 21, "Setup");
  display.drawString(10, 32, "completed."); 
  snprintf(buff, 24, "RSSI: %i", WiFi.RSSI());
  display.drawString(10, 43, buff); 
  display.drawString(10, 54, "MAC:");
  display.drawString(30, 54, WiFi.macAddress());
  display.display(); 
  wait(4000);

  FastLED.clear();
  FastLED.show();    
  digitalWrite(BUILT_IN_LED, HIGH);       // start with the 1Hz LED off
  breatheStepTime = 900 / ((BREATHE_HIGH_VALUE - BREATHE_LOW_VALUE) / BREATHE_LEVEL_CHANGE * 2);   
  DF1201S.setVol(CHIME_VOLUME);     
}

/*******************************************************************************************************************************************
* Main loop
********************************************************************************************************************************************/

void loop() 
{ 
  static time_t previous = 0; 
  
  // Check the switch
  functionButton.tick();
  
  
  if (timeStatus() != timeNotSet)
  { 
    if (now() != previous)          // update display only if the time has changed  
    {
      previous = now();

      uint8_t h = analogRead(A0) / 4;   // get hue in 0 to 255 range, analog input comes in 0 to 1023
      if (h > 4) hue = h; 

      local = tz.toLocal(now(), &tcr);
      chimes(chimeFileSet);

      if (isAM(local)) ledArray[4] = CHSV(YELLOW, 255, 150);
      else ledArray[4] = CHSV(109, 255, 150);    // green

      //fourLeds(hue);   // call one or the other but not both!
      breathe.attach_ms(breatheStepTime, sequenceBreathing);

      displayTime();                                                                                                                                                                                                                                                                                                                    
     
      //serialMonitor();       
      //Serial. printf("ADC: %u", h); 
            
      if (h < 5) hue++;             // auto color if pot is fully counter clockwise, manual otherwise  

      digitalWrite(BUILT_IN_LED, LOW);
      wait(2);
      digitalWrite(BUILT_IN_LED, HIGH);
    }      
  }
  
  ArduinoOTA.handle();   
  delay(1); 
  ESP.wdtFeed(); 
} 

/*******************************************************************************************************************************************
* Light the LEDs
********************************************************************************************************************************************/

void fourLeds(uint8_t HSVhue)
{
  for (uint8_t i = 0; i < 4; i++) ledArray[i] = CHSV(HSVhue, 255, 255);  
  FastLED.show(); 
}

void fourLeds(uint8_t r, uint8_t g, uint8_t b)
{
  for (uint8_t i = 0; i < 4; i++) ledArray[i] = CRGB(r, g, b);  
  FastLED.show();
}

void confirmation(uint8_t r, uint8_t g, uint8_t b)
{ 
  // flash the AM/PM LED

  ledArray[2] = CRGB(0, 0, 0);   
  FastLED.show();
  wait(250);
  ledArray[2] = CRGB(r, g, b);   
  FastLED.show();
  wait(250);
  ledArray[2] = CRGB(0, 0, 0);
  FastLED.show();
  wait(250);  
}

/*******************************************************************************************************************************************
* Function called if WiFiManager is in config mode (AP open)
********************************************************************************************************************************************/

void configModeCallback (WiFiManager *myWiFiManager) 
{ 
  Serial.println("Access Point mode. Meter LEDs set to red.");   
  Serial.println(myWiFiManager->getConfigPortalSSID());           // display the AP name  
  fourLeds(255, 0, 0);
}

/*******************************************************************************************************************************************
* Function called when the time changes each second
********************************************************************************************************************************************/

void displayTime()
{
  char buff[24];
  display.clear();

  if (digitalRead(DISPLAY_PIN) == LOW)
  {
    header();  
    snprintf(buff, 16, "%.2i:%.2i:%.2i", hour(local), minute(local), second());
    display.drawString(10, 21, buff);
    display.drawString(10, 32, loc.timezone);
    snprintf(buff, 24, "RSSI: %i", WiFi.RSSI());
    display.drawString(10, 43, buff); 
    IPAddress ip = WiFi.localIP();
    snprintf(buff, 20, "IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    display.drawString(10, 54, buff);
  }
  display.display(); 
}  

/*******************************************************************************************************************************************
 * Serial Monitor Display
********************************************************************************************************************************************/

void serialMonitor()
{   
  char buff[128];

  sprintf(buff, "\n\nUTC: %.2i:%.2i:%.2i  ", hour(), minute(), second());
  Serial.print(buff);

  Serial.print(dayStr(weekday()));   Serial.print(", ");
  Serial.print(monthStr(month()));   Serial.print(" ");
  Serial.print(day());               Serial.print(", ");
  Serial.println(year());   

  Serial.print("Timezone: "); 
  Serial.print(loc.timezone);       Serial.print("  ");
  Serial.print(loc.city);           Serial.print(", "); 
  Serial.print(loc.region);         Serial.print(" "); 
  Serial.print(loc.country);
  Serial.print("  Latitude: ");     Serial.print(loc.latitude); 
  Serial.print("  Longitude: ");    Serial.println(loc.longitude); 
  Serial.print("Offset: ");         Serial.println(loc.offsetSeconds);   
    
  sprintf(buff, "Local: %.2i:%.2i  ", hour(local), minute(local));
  Serial.print(buff); Serial.print(tcr -> abbrev);   Serial.print("  ");
  Serial.print(dayStr(weekday(local)));              Serial.print(", ");
  Serial.print(monthStr(month(local)));              Serial.print(" ");
  Serial.print(day(local));                          Serial.print(", ");
  Serial.println(year(local)); 

  Serial. print("Chime file set: ");  Serial.println(chimeFileSet);  
  Serial. print("Hue:  ");             Serial.println(hue);  
  Serial. print("MAC:  ");             Serial.println(WiFi.macAddress());  
  Serial. print("IP :  ");             Serial.println(WiFi.localIP());  
  Serial. print("RSSI: ");             Serial.println(WiFi.RSSI());   
}

/*******************************************************************************************************************************************
* Set timezone                   
********************************************************************************************************************************************/

bool setTimeZone()
{
  bool status;
  uint8_t count = 1;
  int16_t stdOffsetMinutes;

  loc = geoip.getGeoFromWiFi();  
  
  while (!loc.status && count < 21)
  {
    loc = geoip.getGeoFromWiFi();       
    count++;          
  } 
  
  if (!loc.status)   // time zone information is not valid   
  {
    status = false;   

    loc.offsetSeconds = DEFAULT_UTC_OFFSET;       
    strcpy(loc.city, "Stored default");   
    strcpy(loc.region, "Auto Timezone");
    strcpy(loc.country, "not available");
    loc.latitude = 0.0;
    loc.longitude = 0.0;

    stdOffsetMinutes = loc.offsetSeconds / 60;
    dstRule.offset = stdOffsetMinutes + 60;
    stdRule.offset = stdOffsetMinutes; 
    tz.setRules(dstRule, stdRule);                                                   
  }
  else
  {
    status = true;   
    
    // ipapi.co supplies local UTC offset already adjusted for Daylight Saving Time.
    // However displayAnalogData() function expects Standard Time and it does the adjustment for DST if necessary.
    // This avoids having to call ipapi every day to see if DST has changed.
    // So we adjust the offset from ipapi back to Standard Time if it comes in during DST.

    // First check the current local time

    stdOffsetMinutes = loc.offsetSeconds / 60;
    dstRule.offset = stdOffsetMinutes + 60;
    stdRule.offset = stdOffsetMinutes; 
    tz.setRules(dstRule, stdRule);    
    local = tz.toLocal(now(), &tcr);

    // Then change the offset back to Standard Time if it came in during DST
    
    if (tz.locIsDST(local))
    {
     loc.offsetSeconds -= 3600;   // remove one hour 
     stdOffsetMinutes = loc.offsetSeconds / 60;
     dstRule.offset = stdOffsetMinutes + 60;
     stdRule.offset = stdOffsetMinutes; 
     tz.setRules(dstRule, stdRule); 
    }     
  }

  DEBUG_PRINT("DST:");                 DEBUG_PRINTLN(tz.locIsDST(local)); 
  DEBUG_PRINT("\nTimezone set to ");   DEBUG_PRINTLN(loc.timezone);  
  DEBUG_PRINT("Offset Seconds: ");     DEBUG_PRINTLN(loc.offsetSeconds);  
  DEBUG_PRINT("  Count: ");            DEBUG_PRINTLN(count);   

  return status;
}

/*******************************************************************************************************************************************
* Button click functions                      
********************************************************************************************************************************************/

void buttonDoubleClick()
{
  ;
}

void buttonClick()
{
  chimeFileSet++;
  if (chimeFileSet > fileSetsOnPlayer) chimeFileSet = 1; 
   
  EEPROM.put(0, chimeFileSet);
  EEPROM.commit();
  uint8_t fileSetBase = (chimeFileSet - 1) * 8;
  DF1201S.playFileNum(fileSetBase + 1);    // play single bell to confirm (the first file of the set)  
}  
  

void buttonLongPressStop()
{
  confirmation(4, 255, 255);     // flash AM/PM LED cyan to confirm
}

void buttonLongPressStart()
{
   loc.offsetSeconds = loc.offsetSeconds + (0 * 3600 + 15 * 60);    // saying loc.offsetSeconds += 900 didn't work
   if (loc.offsetSeconds > 50400) loc.offsetSeconds = -43200;       // cannot exceed 14 hours, 0 minutes, must return to -12 hours, 0 minutes
                        
  strcpy(loc.city, "Manually set"); 
  strcpy(loc.region, "reset the ESP");
  strcpy(loc.country, "for Auto TZ");
  loc.latitude = 0.0;
  loc.longitude = 0.0;

  int16_t stdOffsetMinutes = loc.offsetSeconds / 60;
  dstRule.offset = stdOffsetMinutes + 60;
  stdRule.offset = stdOffsetMinutes; 
  tz.setRules(dstRule, stdRule);   
}

/********************************************************************************************************************************************
*  NTP Code that everybody uses                     
********************************************************************************************************************************************/

const int NTP_PACKET_SIZE = 48;           // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE];       // buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP;                  // NTP server's ip address

  while (Udp.parsePacket() > 0) delay(0);          // discard any previously received packets
  Serial.println("Transmit NTP Request");
  
  // get a random server from the pool
  
  if (!WiFi.hostByName(ntpServerName, ntpServerIP))
  {
    IPAddress nist(132, 163, 96, 6);    // use NIST time-e-b as backup
    sendNTPpacket(nist);
    Serial.println("NIST");
  }
  else 
  {
    sendNTPpacket(ntpServerIP);
    Serial.print(ntpServerName);
    Serial.print(": ");
    Serial.println(ntpServerIP);
  }
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) 
  {
    delay(0);
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) 
    {
      Serial.println("Receive NTP Response");       
      Udp.read(packetBuffer, NTP_PACKET_SIZE);    // read packet into the buffer

      // Convert four bytes starting at location 40 to a long integer
      unsigned long secsSince1900;      
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;             // return 0 if unable to get the time
}

// Send an NTP request to the time server at the given address

void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;            // Stratum, or type of clock
  packetBuffer[2] = 6;            // Polling Interval
  packetBuffer[3] = 0xEC;         // Peer Clock Precision
  
  // Bytes 4 to 11 left set to zero for Root Delay & Root Dispersion
  
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  
  // All NTP fields have been given values, now send a packet requesting a timestamp
  
  Udp.beginPacket(address, 123);            //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

/*******************************************************************************************************************************************
* Check time and play appropriate mp3 file -  the function only gets called once each second
********************************************************************************************************************************************/

void chimes(uint8_t fileSet)
{
  uint8_t fileSetBase = (chimeFileSet - 1) * 8;
  uint8_t h12 = hourFormat12(local);

  if(timeInterval(CHIME_START_HOUR, CHIME_END_HOUR))         
  {
    if (minute(local) == 30 && second() == 30 && (h12 == 12 || h12 == 4 || h12 == 8))
    {
      DF1201S.playFileNum(fileSetBase + 1); 
      DEBUG_PRINT("Play one bell, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());  
    }
    if (minute(local) == 0  && second() == 30 && (h12 == 1 || h12 == 5 || h12 == 9))
    {
      DF1201S.playFileNum(fileSetBase + 2); 
      DEBUG_PRINT("Play two bells, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());
    }
    else if (minute(local) == 30  && second() == 30 && (h12 == 1 || h12 == 5 || h12 == 9))
    {
      DF1201S.playFileNum(fileSetBase + 3); 
      DEBUG_PRINT("Play three bells, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());
    }
    else if (minute(local) == 0 && second() == 30 && (h12 == 2 || h12 == 6 || h12 == 10))
    {
      DF1201S.playFileNum(fileSetBase + 4); 
      DEBUG_PRINT("Play four bells, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());
    }
    else if (minute(local) == 30 && second() == 30 && (h12 == 2 || h12 == 6 || h12 == 10))
    {
      DF1201S.playFileNum(fileSetBase + 5); 
      DEBUG_PRINT("Play five bells, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());
    }
    else if (minute(local) == 0 && second() == 30 && (h12 == 3 || h12 == 7 || h12 == 11))
    {
      DF1201S.playFileNum(fileSetBase + 6); 
      DEBUG_PRINT("Play six bells, file no. ");
      Serial.println(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      Serial.println(DF1201S.getFileName());
    }
    else if (minute(local) == 30 && second() == 30 && (h12 == 3 || h12 == 7 || h12 == 11))
    {
      DF1201S.playFileNum(fileSetBase + 7); 
      DEBUG_PRINT("Play seven bells, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());
    }
    else if (minute(local) == 0 && second() == 30 && (h12 == 4 || h12 == 8 || h12 == 12))
    {
      DF1201S.playFileNum(fileSetBase + 8); 
      DEBUG_PRINT("Play eight bells, file no. ");
      DEBUG_PRINTLN(DF1201S.getCurFileNumber());
      DEBUG_PRINT("Name of the currently-playing file: ");
      DEBUG_PRINTLN(DF1201S.getFileName());
    }            
  }
}

/*******************************************************************************************************************************************
* Returns true if hour is between start and end, bridges midnight if necessary                      
********************************************************************************************************************************************/

bool timeInterval(uint8_t start, uint8_t end) 
{
  bool state = false;      

  if (start == end) state = false;

  if (start < end)
  {
    if (hour(local) >= start && hour(local) < end) state = true; 
    else if (hour(local) >= end) state = false;
    else state = false;
  }

  if (start > end)
  {  
    if (hour(local) >= start && hour(local) <= 23) state = true;  
    else if (hour(local) < end) state = true;
    else if (hour(local) >= end && hour(local) < start) state = false;
  }
  return state;
}  

/*******************************************************************************************************************************************
* Breathing colon sequencer                       
********************************************************************************************************************************************/

void sequenceBreathing()
{  
  static bool decrease = false;                        // initial values when first called
  static uint16_t level = BREATHE_LOW_VALUE;

  //Serial.println(level);
  
  ledArray[0] = CHSV(hue, 255, level);
  ledArray[1] = CHSV(hue, 255, level);
  ledArray[2] = CHSV(hue, 255, level);
  ledArray[3] = CHSV(hue, 255, level);
  FastLED.show();    

  if ((level < BREATHE_HIGH_VALUE) && !decrease) level += BREATHE_LEVEL_CHANGE;      // increase the brightness
  else decrease = true;                                                              // increasing finished, time to decrease
  
  if (decrease) level -= BREATHE_LEVEL_CHANGE;                                       // decrease the brightness  
  if (decrease && (level < BREATHE_LOW_VALUE))             // sequence done, turn off timer and reset for next second
  {                                               
    breathe.detach();             
    decrease = false;                 
    level = BREATHE_LOW_VALUE;                              
  }
  delay(0);                 // yield to background functions  
}

/*******************************************************************************************************************************************
* Player extra functions, not called, it's just here for reference                        
********************************************************************************************************************************************/

void playerFunctions()
{
  // Set baud rate to 115200 (Need to power off and restart, power-down save)
  // DF1201S.setBaudRate(115200);
  // Turn on indicator LED (Power-down save)
  // DF1201S.setLED(true);
  // Turn on the prompt tone (Power-down save) 
  // DF1201S.setPrompt(true);
  // Enable amplifier chip 
  // DF1201S.enableAMP();
  // Disable amplifier chip 
  // DF1201S.disableAMP();
  Serial.println("Start playing");
  // Start playing
  DF1201S.start();
  delay(3000);
  Serial.println("Pause");
  // Pause
  DF1201S.pause();
  delay(3000);
  Serial.println("Next");
  // Play the next song
  DF1201S.next();
  delay(3000);
  Serial.println("Previous");
  // Play the previous song
  DF1201S.last();
  delay(3000);
  Serial.println("Start playing");
  // Fast forward 10S
  DF1201S.fastForward(10);
  // Fast Rewind 10S
  DF1201S.fastReverse(10);
  // Start the song from the 10th second 
  DF1201S.setPlayTime(10);  
  Serial.print("File number:");
  // Get file number
  Serial.println(DF1201S.getCurFileNumber());  
  Serial.print("The number of files available to play:");
  // The number of files available to play
  Serial.println(DF1201S.getTotalFile());  
  Serial.print("The time length the current song has played:");
  // Get the time length the current song has played 
  Serial.println(DF1201S.getCurTime());  
  Serial.print("The total length of the currently-playing song: ");
  // Get the total length of the currently-playing song 
  Serial.println(DF1201S.getTotalTime());
  Serial.print("The name of the currently-playing file: ");
  // Get the name of the playing file 
  Serial.println(DF1201S.getFileName());
  delay(3000);
  // Play the file No.1, the numbers are arranged according to the sequence of the files copied into the U-disk 
  DF1201S.playFileNum(1);
  // Play the test.mp3 file in test folder 
  DF1201S.playSpecFile("/test/test.mp3");  
  // Delete the currently-playing file 
  DF1201S.delCurFile();
} 

/*******************************************************************************************************************************************
* OLED header                      
********************************************************************************************************************************************/

void header()
{
  char buffer[36];
  sprintf(buffer, "Ship's Bell Clock v%i.%i%i", VERSION_MAJOR, VERSION_MINOR, VERSION_BUILD);

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(5, 0, buffer);
  display.drawLine(0, 13, 128, 13);  
}

/*******************************************************************************************************************************************
* End                        
********************************************************************************************************************************************/
