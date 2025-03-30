
///////////////////////////////////////////////////////////////
//
//
//     CONTROLADOR DE ACUARIO ANAKINO AQUARIUM 
//     

String SemV = "24.02.00";  // VERSION DEL FIRMWARE

//           PLACA ESP32
//               
//
///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////
//Librerias necesarias
///////////////////////////////////////////////////////////////

#include <ESPUI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <NTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include "FS.h"
#include <esp32fota.h>
////////////////////////////////////////////////////////////////

// esp32fota esp32fota("<Type of Firmware for this device>", <this version>, <validate signature>, <allow insecure https>);
esp32FOTA esp32FOTA("esp32-fota-http", SemV, false, true);
const char* manifest_url = "https://raw.githubusercontent.com/Atorcha/Anakino_Aquarium_ESPUI/main/OTA/fota.json";

////////////////////////////////////////////////////////////////

  //#define DEBUG
    #define CONTADOR
    int contador_1 = 0;   // variable contador del loop
    int contador_2 = 0; // contador para restart en caso de no wifi
   
////////////////////////////////////////////////////////////////
// Definimos los pines y variables
////////////////////////////////////////////////////////////////
// mejores pines OUTPUT 32,33,25,26,27,14,23,22,21,19,18,5,17
// mejores pines INPUT 36,39,34,35,32,33,25,26,27,14,23,22,21,19,18
#define sensores_temp  13  //  Sensores de temp de agua y pantalla leds
#define calentador 17      // Calentador    * ENCHUFE 1         
#define leds   21           // luz      * ENCHUFE 2
#define aireador 19          //     * ENCHUFE 3
#define rele 21             //      * ENCHUFE 4 el pin 18 si lo pongo high da error

//*********************** Variables de control de temperatura del agua ********************

float temp_agua;             //Temperatura del agua
float temp_agua_des;         //temperatura agua deseada
byte contador_temp = 0;
float temperatura_agua_temp;       // Temperatura temporal del agua
///////////////////////////////////////////////////////////

byte modo_luz;      // modo de funcionamiento luz
bool luz;    // Variable para indicar si activa o no la luz
byte luz_on_hora;       // Horario para encender leds.
byte luz_on_minuto;
byte luz_off_hora;      // Horario para apagar leds.
byte luz_off_minuto;
////////////////////////////////////////////////////////////
int modo_ai;      // modo de funcionamiento ai
bool ai;    // Variable para indicar si activa o no la ai
int ai_on_hora;       // Horario para encender ai.
int ai_on_minuto;
int ai_off_hora;      // Horario para apagar ai.
int ai_off_minuto;

uint16_t tempHBLabelId, humedadHBLabelId, aguatempId, RSSItempId, versionLabelId;
uint16_t realtime_LabelId;
uint16_t boton_param, boton_aire, boton_restart, boton_ver;
uint16_t text_time1, text_time2, text_time_ai1, text_time_ai2;
char timeString[9];

//UI handles
uint16_t wifi_ssid_text, wifi_pass_text;
char userLogin_text;
char passLogin_text;
uint16_t mainLabel, gruposreles, Switch_4, Switch_3, Switch_2, Switch_1, mainSlider, mainText, mainScrambleButton, mainTime;
uint16_t styleButton, styleLabel, styleSwitcher, styleSlider, styleButton2, styleLabel2, styleSlider2;
float mainNumber;

// Variables to save date and time
String realtime;
String luz_on_temp;
String luz_off_temp;
String ai_on_temp;
String ai_off_temp;


unsigned char th; // tiempo en hora
unsigned char tm; // tiempo en minuto
/*
int on1_hora; /// temporizador 1 hora ON
int on1_minuto; //// temporizador 1 minuto ON
int off1_hora; /// temporizador 1 hora OFF
int off1_minuto; //// temporizador 1 minuto OFF
int on2_hora; /// temporizador 2 hora ON
int on2_minuto; //// temporizador 2 minuto ON
int off2_hora; /// temporizador 2 hora OFF
int off2_minuto; //// temporizador 2 minuto OFF
*/

 String  userLogin;
 String  passLogin;

////////////////////////////////////////////////////////////////
   
////////////////////////////////////////////////////////////////


Preferences nvs;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String ssid;
String password;
bool modo_wifi_cliente;//  si modo cliente = true checkea la conexion wifi para restart ESP32

AsyncWebServer server(8080);
//unsigned long previousMillis = 0;
unsigned long reinicio = 0;
//unsigned long interval = 30000; 

String hostname = "ESP32_Anakino";

/////////////////////////////// FOTA .   /////////////////

bool MUST_UPDATE = false;
                
////////////////////////////////////////////////////////////////
//ARRANCA  EL SENSOR DE TEMP DEL AGUA 
///////////////////////////////////////////////////////////////

OneWire oneWire(sensores_temp);      //Sensores de temperatura conectados al pin 22.
DallasTemperature sensors(&oneWire);
//DeviceAddress sensor_agua;

void textCallback(Control *sender, int type) {
  //This callback is needed to handle the changed values, even though it doesn't do anything itself.
}

///////////////////////////////////////////////////////////
//
//              CARDS
//
/////////////////////////////////////////////////////////////

