#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>

byte mac[] = { 
  0x90,0xA2,0xDA,0x0D, 0x30, 0xD7 };
char serverName[] = "192.168.1.1";
EthernetClient main_client;
byte ip[] = {
  192,168, 1,2};  
//byte ip[]={
//10,100,0,220};
int totalCount = 0;
String responce;
String token = "not_set";
int emm_port = 9763;
int apim_port = 8281;

void setup() {
  Serial.begin(9600);
   digitalWrite(10,OUTPUT);
  while (!Serial) {

  }
  delay(500);
  Serial.println(F("Starting ethernet..."));
  
  Ethernet.begin(mac,ip);
  main_client.connected();
  Serial.println("Requeting token");
  requestToken(main_client,serverName,"U78");
 
}


void inisializePort(int emm,int apim){
  emm_port =  emm;
  apim_port = apim;
}

void loop()
{
  
}


byte postPage(EthernetClient  user_client,char server[],int thisPort,char* page,char* thisData,String method)
{
  //  char* domainBuffer,
  char outBuf[80];
  Serial.print(F("connecting..."));
  Serial.println(thisData);
  //Serial.print("Server name:");
  if(user_client.connect(server,thisPort) == 1)
    //if(user_client.connect(server,thisPort))
  {
    Serial.println(F("connected"));
    // send the header
    if(method.equals("PUT")){
      sprintf(outBuf,"PUT %s HTTP/1.1",page);
    }
    else{
      Serial.println("POST method called");
      sprintf(outBuf,"POST %s HTTP/1.1",page);
    }
    user_client.println(outBuf);
    sprintf(outBuf,"Host: %s","192.168.1.1");
    user_client.println(outBuf);
    user_client.println("Authorization:  Bearer "+token);
    user_client.println(F("Connection: close\r\nAccept: application/json\r\nContent-Type: application/json"));
    sprintf(outBuf,"Content-Length: %u\r\n",strlen(thisData));
    user_client.println(outBuf);
    // send the body (variables)
    user_client.print(thisData);
  }
  else
  {
    Serial.println(F("failed"));
    return 0;
  }
  int connectLoop = 0;
  char inChar;
  boolean isBegin = false;
  responce = "";

  while(user_client.connected())
  {
    while(user_client.available())
    {
      inChar = user_client.read();

      if(method.equals("POST")){

        if(inChar == '{'){
          isBegin = true;  
        }
        if(isBegin){
          responce +=inChar; 
        }

        if (inChar == '}') {
          user_client.stop();
          break;
        }
      }
      else{
        if(inChar == 'H'){
          isBegin = true;
        }
        if(isBegin){
          responce+= inChar;
        }
        if(inChar == '\n'){
          user_client.stop();
          break;
        }
      }
      connectLoop = 0;
    }
    delay(1);
    connectLoop++;
    if(connectLoop > 10000)
    {
      Serial.println();
      Serial.println(F("Timeout"));
      user_client.stop();
    }
  }
  Serial.println();
  Serial.println(F("disconnecting."));
  user_client.stop();
  return 1;
}

void requestToken(EthernetClient client,char serverIP[],String id){
  String param = "{\"deviceID\" : \""+id+"\"}";
  int str_len = param.length() + 1; 
  char charPayload[str_len];
  param.toCharArray(charPayload, str_len);
  postPage(client,serverIP,emm_port,"/emm/api/devices/iot/getToken",charPayload,"POST");
  if(responce != "" ){
    int firstPartStartingIndex = responce.indexOf(',');
    String firstSubstringPart = responce.substring(1,firstPartStartingIndex);
    String lastSubstringPart = responce.substring(firstPartStartingIndex+1,(responce.length()-1));
    firstSubstringPart.trim();
    lastSubstringPart.trim();
    String errorStatus = firstSubstringPart.substring(firstSubstringPart.indexOf(':')+3,(firstSubstringPart.length()-1));
    Serial.println("error is : "+errorStatus);
    String new_token = lastSubstringPart.substring(lastSubstringPart.indexOf(':')+3,(lastSubstringPart.length()-1));
    Serial.println("token is : "+new_token);
    if(errorStatus.equals("false")){
      Serial.println("Saving token....");
      token =  new_token;
    }
    else if (errorStatus.equals("Please claim your device. Device is not claimed")){
      Serial.println("Please claim your device. Device is not claimed");
      //registerDevice(client,serverIP,id);
      requestToken(client,serverIP,id);
    }
    else if(errorStatus.equals("true")){
      Serial.println(token);
      requestToken(client,serverIP,id);
    }
  }
  else{
    requestToken(client,serverIP,id);
  }

}

void registerDevice(EthernetClient client,char serverIP[],String deviceID){
  Serial.println("Registerig device");
  String deviceDetails = "{\"platform\":\"linux\",\"version\":\"1.0.0\",\"deviceID\":\""+deviceID+"\"}";
  String strSPyload ="{\"auth\": \"token\",\"auth_params\": {},\"properties\":"+deviceDetails+"}";
  int str_len = strSPyload.length() + 1; 
  char charPayload[str_len];
  strSPyload.toCharArray(charPayload, str_len);
  postPage(client,serverIP,emm_port,"/emm/api/devices/iot/register",charPayload,"POST");
  String responceStatus = responce.substring(responce.indexOf(':')+3,(responce.length()-2));
  Serial.println("error is : "+responceStatus);
  if(responceStatus.equals("200")){
    Serial.println("Device registered");
  }
  else if(responceStatus.equals("\"500\", \"message\" : \"Device Type not forund for Device ID \"")){
    Serial.println("Device not registred in system");
  }
  else{
    Serial.println("Device registering faield");
    registerDevice(client,serverIP,deviceID);

  }
}

void publishData(EthernetClient client,char serverIP[],String jsonOb,char param_id[]){

  char *first_part = "{\"dataStream\":{\"data\":\"";
  char *data, text[5];
  sprintf(text, "%d", (rand() % 100));
  data = text;
  char jsonObj[150];
  char *str_id = "\",\"id\":\"";
  char *id = param_id;
  char *lastPart = "\",\"stream\": \"Color\",\"version\": \"1.0.0\" }}";
  snprintf(jsonObj, sizeof jsonObj, "%s%s%s%s%s", first_part, data, str_id,
  id, lastPart);
  //port 8281
  postPage(client,serverIP,apim_port,"/wso2/1.0.0/pushData",jsonObj,"PUT");
  String responceStatus = responce.substring(13,responce.length());
  Serial.print("responceStatus"+responceStatus);
  responceStatus.trim();
  if(responceStatus.equals("Unauthorized")){
    Serial.print("Token is invalid.....");
    Serial.print("Requesting Token.....");
    requestToken(client,serverIP,id);
  }
  else if(responceStatus.equals("OK")){
    Serial.print("Data published");
  }

}






































