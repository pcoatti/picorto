//code by Paolo Coatti
 #if !( defined(ARDUINO_RASPBERRY_PI_PICO_W) )
  	#error For RASPBERRY_PI_PICO_W only
 #endif

 #define _ASYNCTCP_RP2040W_LOGLEVEL_     4
 #define _RP2040W_AWS_LOGLEVEL_          3

 ///////////////////////////////////////////////////////////////////

 #include <pico/cyw43_arch.h>

 ///////////////////////////////////////////////////////////////////

 #include <AsyncWebServer_RP2040W.h>
 #include <Wire.h>
 #include <LittleFS.h>

 #include <Adafruit_INA219.h>

 Adafruit_INA219 ina219;

 // Date and time functions using a DS3231 RTC connected via I2C and Wire lib
 
 #include "RTClib.h"

 RTC_DS3231 rtc;

 int status = WL_IDLE_STATUS;
 AsyncWebServer    server(80);
 int Rele = 16;
 int WaterLvl = 17;
 int WebPage = 0;
 int LastWaterLvl = -1;
 unsigned long StartTime;
 long TempoErogazione;  // Tempo azionamento pompa in millisecondi
 unsigned long LastTime;
 const long interval = 1000;
 char countryCode[3] = { 0, 0, 0 };

 typedef struct
 {
   int StartHour;
   int StartMin;
   int PumpTime;
   char ssid[21];
   char pass[21];
 } configuration_type;

 // loaded with DEFAULT values!
 configuration_type CONFIG = {
   20,
   30,
   15,
   "PICORTO",
   "cambiare"
 }; 
 char fileConfig[] = "config.bin";
 char fileErogazioni[] = "erogazioni.log";


 void LoadConfig() {
   char s1[128];
   configuration_type conf;

   File bin = LittleFS.open(fileConfig, "r");
  
   if (bin) {
     Serial.println(" => Open config.bin OK");
   }
   else {
     Serial.println(" => Open config.bin Failed");
     return;
   }

   if (bin.read((uint8_t *) &conf, sizeof(conf)) == 0) {
     Serial.println(" => config.bin invalid file length");
     return;
   }
   sprintf(s1, "config.bin inizio orario=%02d:%02d durata=%ld SSID=%s, password=%s", conf.StartHour, conf.StartMin, conf.PumpTime, conf.ssid, conf.pass);
   Serial.println(s1);
   memcpy(&CONFIG, &conf, sizeof(CONFIG));
   bin.close();
 }

 void SaveConfig() {
   File bin = LittleFS.open(fileConfig, "w");
  
   if (bin) {
     Serial.println(" => Open config.bin OK");
   }
   else {
     Serial.println(" => Open config.bin Failed");
     return;
   }
 
   if (bin.write((uint8_t *) &CONFIG, sizeof(CONFIG))) {
     Serial.println("* Writing config.bin OK");
   } 
   else {
     Serial.println("* Writing config.bin failed");
   }
  
   bin.close();
 }

 void appendFile(const char * message) {
   File log = LittleFS.open(fileErogazioni, "a");
  
   if (log) {
     Serial.println(" => Open erogazioni.log OK");
   }
   else {
     Serial.println(" => Open erogazioni.log Failed");
     return;
   }

   if (log.write((uint8_t *) message, strlen(message))) {
     Serial.println("* Appending erogazioni OK");
   } 
   else {
     Serial.println("* Appending erogazioni failed");
   }
   
   log.close();
 }

 const String htmlHome =
   "<html><head>\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 1.5rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO Web Server</h1>\
   <p>DATA: <strong>{DateTime} </strong></p>\
   <p>LIVELLO ACQUA: <strong>{WaterStatus}</strong></p>\
   <p><a href=\"/?status\"><button class=\"button\">STATO</button></a></p>\
   <p><a href=\"/?setup\"><button class=\"button button2\">SETUP</button></a></p>\
   <p><a href=\"/?log\"><button class=\"button\">STORICO</button></a></p>\
   <p><a href=\"/?rtc\"><button class=\"button button2\">OROLOGIO</button></a></p>\
   </body></html>";

 const String htmlStatus =
   "<html><head>\
   <meta http-equiv=\"refresh\" content=\"3\">\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 1.5rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO STATUS</h1>\
   <p>DATA: <strong>{DateTime} </strong></p>\
   <p>TEMP: <strong>{Temperature}</strong></p>\
   <p>STATO POMPA: <strong>{ReleStatus}</strong></p>\
   <p>LIVELLO ACQUA: <strong>{WaterStatus}</strong></p>\
   <p>TENSIONE: <strong>{Voltage}</strong></p>\
   <p>CORRENTE: <strong>{Current}</strong></p>\
   <p><a href=\"/?back\"><button class=\"button\">INDIETRO</button></a></p>\
   </body></html>";