//   PESTAÑA  TEMP AGUA 
  void temperatura_Callback(Control *sender, int type) 
  {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  temp_agua_des = (sender->value).toFloat();
  Serial.print("Temp agua deseada: ");
  Serial.println(temp_agua_des);
}


/////////////////   CARD LUZ   ///////////////////////////////
void selectCall(Control* sender, int value) // MODO LUZ
{
    Serial.print("Select: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
    if (String(sender->value)==("MODO AUTO")) {(modo_luz = 0);}
    if (String(sender->value)==("MODO ON")) {(modo_luz = 1);}
    if (String(sender->value)==("MODO OFF")) {(modo_luz = 2);}
    Serial.print("Modo Luz: ");
    Serial.println(modo_luz);
}

void luz_on_Callback(Control *sender, int type) // HORA ON LUZ
{
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  //Convert the hours. Rely on the fact that it will stop converting when it hits the :
  luz_on_hora = sender->value.toInt();
  //Look for the : 
 char *mins_str = strstr(sender->value.c_str(), ":");
 //strstr returns a pointer to where the : was found. Increment past it.
 mins_str += 1;
 //And finally convert the rest of the string.
 luz_on_minuto = String(mins_str).toInt();
 Serial.print("Luz ON numMins: ");
 Serial.println(NumMins(luz_on_hora,luz_on_minuto));
}

void luz_off_Callback(Control *sender, int type) // HORA OFF LUZ
   {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
    //Convert the hours. Rely on the fact that it will stop converting when it hits the :
    luz_off_hora = sender->value.toInt();
    //Look for the : 
   char *mins_str = strstr(sender->value.c_str(), ":");
   //strstr returns a pointer to where the : was found. Increment past it.
   mins_str += 1;
   //And finally convert the rest of the string.
   luz_off_minuto = String(mins_str).toInt();
   Serial.print("Luz OFF NumMins: ");
   Serial.println(NumMins(luz_off_hora,luz_off_minuto));
   }

/////////////////   CARD AIREADOR   ///////////////////////////////
void selectCall_2(Control* sender, int value) // MODO AIREADOR
{
    Serial.print("Select: ID: ");
    Serial.print(sender->id);
    Serial.print(", Value: ");
    Serial.println(sender->value);
    if (String(sender->value)==("MODO AUTO")) {(modo_ai = 0);}
    if (String(sender->value)==("MODO ON")) {(modo_ai = 1);}
    if (String(sender->value)==("MODO OFF")) {(modo_ai = 2);}
    Serial.print("Modo ai: ");
    Serial.println(modo_ai);
}

void ai_on_Callback(Control *sender, int type) // HORARIO aireador
{
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
  //Convert the hours. Rely on the fact that it will stop converting when it hits the :
  ai_on_hora = sender->value.toInt();
  //Look for the : 
 char *mins_str = strstr(sender->value.c_str(), ":");
 //strstr returns a pointer to where the : was found. Increment past it.
 mins_str += 1;
 //And finally convert the rest of the string.
 ai_on_minuto = String(mins_str).toInt();
 Serial.print("Ai ON numMins: ");
 Serial.println(NumMins(ai_on_hora,ai_on_minuto));
}

void ai_off_Callback(Control *sender, int type) 
   {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
    //Convert the hours. Rely on the fact that it will stop converting when it hits the :
    ai_off_hora = sender->value.toInt();
    //Look for the : 
   char *mins_str = strstr(sender->value.c_str(), ":");
   //strstr returns a pointer to where the : was found. Increment past it.
   mins_str += 1;
   //And finally convert the rest of the string.
   ai_off_minuto = String(mins_str).toInt();
   Serial.print("ai OFF NumMins: ");
   Serial.println(NumMins(ai_off_hora,ai_off_minuto));
   }

void boton_param_Callback(Control* sender, int type)
{
    switch (type)
    {
    case B_DOWN:
       // Serial.println("Button DOWN");
        break;

    case B_UP:
        Serial.println("Button UP, graba parametros");
        SAVEparametrosNVS();
        break;
    }
}


/////////////////   BOTON UPDATE   ///////////////////////////////

void boton_ver_Callback(Control* sender, int type)
{
    switch (type)
    {
    case B_DOWN:
        //Serial.println("Button DOWN");
        break;

    case B_UP:
    
    Serial.println("Check update");  // Check version firmware    
    bool updatedNeeded = esp32FOTA.execHTTPcheck(); 
    if (updatedNeeded)
      {
        nvs.putBool("must_update", true);
        Serial.println("Necesita actualizar...y reinicia");
        delay (3000);
        ESP.restart(); // Restart ESP32     
      } 
        Serial.println("NO necesita actualizar...");
        break;
    }
}

/////////////////   BOTON RESTART   ///////////////////////////////

void boton_restart_Callback(Control* sender, int type)
{
    switch (type)
    {
    case B_DOWN:
        //Serial.println("Button RESTART DOWN");
        break;

    case B_UP:
        Serial.println("Boton reinicio presionado");
        ESP.restart(); // Restart ESP
        break;
    }
}

//Most elements in this test UI are assigned this generic callback which prints some
//basic information. Event types are defined in ESPUI.h






void generalCallback(Control *sender, int type) {
  Serial.print("CB: id(");
  Serial.print(sender->id);
  Serial.print(") Type(");
  Serial.print(type);
  Serial.print(") '");
  Serial.print(sender->label);
  Serial.print("' = ");
  Serial.println(sender->value);
}




void numberCall(Control* sender, int type)
{
    Serial.println(sender->value);
}
/*
void padExample(Control* sender, int value)
{
    switch (value)
    {
    case P_LEFT_DOWN:
        Serial.print("left down");
        break;

    case P_LEFT_UP:
        Serial.print("left up");
        break;

    case P_RIGHT_DOWN:
        Serial.print("right down");
        break;

    case P_RIGHT_UP:
        Serial.print("right up");
        break;

    case P_FOR_DOWN:
        Serial.print("for down");
        break;

    case P_FOR_UP:
        Serial.print("for up");
        break;

    case P_BACK_DOWN:
        Serial.print("back down");
        break;

    case P_BACK_UP:
        Serial.print("back up");
        break;

    case P_CENTER_DOWN:
        Serial.print("center down");
        break;

    case P_CENTER_UP:
        Serial.print("center up");
        break;
    }

    Serial.print(" ");
    Serial.println(sender->id);
}
*/
void enterWifiDetailsCallback(Control *sender, int type) {
  if(type == B_UP) {
    Serial.println("Saving credentials to NVS...");
    Serial.println(ESPUI.getControl(wifi_ssid_text)->value);
    Serial.println(ESPUI.getControl(wifi_pass_text)->value);

    nvs.putString("ssid", (ESPUI.getControl(wifi_ssid_text)->value)); 
    nvs.putString("password", (ESPUI.getControl(wifi_pass_text)->value));
    Serial.println("Network Credentials Saved using Preferences");
    nvs.end();
    ESP.restart(); // Restart ESP
  }
}

void enterLoginDetailsCallback(Control *sender, int type) {
  if(type == B_UP) {
    Serial.println("Saving User login to NVS...");
    Serial.println(ESPUI.getControl(userLogin_text)->value);
    Serial.println(ESPUI.getControl(passLogin_text)->value);

    nvs.putString("user", (ESPUI.getControl(userLogin_text)->value)); 
    nvs.putString("pass", (ESPUI.getControl(passLogin_text)->value));
    Serial.println("Login Credentials Saved using Preferences");
    nvs.end();
    ESP.restart(); // Restart ESP
  }
}

///////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////
// FUNCION PARA PASAR LAS HORAS A MINUTOS Y ASI PODER GESTIONAR MEJOR LOS TEMPORIZADORES
int NumMins(uint8_t ScheduleHour, uint8_t ScheduleMinute)
{
  return (ScheduleHour*60) + ScheduleMinute;
}


///////////////////////////////////////////////////////////

     /////////////////////////////////////
    ///////////////             /////////
   //////////////// PESTAÑAS    ////////
  /////////////////             ///////
 /////////////////////////////////////

void setupUI(){
  
  String clearLabelStyle = "background-color: unset; width: 100%;";
  
  /*
   * Tab: STATUS
   * This tab contains all the basic ESPUI controls, and shows how to read and update them at runtime.
   *-----------------------------------------------------------------------------------------------------------*/
   
    auto status_tab = ESPUI.addControl(Tab, "", "Status");

  String labelStyle = "background-color: unset; width: 20%;";
  String inputStyle = "width: 70%; text-align: center;";

  auto estado = ESPUI.addControl(Label, "Estado", "Agua ºC", Peterriver, status_tab);
   ESPUI.setElementStyle(estado, labelStyle);
   ESPUI.setElementStyle(aguatempId = ESPUI.addControl(Label, "", "", Peterriver, estado, generalCallback), inputStyle);

   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Hora", None, estado), labelStyle);
   ESPUI.setElementStyle(realtime_LabelId = ESPUI.addControl(Label, "", "", Peterriver, estado, generalCallback), inputStyle);

   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "RSSI", None, estado), labelStyle);
   ESPUI.setElementStyle(RSSItempId = ESPUI.addControl(Label, "", "", Peterriver, estado, generalCallback), inputStyle);

    // group switchers. AQUI IRA EL ESTADO DE LOS RELES   
  auto gruposreles = 
   ESPUI.addControl(Switcher, "Estados RELES", "0", Peterriver, status_tab, generalCallback);
   Switch_2 = ESPUI.addControl(Switcher, "", "", Sunflower, gruposreles, generalCallback);// aireador
   Switch_3 = ESPUI.addControl(Switcher, "", "", Sunflower, gruposreles, generalCallback);//calentador
   Switch_4 = ESPUI.addControl(Switcher, "", "", Sunflower, gruposreles, generalCallback);//led
  
   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "", None, gruposreles), clearLabelStyle);
   //We will now need another label style. This one sets its width to the same as a switcher (and turns off the background)
   String switcherLabelStyle = "width: 60px; margin-left: .3rem; margin-right: .3rem; background-color: unset;";
   //We can now just add the styled labels.
   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "1", None, gruposreles), switcherLabelStyle);
   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "AIR", None, gruposreles), switcherLabelStyle);
   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "HEAT", None, gruposreles), switcherLabelStyle);
   ESPUI.setElementStyle(ESPUI.addControl(Label, "", "LED", None, gruposreles), switcherLabelStyle);
    
  /*
   * Tab: PARÁMETROS
   * This tab contains all the basic ESPUI controls, and shows how to read and update them at runtime.
   *-----------------------------------------------------------------------------------------------------------*/
    auto param_tab = ESPUI.addControl(Tab, "", "Parámetros");
    
    // ESPUI.separator("Configuracion agua"); 
    ESPUI.addControl(ControlType::Separator, "Configuracion agua", "", ControlColor::None, param_tab); //CONFIGURACION AGUA
    
    // INPUT para la temperatura del agua Number inputs also accept Min and Max components, but you should still validate the values.
    mainNumber = ESPUI.addControl(Number, "Temperatura agua deseada", "24.5", Peterriver, param_tab, temperatura_Callback);
    ESPUI.addControl(Step, "", "0.1", None, mainNumber);
    ESPUI.addControl(Min, "", "20", None, mainNumber);
    ESPUI.addControl(Max, "", "30", None, mainNumber);

   // ESPUI.separator("Configuracion luz"); 
    ESPUI.addControl(ControlType::Separator, "Configuracion luz", "", ControlColor::None, param_tab); //MODO FUNCIONAMIENTO LUZ
    
  auto select1 = ESPUI.addControl(ControlType::Label, "LUZ", "Modo", ControlColor::Peterriver, param_tab);
    ESPUI.setElementStyle(select1, labelStyle);
    auto select1b = ESPUI.addControl(ControlType::Select, "", "", ControlColor::Peterriver, select1, selectCall);
      ESPUI.setElementStyle(select1b, inputStyle);
      ESPUI.setElementStyle(ESPUI.addControl(ControlType::Option, "MODO AUTO", "MODO AUTO", ControlColor::Peterriver, select1b), inputStyle);
      ESPUI.setElementStyle(ESPUI.addControl(ControlType::Option, "MODO ON", "MODO ON", ControlColor::Peterriver, select1b), inputStyle);
      ESPUI.setElementStyle(ESPUI.addControl(ControlType::Option, "MODO OFF", "MODO OFF", ControlColor::Peterriver, select1b), inputStyle);
     
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Hora ON", Peterriver, select1), labelStyle);   // INTRODUCCION HORA ENCENDIDA/APAGADA
    ESPUI.setElementStyle(text_time1 = ESPUI.addControl(Text, "", luz_on_temp, Peterriver, select1, luz_on_Callback), inputStyle); // INPUT ON DEL PRIMER TIMER
    ESPUI.setInputType(text_time1, "time");
    
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Hora OFF", Peterriver, select1), labelStyle); 
    ESPUI.setElementStyle(text_time2 = ESPUI.addControl(Text,"", luz_off_temp, Peterriver, select1, luz_off_Callback), inputStyle); // INPUT OFF DEL PRIMER TIMER
    ESPUI.setInputType(text_time2, "time");

  //  ESPUI.separator("Configuracion aireador"); 
    ESPUI.addControl(Separator, "Configuracion aireador", "", None, param_tab); //MODO FUNCIONAMIENTO AIREADOR

    auto select2 = ESPUI.addControl(ControlType::Label, "AIREADOR", "Modo", ControlColor::Peterriver, param_tab);
      ESPUI.setElementStyle(select2, labelStyle);
      auto select2b = ESPUI.addControl(ControlType::Select, "", "", ControlColor::Peterriver, select2, selectCall_2);
        ESPUI.setElementStyle(select2b, inputStyle);
        ESPUI.setElementStyle(ESPUI.addControl(ControlType::Option, "MODO AUTO", "MODO AUTO", ControlColor::Peterriver, select2b), inputStyle);
        ESPUI.setElementStyle(ESPUI.addControl(ControlType::Option, "MODO ON", "MODO ON", ControlColor::Peterriver, select2b), inputStyle);  
        ESPUI.setElementStyle(ESPUI.addControl(ControlType::Option, "MODO OFF", "MODO OFF", ControlColor::Peterriver, select2b), inputStyle);
        
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Hora ON", Peterriver, select2), labelStyle);   // INTRODUCCION HORA ENCENDIDA/APAGADA
      ESPUI.setElementStyle(text_time_ai1 = ESPUI.addControl(Text, "", ai_on_temp, Peterriver, select2, ai_on_Callback), inputStyle); // INPUT ON DEL PRIMER TIMER
      ESPUI.setInputType(text_time_ai1, "time");
      
      ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Hora OFF", Peterriver, select2), labelStyle); 
      ESPUI.setElementStyle(text_time_ai2 = ESPUI.addControl(Text,"", ai_off_temp, Peterriver, select2, ai_off_Callback), inputStyle); // INPUT OFF DEL PRIMER TIMER
      ESPUI.setInputType(text_time_ai2, "time");

    // BOTON GRABAR EN EEPROM
    boton_param = ESPUI.addControl(ControlType::Button, "GRABAR", "Press", ControlColor::Alizarin, param_tab, &boton_param_Callback);

         
  /*
   * Tab: CONFIGURACION
   * You use this tab to enter the SSID and password of a wifi network to autoconnect to.
   *-----------------------------------------------------------------------------------------------------------*/
  ///////////////////////// UPDATE WIFI
  auto config_tab = ESPUI.addControl(Tab, "", "Configuracion");
  // WIFI
  ESPUI.addControl(ControlType::Separator, "Configurar WIFI", "", ControlColor::None, config_tab);
  wifi_ssid_text = ESPUI.addControl(Text, "SSID", "", Peterriver, config_tab, textCallback);
  //Note that adding a "Max" control to a text control sets the max length
  ESPUI.addControl(Max, "", "32", None, wifi_ssid_text);
  wifi_pass_text = ESPUI.addControl(Text, "Password", "", Peterriver, config_tab, textCallback);
  ESPUI.addControl(Max, "", "64", None, wifi_pass_text);
  ESPUI.addControl(Button, "Guardar", "Guardar", Alizarin, config_tab, enterWifiDetailsCallback);

  ////////////////////////// UPDATE LOGIN
  ESPUI.addControl(ControlType::Separator, "Configurar login", "", ControlColor::None, config_tab);
  userLogin_text = ESPUI.addControl(Text, "Usuario", "", Peterriver, config_tab, textCallback);
  //Note that adding a "Max" control to a text control sets the max length
  passLogin_text = ESPUI.addControl(Text, "Contraseña", "", Peterriver, config_tab, textCallback);
  ESPUI.addControl(Button, "Guardar", "Guardar", Alizarin, config_tab, enterLoginDetailsCallback);
  
  /////////////////////////// UPDATE FIRM
  ESPUI.addControl(ControlType::Separator, "Actualizar firmware", "", ControlColor::None, config_tab);

  // BOTON COMPROBAR VERSION
    boton_ver = ESPUI.addControl(ControlType::Button, "Actualizar Firmware", "Update", ControlColor::Alizarin, config_tab, &boton_ver_Callback);
    versionLabelId = ESPUI.addControl(Label, "Version Firmware ", "", ControlColor::Peterriver, config_tab); // version firmware

      /////////////////////////// RESET
  ESPUI.addControl(ControlType::Separator, "REINCIAR", "", ControlColor::None, config_tab);
    boton_restart = ESPUI.addControl(ControlType::Button, "REINICIAR ESP32", "Press", ControlColor::Alizarin, config_tab, &boton_restart_Callback);
   
    // Enable this option if you want sliders to be continuous (update during move) and not discrete (update on stop)
    // ESPUI.sliderContinuous = true;

    /*
     * Optionally you can use HTTP BasicAuth. Keep in mind that this is NOT a
     * SECURE way of limiting access.
     * Anyone who is able to sniff traffic will be able to intercept your password
     * since it is transmitted in cleartext. Just add a string as username and
     * password, for example begin("ESPUI Control", "username", "password")
     */   
}
////////////////////////////////////////////////////////////////
// VOID SETUP
///////////////////////////////////////////////////////////////

