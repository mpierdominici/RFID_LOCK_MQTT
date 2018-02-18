//***********************************//
//Modulo cerradura rfid              //
//                                   //
//Matias Pierdominici                //
//mpierdominici@itba.edu.ar          //
//***********************************//
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         D3
#define SS_PIN          D4

MFRC522 rfidCardReader(SS_PIN, RST_PIN);




#define DEBUGG

#define SAMPLE_TIME  1

#define SEC_TO_MILISEC(x) ((x)*1000) 
#define LOCK_PIN D2
#define PUSH_BUTTON_PIN D1
#define RED_LED_PIN D0
#define GREEN_LED_PIN D8




class myTimer
{
  public:
  myTimer(unsigned int seconds=0);
  bool timeOver(void);
  void setNewTime(unsigned long seconds_);
  void showInfo();
  
  unsigned long seconds;
  unsigned long startTime;
  void resetTimer(void);
    
};



class waterBomb
{
  public:
  waterBomb(unsigned int pin_);
  void onn(){digitalWrite(pin,HIGH);};
  void off(){digitalWrite(pin,LOW);};
  private:
  unsigned int pin;
  
};


waterBomb lock(LOCK_PIN);
waterBomb redLed(RED_LED_PIN);
waterBomb greenLed(GREEN_LED_PIN);

char * ssid ="WIFI Pier";
char * pass ="pagle736pagle";
unsigned int mqttPort=1883;

const char MqttUser[]="lock";
const char MqttPassword[]="1234";
const char MqttClientID[]="lock";

IPAddress mqttServer(192,168,0,116);

WiFiClient wclient;
PubSubClient mqtt_client(wclient);






void callback(char* topic, byte* payload, unsigned int length);
void  debug_message (char * string, bool newLine)
{
#ifdef DEBUGG
  if(string !=NULL)
  {
    if (!newLine)
    {
      Serial.print(string);
    }else
    {
      Serial.println(string);
    }
  }
  #endif
}

void setUpWifi(char * ssid, char * pass)
{
  String ip;
  debug_message(" ",true);
  debug_message(" ",true);
  debug_message("Conectandose a: ",false);
  debug_message(ssid,true);

  WiFi.begin(ssid,pass);

  while(WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    debug_message(".",false);
  }
  debug_message(" ",true);
  debug_message("Coneccion realizada",true);
  debug_message("La ip es: ",false);
  ip=WiFi.localIP().toString();
  debug_message((char *)ip.c_str(),true);
}

void setUpMqtt(void)
{
  mqtt_client.setServer(mqttServer,mqttPort);
  mqtt_client.setCallback(callback);
}


void callback(char* topic, byte* payload, unsigned int length)
{
  int tiempo=0;
  payload[length]='\n';
  String message((char *)payload);
  debug_message("Llego un mensage, topic:",false);
  debug_message(topic,false);
  debug_message(", payload : ",false);
  debug_message((char *)payload,true);

  if(!strcmp(topic,"lock/open"))
  {
    lock.onn();
    redLed.off();
    greenLed.onn();   
  }
  else if(!strcmp(topic,"lock/close"))
  {
    lock.off();
    redLed.onn();
    greenLed.off(); 
  }

}

void reconnect()
{
  while(!mqtt_client.connected())
  {
    debug_message("Intentando conectar al servidor MQTT",true);
    if (mqtt_client.connect(MqttClientID,MqttUser,MqttPassword))
      {
            debug_message("conectado",true);
  
  
            // ...suscrivirse a topicos
            mqtt_client.subscribe("lock/open");
            mqtt_client.subscribe("lock/close");
         
            


      }
      else
      {
        debug_message("intentando conetarse al broker",true);
        delay(3000);
      }
  }
}

void setup() {
  Serial.begin(9600);
  setUpWifi(ssid,pass);
  setUpMqtt();
  pinMode(PUSH_BUTTON_PIN,INPUT);
  redLed.onn();
  SPI.begin();
  rfidCardReader.PCD_Init(); //inicio el lector rfid

}





myTimer readTime(SAMPLE_TIME);
 String arrivedUid = "";
 int buttonState=LOW;
 bool realease=false;
void loop() {
  if (!mqtt_client.connected()) 
  {
      reconnect();
      redLed.onn();
      greenLed.off();
      lock.off();
      
 }
 mqtt_client.loop();
  
  if(readTime.timeOver())
  {
    readTime.resetTimer();
    

    if (rfidCardReader.PICC_IsNewCardPresent())
    {
    
      if (rfidCardReader.PICC_ReadCardSerial())
      {
      
        for (byte i = 0; i < rfidCardReader.uid.size; i++)
        {
          arrivedUid.concat(String(rfidCardReader.uid.uidByte[i] < 0x10 ? " 0" : " "));
          arrivedUid.concat(String(rfidCardReader.uid.uidByte[i], HEX));
        }
        arrivedUid.toUpperCase();
        mqtt_client.publish("lock/uid",(String(arrivedUid)).c_str());
        arrivedUid="";

      }
    }
  }
 buttonState=digitalRead(PUSH_BUTTON_PIN);
  if(buttonState==HIGH)
  {
    debug_message("button presionado",true);
    realease=true;
  }else if(realease)
  {
    realease=false;
    debug_message("se envio press",true);
    mqtt_client.publish("lock/button","press");
  }

}





//***********************TIMER**********************************



myTimer::myTimer(unsigned int seconds)
{
  setNewTime(seconds);
}

//timeOver
//devuelve true si ya paso el tiempo seteado,
//caso contrario devuelve false
//
bool myTimer::timeOver(void)
{
  if((millis())>startTime)
  {
    resetTimer();
    return true;
  }
  else
  {
    return false;
  }
}

void myTimer::resetTimer(void)
{
  unsigned long temp=seconds+millis();
 
  startTime=temp;
  //Serial.print("se llamo a rest timer con: ");
  //Serial.println(startTime);
}

void  myTimer::setNewTime(unsigned long seconds_)
{
  unsigned long temp=1000*seconds_;
  //Serial.println(temp);
  seconds=temp;
 
  //Serial.print("s seteo un timer cada: ");
  //Serial.print(seconds_);
  //Serial.print(" se registro un tirmpo de: ");
  //Serial.println(seconds/1000);
  resetTimer();

}

void myTimer::showInfo()
{
  //Serial.println(startTime);
  unsigned long dif=startTime-millis();
  //Serial.print("Remaining time (seconds):");
  //Serial.println(dif/1000);
  //Serial.println(startTime);
  //Serial.println(millis());
  //Serial.println(seconds/1000);
}

//*********************BOMBA DE AGUA********************

waterBomb::waterBomb(unsigned int pin_)
{
  pin=pin_;
  pinMode(pin,OUTPUT);
  off();
}