const String htmlSetup =
   "<html><head>\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 1.0rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO SETUP</h1>\
   <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postsetup/\">\
   <p>\
   ORA INIZIO: <input type=\"text\" name=\"StartHour\"value=\"{StartHour}\" size=\"2\" maxlength=\"2\"><br>\
   MINUTI: <input type=\"text\" name=\"StartMin\"value=\"{StartMin}\" size=\"2\" maxlength=\"2\"><br>\
   TEMPO ACC.: <input type=\"text\" name=\"PumpTime\"value=\"{PumpTime}\" size=\"2\" maxlength=\"2\"><br>\
   SSID: <input type=\"text\" name=\"SSID\"value=\"{SSID}\" size=\"20\" maxlength=\"20\"><br>\
   Password: <input type=\"password\" name=\"password\"value=\"{password}\" size=\"20\" maxlength=\"20\"><br>\
   <button type=\"submit\" class=\"button\">CONFERMA</button>\
   </p></form>\
   <p><a href=\"/?back\"><button class=\"button button2\">INDIETRO</button></a></p>\
   </body></html>";
//   <input type=\"submit\" value=\"Submit\">\

const String htmlSetupCheck =
   "<html><head>\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 1.5rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO SETUP</h1>\
   <p>SETUP <strong>{Result} </strong></p>\
   <p><a href=\"/?back\"><button class=\"button\">INDIETRO</button></a></p>\
   </body></html>";

const String htmlLog =
   "<html><head>\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 0.8rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO STORICO</h1>\
   {Storico}\
   <p><a href=\"/?back\"><button class=\"button\">INDIETRO</button></a></p>\
   </body></html>";

const String htmlRtc =
   "<html><head>\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 1.0rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO OROLOGIO</h1>\
   <form method=\"post\" enctype=\"application/x-www-form-urlencoded\" action=\"/postrtc/\">\
   <p>\
   DATA: <input type=\"date\" name=\"Date\"value=\"{Date}\"><br>\
   ORA: <input type=\"time\" name=\"Time\"value=\"{Time}\"><br>\
   <button type=\"submit\" class=\"button\">CONFERMA</button>\
   </p></form>\
   <p><a href=\"/?back\"><button class=\"button button2\">INDIETRO</button></a></p>\
   </body></html>";

const String htmlRtcCheck =
   "<html><head>\
   <title>PICORTO Web Server</title>\
   <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
   <link rel=\"icon\" href=\"data:,\">\
   <style>\
   html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}\
   h1{color: #0F3376; padding: 2vh;}\
   p{font-size: 1.5rem;}\
   .button{display: inline-block; background-color: #e7bd3b; border: none; border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}\
   .button2{background-color: #4286f4;}\
   </style>\
   </head>\
   <body> <h1>PICORTO OROLOGIO</h1>\
   <p>SETUP <strong>{Result} </strong></p>\
   <p><a href=\"/?back\"><button class=\"button\">INDIETRO</button></a></p>\
   </body></html>";