void setup(void)
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    esp32FOTA.setManifestURL( manifest_url );
    esp32FOTA.printConfig();  
    sensors.begin();
    nvs.begin("datos",false); // use "datos" namespace
    READfromNVS();
    READLoginfromNVS();  
    WiFi.onEvent(WiFiStationDisconnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);  
    connectWIFI();          
    if (MUST_UPDATE == true) {
        nvs.putBool("must_update", false);
        Serial.println("Ha reiniciado ...y empieza actualizacion");
        esp32FOTA.execHTTPcheck();       
        esp32FOTA.execOTA();
        delay (2000);
    }
    
  //  ESPUI.setVerbosity(Verbosity::Verbose); //Turn ON verbose debugging
    timeClient.begin(); // Initialize a NTPClient to get time
    timeClient.setTimeOffset(3600); // Set offset time in seconds to adjust for your timezone, for example:
    // GMT +1 = 3600
    timeClient.update();

///////////////////////////////////////
    //Asignamos los distintos PINES

  pinMode(calentador, OUTPUT); // Calentador     
  pinMode(luz, OUTPUT);
  pinMode(aireador, OUTPUT);
  pinMode(rele, OUTPUT);

  
/////////////////////////////////////////////////

    check_time();
    
    
    Serial.print("\nHora actual:");
    Serial.println(timeClient.getFormattedTime());
    Serial.print("Temp deseada: ");
    Serial.println(temp_agua_des);
    //Serial.println(NumMins(th,tm));
    Serial.print("Hora LUZ ON: ");
    //Serial.println(NumMins(luz_on_hora,luz_on_minuto));
    luz_on_temp = String() + (luz_on_hora < 10 ? "0" : "") + luz_on_hora + ':' + (luz_on_minuto < 10 ? "0" : "") + luz_on_minuto;
    Serial.println(luz_on_temp);
    Serial.print("Hora LUZ OFF: ");
    //Serial.println(NumMins(luz_off_hora,luz_off_minuto));
    luz_off_temp = String() + (luz_off_hora < 10 ? "0" : "") + luz_off_hora + ':' + (luz_off_minuto < 10 ? "0" : "") + luz_off_minuto;
    Serial.println(luz_off_temp);
    Serial.print("Modo Luz: ");
    Serial.println(modo_luz);

    Serial.print("Hora Aireador ON: ");
    //Serial.println(NumMins(luz_on_hora,luz_on_minuto));
    ai_on_temp = String() + (ai_on_hora < 10 ? "0" : "") + ai_on_hora + ':' + (ai_on_minuto < 10 ? "0" : "") + ai_on_minuto;
    Serial.println(ai_on_temp);
    Serial.print("Hora Aireador OFF: ");
    //Serial.println(NumMins(luz_off_hora,luz_off_minuto));
    ai_off_temp = String() + (ai_off_hora < 10 ? "0" : "") + ai_off_hora + ':' + (ai_off_minuto < 10 ? "0" : "") + ai_off_minuto;
    Serial.println(ai_off_temp);
    Serial.print("Modo Aireador: ");
    Serial.println(modo_ai);
    Serial.println(userLogin.c_str()); 
    Serial.println(passLogin.c_str());
    
    
  setupUI();
  ESPUI.begin("Anakino Aquarium", userLogin.c_str(), passLogin.c_str());
  ESPUI.updateLabel(versionLabelId, String (SemV));
  //ESPUI.updateLabel(estadotempId, String (estado));
}

