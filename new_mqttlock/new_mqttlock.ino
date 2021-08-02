//***********************************//
//Modulo cerradura rfid              //
//                                   //
//Matias Pierdominici                //
//mpierdominici@itba.edu.ar          //
//***********************************//
#include <ESP8266WiFi.h>
#include <PubSubClient.h>





//#define DEBUGG



#define SEC_TO_MILISEC(x) ((x)*1000) 
#define LOCK_PIN D2
#define PUSH_BUTTON_PIN D5
#define RED_LED_PIN D0
#define GREEN_LED_PIN D7

void openLock(void);
void closeLock(void);


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

class memEdgeDetector
{
  public:
  memEdgeDetector();
  bool risingEdge(void);
  bool fallingEdge(void);
  void updateMme(bool newData);
  bool getState(void);

  bool newData;
  bool currentData;
  bool isRising;
  bool isFalling;
};


char * ssid ="WIFI Pier2";
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
    openLock();
    delay(100);
  }
  else if(!strcmp(topic,"lock/close"))
  {
    closeLock();
    delay(100); 
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

void openLock(void){
  digitalWrite(LOCK_PIN,HIGH);
  digitalWrite(GREEN_LED_PIN,HIGH);
  digitalWrite(RED_LED_PIN,LOW);
  
}
void closeLock(void){
  digitalWrite(LOCK_PIN,LOW);
  digitalWrite(GREEN_LED_PIN,LOW);
  digitalWrite(RED_LED_PIN,HIGH);
}

void connectionLost(void){
  digitalWrite(LOCK_PIN,LOW);
  digitalWrite(GREEN_LED_PIN,HIGH);
  digitalWrite(RED_LED_PIN,HIGH);
}
void connectionOK(void){
  closeLock();
}

void setup() {
  Serial.begin(9600);
  pinMode(PUSH_BUTTON_PIN,INPUT);
  pinMode(LOCK_PIN,OUTPUT);
  pinMode(RED_LED_PIN,OUTPUT);
  pinMode(GREEN_LED_PIN,OUTPUT);
  connectionLost();
  setUpWifi(ssid,pass);
  setUpMqtt();
  
}



memEdgeDetector openButton;
bool isConnectionLost=false;
void loop() {
  if (!mqtt_client.connected()) 
  {
      if(!isConnectionLost){
        isConnectionLost=true;
        connectionLost();
      }
      
      reconnect();

      
 }
 if(isConnectionLost){
  isConnectionLost=false;
  connectionOK();
 }
 mqtt_client.loop();
 
 openButton.updateMme(digitalRead(PUSH_BUTTON_PIN));//sampleo el estado del boton
 if(openButton.fallingEdge()){
    mqtt_client.publish("lock/button","F");
      
 }
 if(openButton.risingEdge()){
    mqtt_client.publish("lock/button","R");  
    
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
//*************************EdgeDetector***********************

memEdgeDetector::memEdgeDetector()
{
  newData=0;
  currentData=0;
  isRising=0;
  isFalling=0;
}

bool memEdgeDetector::risingEdge(void){
  bool tempRes;
  tempRes=isRising;
  isRising=false;
  return tempRes;
}

bool memEdgeDetector::fallingEdge(void){
  bool tempRes;
  tempRes=isFalling;
  isFalling=false;
  return tempRes;
}

bool memEdgeDetector::getState(void){
  return currentData;
}

void memEdgeDetector::updateMme(bool newData){
  this->newData=newData;

  if(currentData==false && this->newData==true){
    isRising=true;
  }else if(currentData==true && this->newData==false){
    isFalling=true;    
  }
  currentData=this->newData;

  
}
