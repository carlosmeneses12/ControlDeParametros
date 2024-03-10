#include <SPI.h>
#include <Ethernet.h>
#include <OneWire.h> 

int DS18S20_Pin = 1;
OneWire ds(DS18S20_Pin);  // on digital pin 1

// Registrar la Mac para su proyecto.
// Ingrese la MAC de su shell, para obtenerla, puede acceder a: Archivo->Ejemplos->Ethernet->DhcpAdressPrinter.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
char serverName[] = "10.58.1.84";    // name address of your domain

// Esta IP se asignará en caso de que el DHCP falle en asignar la ip al servidor
IPAddress ip(192,168,0,177);
int serverPort = 80;
EthernetClient client;
int totalCount = 0;
char pageAdd[64];

char tempString[] = "00.00";
char humedadString[] = "000.00";
char phString[] = "00.00";

// Setea el delay en milisegundos.
// 5 segundos.
#define delayMillis 5000UL

unsigned long thisMillis = 0;
unsigned long lastMillis = 0;

//DEFINIR DATOS SENSORES DHT11
// Incluimos librería
#include <DHT.h>
 
// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 7
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
 
// Inicializamos el sensor DHT11
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);

 
  // Comenzamos el sensor DHT
  dht.begin();
  //////////////////////////
  pinMode(A0,INPUT);
  pinMode(A2,INPUT);

  
  // disable SD SPI
  pinMode(4,OUTPUT);
  digitalWrite(4,HIGH);

  // Iniciar ethernet
  Serial.println(F("Conectando a ethernet..."));
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Falla en la configuración de DHCP");

    Ethernet.begin(mac, ip);
  }

  digitalWrite(10,HIGH);

  Serial.println(Ethernet.localIP());

  delay(2000);
  Serial.println(F("Listo"));
}

void loop()
{  
  thisMillis = millis();

  if(thisMillis - lastMillis > delayMillis)
  {
    lastMillis = thisMillis;

//LLamar método para obtener datos 
    getTemperatura();
    getHumedad();
    getpH();

  }    
}

//Caputra de temperatura C en DHT11
void getTemperatura(){
   //Lectura en Celsius 
    float t = dht.readTemperature();

    Serial.println("Temperatura C:");
    //Al método ftoa, se le envía el arreglo llamado tempString, el valor del sensor, cantidad de decimales
    Serial.println(ftoa(tempString,t,2));
    //Se envian los datos a las API creadas en el servidor, asegurandose de que los datos sean los mismos que recibirá la página php
    //en la variable $arduino_data = $_GET['data_temperatura']; por ejemplo.
    sprintf(pageAdd,"/arduino/firebaseTemperatura.php?data_temperatura=%s",ftoa(tempString,t,2));

    if(!getPage(serverName,serverPort,pageAdd)) Serial.print(F("Falla"));
    else Serial.print(F("OK "));
    totalCount++;
    Serial.println(totalCount,DEC);

    }

  //Caputra de humedad 
void getHumedad(){
   //Lectura humedad
    int valorHum = analogRead(A2);
    float Hum_porcen = ( 100 - ( (valorHum/1023.00) * 100 ) );

    Serial.println("Porcentaje de humedad:");
    //Al método ftoa, se le envía el arreglo llamado humedadString, el valor del sensor, cantidad de decimales
    Serial.println(ftoa(humedadString,Hum_porcen,2));
    //Se envian los datos a las API creadas en el servidor, asegurandose de que los datos sean los mismos que recibirá la página php
    //en la variable $arduino_data = $_GET['data_temperatura']; por ejemplo.
    sprintf(pageAdd,"/arduino/firebaseHumedad.php?data_humedad=%s",ftoa(humedadString,Hum_porcen,2));

    if(!getPage(serverName,serverPort,pageAdd)) Serial.print(F("Falla"));
    else Serial.print(F("OK "));
    totalCount++;
    Serial.println(totalCount,DEC);

    }

    //Caputra de pH
void getpH(){
   //Lectura en Celsius 
    int valorpH= analogRead(A0); 
    float voltpH = float (valorpH)/ 1023*5.0;
    float pHValue = 2.63 * voltpH - 0.36;

    Serial.println("pH:");
    //Al método ftoa, se le envía el arreglo llamado tempString, el valor del sensor, cantidad de decimales
    Serial.println(ftoa(phString,pHValue,2));
    //Se envian los datos a las API creadas en el servidor, asegurandose de que los datos sean los mismos que recibirá la página php
    //en la variable $arduino_data = $_GET['data_ph']; por ejemplo.
    sprintf(pageAdd,"/arduino/firebasePh.php?data_ph=%s",ftoa(phString,pHValue,2));

    if(!getPage(serverName,serverPort,pageAdd)) Serial.print(F("Falla"));
    else Serial.print(F("OK "));
    totalCount++;
    Serial.println(totalCount,DEC);

    }
//Método que obiene la dirección de la página en la API que permite enviar los datos.
byte getPage(char *ipBuf,int thisPort, char *page)
{
  int inChar;
  char outBuf[128];

  Serial.print(F("Conectando..."));

  if(client.connect(ipBuf,thisPort))
  {
    Serial.println(F("Conectado"));

    sprintf(outBuf,"GET %s HTTP/1.1",page);
    client.println(outBuf);
    sprintf(outBuf,"Host: %s",serverName);
    client.println(outBuf);
    client.println(F("Connection: close\r\n"));
  } 
  else
  {
    Serial.println(F("failed"));
    return 0;
  }

  // Este loop controla si el hardware de desconecta
  int connectLoop = 0;

  while(client.connected())
  {
    while(client.available())
    {
      inChar = client.read();
      Serial.write(inChar);
      // Si hay nuevos paquetes, se reinicia el contador
      connectLoop = 0;
    }

    connectLoop++;

    // si el loop tiene más de 10 segundos.
    if(connectLoop > 10000)
    {
      // Entonces cierra la conección por tiempo.
      Serial.println();
      Serial.println(F("Timeout"));
      client.stop();
    }
    
    delay(1);
  }

  Serial.println();

  Serial.println(F("Desconectado."));

  client.stop();

  return 1;
}
//Este código permite convertir los valores del sensor a string para su interpretación como valor al ser enviado a la API en PHP.
char *ftoa(char *a, double f, int precision)
{
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};  
  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long desimal = abs((long)((f - heiltal) * p[precision]));
  itoa(desimal, a, 10);
  return ret;
}