/////////////////////////////////////////////////////////////////
//   TEMPERATURA DEL AGUA
/////////////////////////////////////////////////////////////////

void check_temp(){ // sensor de temp del agua
  
  contador_temp ++;
  sensors.requestTemperatures();   // call sensors.requestTemperatures() to issue a global 
  // temperature request to all devices on the bus
  
  temperatura_agua_temp += sensors.getTempCByIndex(0);       // lee temp del agua
  if(contador_temp == 10){
    temp_agua = temperatura_agua_temp / 10;
    contador_temp = 0;  
    temperatura_agua_temp = 0;
  }
 }
  
/////////////////////////////////////////////////////////////////
//   CALENTADOR
/////////////////////////////////////////////////////////////////

void check_calentador(){           // activa calentador
    
  #ifdef DEBUG
  Serial.println("check calentador");
  #endif
  
    if (temp_agua > temp_agua_des + 0.2 || temp_agua < temp_agua_des - 0.2)  {
      //alarma_activa =true;
      }  
      else { 
        //alarma_activa = false;
        }
  
    if (temp_agua != -127 && temp_agua != 85 && temp_agua != 0){
     
      if (temp_agua <= temp_agua_des-0.1){
        digitalWrite(calentador,HIGH); // Encendemos Calentador
        ESPUI.updateSwitcher(Switch_3, true);
        // Serial.print("Enciende calentador: "); Serial.print(temp_agua); Serial.print(" "); Serial.println(temp_agua_des-temperatura_margen); 
       }
       
      if (temp_agua >= temp_agua_des+0.1){
        digitalWrite(calentador,LOW); // Apagamos Calentador
        ESPUI.updateSwitcher(Switch_3, false);
       // Serial.print("Apaga calentador"+ String(temp_agua) +"Temp agua + margen= " + String(temp_agua_des+temperatura_margen));
      }
    }
    else
    {
      digitalWrite(calentador,LOW); // Apagamos Calentador
      ESPUI.updateSwitcher(Switch_3, false);
    } 
}