void handleRoot(AsyncWebServerRequest *request)
{
   String htmlResp, curDateTime, Temperature, ReleStatus, WaterStatus, Voltage, Current, StartHour, StartMin, PumpTime, SSID, password;
   DateTime now;
   char s1[128];
   float shuntvoltage = 0;
   float busvoltage = 0;
   float current_mA = 0;
   float loadvoltage = 0;
   float power_mW = 0;

   now = rtc.now();
   sprintf(s1, "%02d/%02d/%04d %02d:%02d:%02d", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
   curDateTime = s1;
   sprintf(s1, "%.02f", rtc.getTemperature());
   Temperature = s1;
   if (digitalRead(Rele) == 1)
     ReleStatus = "ACCESA";
   else
     ReleStatus = "SPENTA";
   if (digitalRead(WaterLvl) == 1)
     WaterStatus = "KO";
   else
     WaterStatus = "OK";

   shuntvoltage = ina219.getShuntVoltage_mV();
   busvoltage = ina219.getBusVoltage_V();
   current_mA = ina219.getCurrent_mA();
   power_mW = ina219.getPower_mW();
   loadvoltage = busvoltage + (shuntvoltage / 1000);

   sprintf(s1, "%.02f V", loadvoltage);
   Voltage = s1;
   sprintf(s1, "%.02f mA", current_mA);
   Current = s1;
   sprintf(s1, "%d", CONFIG.StartHour);
   StartHour = s1;
   sprintf(s1, "%d", CONFIG.StartMin);
   StartMin = s1;
   sprintf(s1, "%d", CONFIG.PumpTime);
   PumpTime = s1;
   sprintf(s1, "%s", CONFIG.ssid);
   SSID = s1;
   sprintf(s1, "%s", CONFIG.pass);
   password = s1;

   Serial.print("URI: ");
	 Serial.println(request->url());
	 Serial.print("Method: ");
	 Serial.println((request->method() == HTTP_GET) ? "GET" : "POST");
	 Serial.print("Arguments: ");
	 Serial.println(request->args());

	 for (uint8_t i = 0; i < request->args(); i++) {
		Serial.print(request->argName(i));
    Serial.println(request->arg(i));
	 }

   WebPage = 0;
   if (request->args() >= 1) {
      if(request->argName(0) == "status")
        WebPage = 1;
      if(request->argName(0) == "setup")
        WebPage = 2;
      if(request->argName(0) == "log")
        WebPage = 3;
      if(request->argName(0) == "rtc")
        WebPage = 4;
   }

   if (WebPage == 0) {
     htmlResp = htmlHome;
     htmlResp.replace("{DateTime}", curDateTime);
     htmlResp.replace("{WaterStatus}", WaterStatus);
   }
   else if (WebPage == 1) {
     htmlResp = htmlStatus;
     htmlResp.replace("{DateTime}", curDateTime);
     htmlResp.replace("{Temperature}", Temperature);
     htmlResp.replace("{ReleStatus}", ReleStatus);
     htmlResp.replace("{WaterStatus}", WaterStatus);
     htmlResp.replace("{Voltage}", Voltage);
     htmlResp.replace("{Current}", Current);
   }
   else if (WebPage == 2) {
     htmlResp = htmlSetup;
     htmlResp.replace("{StartHour}", StartHour);
     htmlResp.replace("{StartMin}", StartMin);
     htmlResp.replace("{PumpTime}", PumpTime);
     htmlResp.replace("{SSID}", SSID);
     htmlResp.replace("{password}", password);
   }
   else if (WebPage == 3) {
     String Storico;
     Storico = "";
     File log = LittleFS.open(fileErogazioni, "r");
  
     if (log) {
       Serial.println(" => Open erogazioni.log OK");
       while (log.available()) {
         sprintf(s1, "<p>%s</p>", log.readStringUntil('\n').c_str());
         Storico = Storico + s1;
       }
       log.close();
     }
     else {
       Serial.println(" => Open erogazioni.log Failed");
       Storico = "<p>STORICO EROGAZIONI ASSENTE</p>";
     }
     htmlResp = htmlLog;
     htmlResp.replace("{Storico}", Storico);
   }
   else if (WebPage == 4) {
     htmlResp = htmlRtc;
     htmlResp.replace("{Date}", "");
     htmlResp.replace("{Time}", "");
   }
	 request->send(200, "text/html", htmlResp);
}

void handleSetup(AsyncWebServerRequest *request)
{
  String htmlResp;
  configuration_type conf;
  int data_ok = 1;

	if (request->method() != HTTP_POST)
	{
		request->send(405, "text/plain", "Method Not Allowed");
	}
	else
	{
    String Result;

    Serial.print("URI: ");
	  Serial.println(request->url());
	  Serial.print("Method: ");
	  Serial.println((request->method() == HTTP_GET) ? "GET" : "POST");
	  Serial.print("Arguments: ");
	  Serial.println(request->args());

    memset(&conf, 0, sizeof(conf));
	  for (uint8_t i = 0; i < request->args(); i++) {
		 Serial.print(request->argName(i));
     Serial.println(request->arg(i));
     if (request->argName(i) == "StartHour")
       conf.StartHour = request->arg(i).toInt();
     if (request->argName(i) == "StartMin")
       conf.StartMin = request->arg(i).toInt();
     if (request->argName(i) == "PumpTime")
       conf.PumpTime = request->arg(i).toInt();
     if (request->argName(i) == "SSID" && request->arg(i).length() <= 20)
       strcpy(conf.ssid, request->arg(i).c_str());
     if (request->argName(i) == "password" && request->arg(i).length() <= 20)
       strcpy(conf.pass, request->arg(i).c_str());
	  }
    if (conf.StartHour < 0 || conf.StartHour > 23) {
      Result = "FALLITO ORA INIZIO NON VALIDA";
      data_ok = 0;
    }
    if (conf.StartMin < 0 || conf.StartMin > 59) {
      Result = "FALLITO MINUTO INIZIO NON VALIDO";
      data_ok = 0;
    }
    if (conf.PumpTime < 1 || conf.PumpTime > 30) {
      Result = "FALLITO TEMPO EROGAZIONE NON VALIDO";
      data_ok = 0;
    }
    if (strlen(conf.ssid) < 4) {
      Result = "FALLITO SSID NON VALIDO";
      data_ok = 0;
    }
    if (strlen(conf.pass) < 8) {
      Result = "FALLITO PASSWORD NON VALIDA";
      data_ok = 0;
    }
    if (data_ok == 1) {
      Result = "SUCCESSO";
      memcpy(&CONFIG, &conf, sizeof(CONFIG));
      SaveConfig();
    }
    htmlResp = htmlSetupCheck;
    htmlResp.replace("{Result}", Result);
	  request->send(200, "text/html", htmlResp);
	}
}

void handleRtc(AsyncWebServerRequest *request)
{
  String htmlResp;
  int data_ok = 1;

	if (request->method() != HTTP_POST)
	{
		request->send(405, "text/plain", "Method Not Allowed");
	}
	else
	{
    String Result, date, time;
    char s1[80];

    Serial.print("URI: ");
	  Serial.println(request->url());
	  Serial.print("Method: ");
	  Serial.println((request->method() == HTTP_GET) ? "GET" : "POST");
	  Serial.print("Arguments: ");
	  Serial.println(request->args());

    date = "";
    time = "";
	  for (uint8_t i = 0; i < request->args(); i++) {
		 Serial.print(request->argName(i));
     Serial.println(request->arg(i));
     if (request->argName(i) == "Date" && request->arg(i).length() == 10)
       date = request->arg(i);
     if (request->argName(i) == "Time" && request->arg(i).length() == 5)
       time = request->arg(i);
	  }
    if (date.length() == 0) {
      data_ok = 0;
      Result = "DATA NON VALIDA";
    }
    if (time.length() == 0) {
      data_ok = 0;
      Result = "ORA NON VALIDA";
    }
    if (data_ok == 1) {
      int yy, mm, dd, hh, mn;
      yy = date.substring(0,4).toInt();
      mm = date.substring(5,7).toInt();
      dd = date.substring(8,10).toInt();
      hh = time.substring(0,2).toInt();
      mn = time.substring(3,5).toInt();
      sprintf(s1, "New date %02d/%02d/%04d %02d:%02d:00", dd, mm, yy, hh, mn);
      Serial.println(s1);
      rtc.adjust(DateTime(yy, mm, dd, hh, mn, 0));
      Result = "SUCCESSO";
    }
    htmlResp = htmlRtcCheck;
    htmlResp.replace("{Result}", Result);
	  request->send(200, "text/html", htmlResp);
	}
}

void handleNotFound(AsyncWebServerRequest *request)
{
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += request->url();
	message += "\nMethod: ";
	message += (request->method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += request->args();
	message += "\n";

	for (uint8_t i = 0; i < request->args(); i++)
	{
		message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
	}

	request->send(404, "text/plain", message);
}

 void setup() {
   //Initialize serial and wait for port to open:
   Serial.begin(115200);

   while (!Serial && millis() < 5000);

   delay(200);

   Serial.println("Access Point Web Server");
 
   pinMode(LED_BUILTIN, OUTPUT);
   pinMode(Rele, OUTPUT);
   pinMode(WaterLvl, INPUT_PULLUP);
   digitalWrite(LED_BUILTIN, LOW);
   digitalWrite(Rele, LOW);
 
   // if (!LittleFS.format()) {
   //   Serial.printf("Unable to format(), aborting\n");
   //   digitalWrite(LED_BUILTIN, HIGH);
   //   while (1) delay(10);
   // }
   if (!LittleFS.begin()) {
     Serial.printf("Unable to begin(), aborting\n");
     digitalWrite(LED_BUILTIN, HIGH);
     while (1) delay(10);
   }
   
   LoadConfig();

   if (! rtc.begin()) {
     Serial.println("Couldn't find RTC");
     Serial.flush();
     digitalWrite(LED_BUILTIN, HIGH);
     while (1) delay(10);
   }

   if (rtc.lostPower()) {
     Serial.println("RTC lost power, let's set the time!");
     // When time needs to be set on a new device, or after a power loss, the
     // following line sets the RTC to the date & time this sketch was compiled
     // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
     // This line sets the RTC with an explicit date & time, for example to set
     // January 21, 2014 at 3am you would call:
     // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
   }

   // Initialize the INA219.
   // By default the initialization will use the largest range (32V, 2A).  However
   // you can call a setCalibration function to change this range (see comments).
   if (! ina219.begin()) {
     Serial.println("Failed to find INA219 chip");
     digitalWrite(LED_BUILTIN, HIGH);
     while (1) { delay(10); }
   }
   // To use a slightly lower 32V, 1A range (higher precision on amps):
   //ina219.setCalibration_32V_1A();
   
   // check for the WiFi module:
   if (WiFi.status() == WL_NO_MODULE) {
     Serial.println("Communication with WiFi module failed!");
     digitalWrite(LED_BUILTIN, HIGH);
     // don't continue
     while (true);
   }
 
   // print the network name (SSID);
   Serial.print("Creating access point named: ");
   Serial.println(CONFIG.ssid);
 
   // Create open network. Change this line if you want to create an WEP network:
   status = WiFi.beginAP(CONFIG.ssid, CONFIG.pass);
   if (status != WL_CONNECTED) {
     Serial.println("Creating access point failed");
     digitalWrite(LED_BUILTIN, HIGH);
     // don't continue
     while (true);
   }
 
   // wait 10 seconds for connection:
   delay(10000);
 
   // you're connected now, so print out the status
   printWiFiStatus();

   server.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
	{
		handleRoot(request);
	});

 	//server.on("/postsetup/", handleForm);
	server.on("/postsetup/", HTTP_POST, [](AsyncWebServerRequest * request)
	{
		handleSetup(request);
	});

 	//server.on("/postrtc/", handleForm);
	server.on("/postrtc/", HTTP_POST, [](AsyncWebServerRequest * request)
	{
		handleRtc(request);
	});

	server.onNotFound(handleNotFound);

	server.begin();
 }
 
 void heartBeatPrint()
{
	static int num = 1;

	Serial.print(F("."));

	if (num == 80)
	{
		Serial.println();
		num = 1;
	}
	else if (num++ % 10 == 0)
	{
		Serial.print(F(" "));
	}
}

void check_status()
{
	static unsigned long checkstatus_timeout = 0;

#define STATUS_CHECK_INTERVAL     10000L

	// Send status report every STATUS_REPORT_INTERVAL (60) seconds: we don't need to send updates frequently if there is no status change.
	if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
	{
		heartBeatPrint();
		checkstatus_timeout = millis() + STATUS_CHECK_INTERVAL;
	}
}

void loop() {
  unsigned long  CurTime = millis();
  char mess[80];
  DateTime now;
  if (CurTime - LastTime >= interval) {
    LastTime = millis();
    if (LastWaterLvl != digitalRead(WaterLvl)) {
      if (digitalRead(WaterLvl) == 1) {
        digitalWrite(LED_BUILTIN, HIGH);              
        if (digitalRead(Rele) == 1) {
          digitalWrite(Rele, 0);
          now  = rtc.now();
          sprintf(mess, "[ERROR] %02d/%02d/%04d %02d:%02d:%02d WATER LEVEL\n", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second());
          appendFile(mess);
        }                
      }
      else {
        digitalWrite(LED_BUILTIN, LOW);              
      }
      LastWaterLvl = digitalRead(WaterLvl);
    }
    if (digitalRead(Rele) == 1) {
      if ((CurTime - StartTime) >= TempoErogazione) {
        digitalWrite(Rele, 0);
        now  = rtc.now();
        sprintf(mess, "[INFO] %02d/%02d/%04d %02d:%02d:%02d EROGAZIONE OK %d MINUTI\n", now.day(), now.month(), now.year(), now.hour(), now.minute(), now.second(), CONFIG.PumpTime);
        appendFile(mess);
      }
    }
    DateTime now = rtc.now();
    if (digitalRead(WaterLvl) == 0 && digitalRead(Rele) == 0 && now.hour() == CONFIG.StartHour && now.minute() == CONFIG.StartMin) {
      digitalWrite(Rele, 1);
      StartTime = millis();
      TempoErogazione = (long) CONFIG.PumpTime * 60L * 1000L;
    } 
  }
  check_status();
}
 
 void printWiFiStatus() {
   // print the SSID of the network you're attached to:
   Serial.print("SSID: ");
   Serial.println(WiFi.SSID());
 
   // print your WiFi shield's IP address:
   IPAddress ip = WiFi.localIP();
   Serial.print("IP Address: ");
   Serial.println(ip);
 
   // print where to go in a browser:
   Serial.print("To see this page in action, open a browser to http://");
   Serial.println(ip);
   // print your board's country code
	 // #define CYW43_COUNTRY(A, B, REV) ((unsigned char)(A) | ((unsigned char)(B) << 8) | ((REV) << 16))
	 uint32_t myCountryCode = cyw43_arch_get_country_code();

	 countryCode[0] = myCountryCode & 0xFF;
	 countryCode[1] = (myCountryCode >> 8) & 0xFF;

	 Serial.print("Country code: ");
	 Serial.println(countryCode);
 } 