/////////////////////////////////////////////////////////////////
//   VENTILADOR
/////////////////////////////////////////////////////////////////

//**********************************************************************************************
//************************ Funciones Non Volatile Storage **************************************
//**********************************************************************************************

void SAVEparametrosNVS()
{
  nvs.putFloat("temp_agua_des", (temp_agua_des));
  nvs.putInt("modo_luz", modo_luz);
  nvs.putInt("luz_on_hora", luz_on_hora);
  nvs.putInt("luz_on_minuto", luz_on_minuto);
  nvs.putInt("luz_off_hora", luz_off_hora);
  nvs.putInt("luz_off_minuto", luz_off_minuto);
  nvs.putInt("modo_ai", modo_ai);
  nvs.putInt("ai_on_hora", ai_on_hora);
  nvs.putInt("ai_on_minuto", ai_on_minuto);
  nvs.putInt("ai_off_hora", ai_off_hora);
  nvs.putInt("ai_off_minuto", ai_off_minuto);
  nvs.end(); // Close the Preferences
  Serial.println("Graba parámetros en NVS");
}

void SAVEwifitoNVS(){
  nvs.putString("ssid", ssid);
  nvs.putString("password", password);
  nvs.end(); // Close the Preferences
}

void SAVElogintoNVS(){
  nvs.putString("user", userLogin);
  nvs.putString("pass", passLogin);
  nvs.end(); // Close the Preferences
}

void READfromNVS()
{
  temp_agua_des=nvs.getFloat("temp_agua_des", 0);
  luz_on_hora=nvs.getInt("luz_on_hora", 0);
  luz_on_minuto=nvs.getInt("luz_on_minuto", 0);
  luz_off_hora=nvs.getInt("luz_off_hora", 0);
  luz_off_minuto=nvs.getInt("luz_off_minuto", 0);
  modo_luz=nvs.getInt("modo_luz", 0);
  ai_on_hora=nvs.getInt("ai_on_hora", 0);
  ai_on_minuto=nvs.getInt("ai_on_minuto", 0);
  ai_off_hora=nvs.getInt("ai_off_hora", 0);
  ai_off_minuto=nvs.getInt("ai_off_minuto", 0);
  modo_ai=nvs.getInt("modo_ai", 0);
  MUST_UPDATE = nvs.getBool("must_update", MUST_UPDATE);
}

void READLoginfromNVS()
{
  userLogin = nvs.getString("user", "");
  passLogin = nvs.getString("pass", "");
  
  if (userLogin == "" || passLogin == ""){
    userLogin = "Anakin";
    passLogin = "1234";
  }
}

/////////////////////////////////////////////////////////////////
//LUZ
/////////////////////////////////////////////////////////////////

void check_luz(){

      if (modo_luz == 0){       // si esta configurado el modo automatico
        
       if (luz == true ) // si en hora
       {
        digitalWrite(leds,HIGH); //activa rele
        ESPUI.updateSwitcher(Switch_4, true);
        //Serial.println("LUZ Auto ON");
      }
      if (luz == false ) // si no en hora
      {
        digitalWrite(leds,LOW); //desactiva rele
        ESPUI.updateSwitcher(Switch_4, false);
       // Serial.println("LUZ Auto OFF");
      }
    }

   else if (modo_luz == 1) // Modo manual ON
    {
    digitalWrite(leds,HIGH); // Encendemos luz,
    ESPUI.updateSwitcher(Switch_4, true); 
   // Serial.println("LUZ Manual ON ");
    }
    else if   
      (modo_luz == 2)   // Modo manual OFF
      {  
      digitalWrite(leds,LOW); // Apagamos luz
      ESPUI.updateSwitcher(Switch_4, false);
    //  Serial.println("LUZ Manual OFF ");
    }
   }

/////////////////////////////////////////////////////////////////
// TEMPORIZADOR LUZ
/////////////////////////////////////////////////////////////////

void tempo_luz(){               ///////////////////   temporizador 1 (LUZ)
       
    if(NumMins(luz_off_hora,luz_off_minuto) > NumMins(luz_on_hora,luz_on_minuto))
    {       
      if((NumMins(th,tm) >= NumMins(luz_on_hora,luz_on_minuto)) && (NumMins(th,tm) <= NumMins(luz_off_hora,luz_off_minuto)))
      {
          luz = true; // activa rele
      }
      if (NumMins(th,tm) > NumMins(luz_off_hora,luz_off_minuto))
      {
         luz = false; // desactiva rele
      }
    }
    if(NumMins(luz_off_hora,luz_off_minuto) < NumMins(luz_on_hora,luz_on_minuto))
    {
     // Serial.println(" Temp 2");                     
      if(NumMins(th,tm) >= NumMins(luz_on_hora,luz_on_minuto)) 
      {
         luz = true;  // activa rele       
      }

      if (NumMins(th,tm) < NumMins(luz_off_hora,luz_off_minuto)) 
      {
          luz = true;  //  ACTIVA RELE         
      }     
      if ((NumMins(th,tm) >= NumMins(luz_off_hora,luz_off_minuto)) && (NumMins(th,tm) < NumMins(luz_on_hora,luz_on_minuto)))
      {
          luz = false; // desactiva rele
      } 
    }  
 }

 
////////////////////////////////////////////////////////////////
// VOID LOOP
///////////////////////////////////////////////////////////////

void loop()
{     
   reinicio = millis();
   if (reinicio/1000/60/60 == 24 ){
    Serial.print("Reinicio diario");
    ESP.restart(); // Restart ESP
    }

   check_time();
   
   #ifdef CONTADOR 
   contador_1 ++;
   switch (contador_1){

     case 100:
    // #ifdef DEBUG Serial.println("contador 100"); #endif  
    check_temp();        // Comprueba valores temperatura  
    check_calentador();  // Comprueba si activa el calentador
    ESPUI.updateLabel(aguatempId, String (temp_agua)); 
    //estado = "Running...";
    //ESPUI.updateLabel(estadotempId, String ("Running..."));     
    break;
     
     case 200:
     tempo_luz(); // comprueba temporizadores
     check_luz();
     break;

     case 300:
     tempo_ai();
     check_ai();
     ESPUI.updateLabel(RSSItempId, String (WiFi.RSSI()));     
     timeClient.update();
     break;
     
     case 1000:
     contador_1 = 0;
     //ESPUI.updateLabel(estadoId, String ("Running..."));
     break;
   }
   #endif
   
   /*if (modo_wifi_cliente == true){
   unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    contador_2++;
    if (contador_2 == 5) {   
    ESP.restart(); // Restart ESP
    previousMillis = currentMillis;
    }
   }
  }
  */
}


 
  void connectWIFI(){  // Connect to Wi-Fi // try to connect to existing network
    
  int connect_timeout;  
  WiFi.setHostname(hostname.c_str()); //define hostname
  Serial.println("Begin wifi...");
  ssid = nvs.getString("ssid", "");
  password = nvs.getString("password", "");
  
  if (ssid == "" || password == ""){
    
  modo_wifi_cliente = false;   
  Serial.println("No values saved for ssid or password");
  Serial.println("\nCreating access point...");
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP("Anakino_control");

    connect_timeout = 20;
    do {
      delay(250);
      Serial.print(",");
      connect_timeout--;
    } 
    while(connect_timeout);
  }
  
////////// MODO CLIENTE . ///////////////////////////////////////
  
  else {
  modo_wifi_cliente= true;
  Serial.println("Modo Cliente");
  Serial.println("\nTry to connect to: ");
  WiFi.setHostname(hostname.c_str()); //define hostname
  WiFi.mode(WIFI_STA);
  delay(2000);
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println(ssid.c_str());
  Serial.println(password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);   
      Serial.print('.');
      if (contador_2 == 60) {
          ESP.restart(); // Restart ESP
      }
   contador_2++; 
  delay(1000);
  }
    Serial.print("Connected to AP");
    Serial.print("RRSI: ");
    Serial.println(WiFi.RSSI());  
}
    Serial.print("Modo: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? "Station" : "Client");
    Serial.print("IP address: ");
    Serial.println(WiFi.getMode() == WIFI_AP ? WiFi.softAPIP() : WiFi.localIP());
    delay(1000);
  
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  WiFi.begin(ssid.c_str(), password.c_str());    
    contador_2++;
    if (contador_2 == 5) {   
    ESP.restart(); // Restart ESP
    }
}


//////////////////////////////////////////////////
//////////////   CHECK TIME     /////////////////
/////////////////////////////////////////////////

void check_time()
{
    realtime = timeClient.getFormattedTime();
    ESPUI.updateLabel(realtime_LabelId, realtime);
    th = timeClient.getHours(); // asigna a th la hora
    tm = timeClient.getMinutes(); // asigna a tm el minuto  
}


/////////////////////////////////////////////////////////////////
// control funcionamiento del aireador
/////////////////////////////////////////////////////////////////

void check_ai(){

      if (modo_ai == 0)      // modo aireador en AUTO
      {       
        if (ai == true ) // si en hora
        {
          digitalWrite(aireador,HIGH); //activa rele
          ESPUI.updateSwitcher(Switch_2, true);      
          //Serial.println(" AI Auto ON"); 
                  
        }
      else if (ai == false) // si no en hora
      { 
        digitalWrite(aireador,LOW); //desactiva rele
        ESPUI.updateSwitcher(Switch_2, false);       
        //Serial.println("AI Auto OFF");
      }
    }
      
   else if (modo_ai == 1) // modo manual ON
    {
      digitalWrite(aireador,HIGH); //activa rele   
      ESPUI.updateSwitcher(Switch_2, true);
    //Serial.println("AI Manual ON");  
      }
    else if   
      (modo_ai == 2) // MODO MANUAL OFF
      {
      digitalWrite(aireador,LOW); // Apagamos  ai,
      ESPUI.updateSwitcher(Switch_2, false);
     // Serial.println("AI Manual OFF");  
         
    }    
   }
/////////////////////////////////////////////////////////////////
// TEMPORIZADOR AIREADOR
/////////////////////////////////////////////////////////////////

void tempo_ai(){
       
    if(NumMins(luz_off_hora,luz_off_minuto) > NumMins(ai_on_hora,ai_on_minuto))
    {       
      if((NumMins(th,tm) >= NumMins(ai_on_hora,ai_on_minuto)) && (NumMins(th,tm) <= NumMins(ai_off_hora,ai_off_minuto)))
      {
          ai = true;
          
         // Serial.println(" ai true");
        //SetRele(temporizador1, HIGH);        // activa rele
      }
      if (NumMins(th,tm) > NumMins(ai_off_hora,ai_off_minuto))
      {
         ai = false;
        // ESPUI.updateSwitcher(Switch_4, false);
        // Serial.println(" ai false");
       //SetRele(temporizador1, LOW);       // desactiva rele
      }
    }
    if(NumMins(ai_off_hora,ai_off_minuto) < NumMins(ai_on_hora,ai_on_minuto))
    {
     // Serial.println(" ai 2");                     
      if(NumMins(th,tm) >= NumMins(ai_on_hora,ai_on_minuto)) 
      {
         ai = true;
        // ESPUI.updateSwitcher(Switch_4, true);
       //  Serial.println(" ai true");
       //SetRele(temporizador1, HIGH);          // activa rele
      }

      if (NumMins(th,tm) < NumMins(ai_off_hora,ai_off_minuto)) 
      {
          ai = true;
        //  ESPUI.updateSwitcher(Switch_4, true);
        //  Serial.println(" ai true");
        //SetRele(temporizador1, HIGH);          //  ACTIVA RELE
      }     
      if ((NumMins(th,tm) >= NumMins(ai_off_hora,ai_off_minuto)) && (NumMins(th,tm) < NumMins(ai_on_hora,ai_on_minuto)))
      {
          ai = false;
         // ESPUI.updateSwitcher(Switch_4, false);
          //Serial.println(" ai false");
        //SetRele(temporizador1, LOW);         // desactiva rele
      } 
    }  
 }
