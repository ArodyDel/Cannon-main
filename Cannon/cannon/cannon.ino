
#include <EasyTransfer.h>

EasyTransfer ETin;
EasyTransfer ETout;

struct RECEIVE_DATA_STRUCTURE{
  char chardata[4];
};

struct SEND_DATA_STRUCTURE{
  char chardata[4];
};

SEND_DATA_STRUCTURE dataReceive;
RECEIVE_DATA_STRUCTURE dataSend;

// Angle control Related
#define aFF2 51
#define aFF1 53
#define aPWM 12
#define aDIR 31
#define aPOS A0

// Pressure control Related
#define relayPower 52
#define fireRelay 17
#define blowoffRelay 20
#define inflatorRelay 21

// Sensor
#define angleSen A1
#define pressureSen A2
#define proxSen A3

// Actuator
const int MIN_ANGLE = 10;
const int MAX_ANGLE = 80;
const int MIN_ANGLE_READING = 60;
const int MAX_ANGLE_READING = 940;
const double TOTAL_LENGTH = 3.93 * 25.4;  // in millimeter
const double LENGTH_PER_READING = TOTAL_LENGTH / (MAX_ANGLE_READING - MIN_ANGLE_READING);
const double ANGLE_INTERVAL = MAX_ANGLE - MIN_ANGLE;

//Transducer
const int MIN_PSI = 0;
const int MAX_PSI = 50;
const double MIN_PSI_READING = 200.0;
const double MAX_PSI_READING = 400.0;
const double PSI_READING_INTERVAL = MAX_PSI_READING - MIN_PSI_READING;
const double PSI_INTERVAL = MAX_PSI - MIN_PSI;

int angle;
int psi;
bool safe;
bool armed;
int aDirection;
char* command;
char* com;


void setup(){
  pinMode(aFF2, INPUT);      
  pinMode(aFF1, INPUT);
  pinMode(aPWM, OUTPUT);
  pinMode(aDIR, OUTPUT);
  pinMode(aPOS, INPUT);
  pinMode(relayPower, OUTPUT);
  pinMode(fireRelay, OUTPUT);
  pinMode(blowoffRelay, OUTPUT);
  pinMode(inflatorRelay, OUTPUT);
  pinMode(angleSen, INPUT);
  pinMode(pressureSen, INPUT);
  pinMode(proxSen, INPUT);

  Serial.begin(9600);
  Serial1.begin(9600);
  ETin.begin(details(dataReceive), &Serial1);
  ETout.begin(details(dataSend), &Serial1);
  aDirection = 0;
  armed = false;
     
  // Angle initialization
  digitalWrite(aDIR, 1);
  analogWrite(aPWM, 255);
  while(analogRead(aPOS) <= MAX_ANGLE_READING){ Serial.println(digitalRead(aFF1)); Serial.println(digitalRead(aFF2)); Serial.println(analogRead(aPOS)); delay(1000);}
  analogWrite(aPWM, 0);
  digitalWrite(aDIR, aDirection);
  angle = MIN_ANGLE;
  sendCommand("a" + String(angle));

  digitalWrite(relayPower, 1);
  // Pressure initialization
  digitalWrite(fireRelay, 1);
  digitalWrite(inflatorRelay, 1);
  digitalWrite(blowoffRelay, 1);
  //pressureOperation(5);

  Serial.println("ANGLE: " + String(angle));
  Serial.println("PRESSURE: " + String(getPsi()));
  Serial.println("PROXIMITY: " + String(getProximitySensor()));
  Serial.println();

}
  

void loop(){
  serialProcess();
  angleOperation();
  // Check pressure/ proximity sensors/ deadman switch
  safe = true;  

//  psi = getPsi();
//  safe &= (psi <= 50);
//  if(!safe){
//    while(getPsi() > 50.0){
//      digitalWrite(blowoffRelay, 1);
//    }
//  }
//  safe &= (getProximitySensor() < 30);
}

void serialEvent(){
  if(Serial.available()){
    char buf[24] = "";
    Serial.readBytes(buf, 1);
    switch(buf[0]){
      case '1':
        digitalWrite(fireRelay, 0);
        Serial.println(buf[0]);
        break;
      case '2':
        digitalWrite(fireRelay, 1);
        Serial.println(buf[0]);
        break;
      case '3':
        digitalWrite(blowoffRelay, 0);
        Serial.println(buf[0]);
        break;
      case '4':
        digitalWrite(blowoffRelay, 1);
        Serial.println(buf[0]);
        break;
      case '5':
        digitalWrite(inflatorRelay, 0);
        Serial.println(buf[0]);
        break;
      case '6':
        digitalWrite(inflatorRelay, 1);
        Serial.println(buf[0]);
        break;
    }
  }
}

void serialProcess(){
  if(ETin.receiveData()){
    Serial.println(String(dataReceive.chardata));
    
    if(safe){
      switch(dataReceive.chardata[0]){
        // Angle
        case 'a':{
//          if(dataReceive.chardata[1] == 'i' && dataReceive.chardata[2] == 'n' && angle < MAX_ANGLE && !armed){
//            angleOperation(++angle);
//          } else if(dataReceive.chardata[1] == 'd' && dataReceive.chardata[2] == 'w' && angle > MIN_ANGLE && !armed){
//            angleOperation(--angle);
//          } else {
//            // Error
//          }
            int temp = (dataReceive.chardata[1] - 0x30) * 10 + (dataReceive.chardata[2] - 0x30);
            angle = temp;
            //angleOperation(angle);
          break;
        }
          
        // Pressure
        case 'p':{
          int psi = getPsi();
          if(dataReceive.chardata[1] == 'i' && dataReceive.chardata[2] == 'n' && psi < MAX_PSI && !armed){
            //pressureOperation(++psi);
              digitalWrite(blowoffRelay, 0);
          } else if(dataReceive.chardata[1] == 'd' && dataReceive.chardata[2] == 'w' && psi > MIN_PSI && !armed){
            //pressureOperation(--psi);
              digitalWrite(blowoffRelay, 1);
          } else {
            // Error
          }
          break;
       }

        // Fire code
        case 'f':{
          Serial.println("Fire code");
          if(dataReceive.chardata[1] == '3' && dataReceive.chardata[2] == '0'){
            sendCommand("a" + String(angle));
            sendCommand("p" + String((int)getPsi()));
            sendCommand("f30");
          } else if(dataReceive.chardata[1] == '2' && dataReceive.chardata[2] == '0' && armed){ 
            digitalWrite(fireRelay, 0);
            delay(2000);
            digitalWrite(fireRelay, 1);
          } else if(dataReceive.chardata[1] == '1' && dataReceive.chardata[2] == '0'){
            armed = !armed;
          } else {
            // Error
          }
          break;
        }
      }
    } else {
      Serial.println("Not Safe!");
    }

    Serial.println("ANGLE: " + String(angle));
    Serial.println("PRESSURE: " + String(getPsi()));
    Serial.println("PROXIMITY: " + String(getProximitySensor()));
    Serial.println();
  }
}


void angleOperation(){
  int requiredReading = angleToReading(angle);
  int reading = analogRead(aPOS);
  if(abs(reading - requiredReading) > 2){
    aDirection = (reading - requiredReading) < 0 ? 1 : 0;
    digitalWrite(aDIR, aDirection);
    analogWrite(aPWM, 100);
    reading = analogRead(aPOS);
    //Serial.print("Require: ");
    //Serial.println(requiredReading);
    //Serial.print("Read: ");
    //Serial.println(analogRead(aPOS));
  } else {
    //sendCommand("a" + String(angle));
    analogWrite(aPWM, 0);
  }
}

int angleToReading(int angle){
  if(angle <= MIN_ANGLE){ return MAX_ANGLE_READING; }
  if(angle >= MAX_ANGLE){ return MIN_ANGLE_READING; }
  double length = ((angle - MIN_ANGLE) / ANGLE_INTERVAL) * TOTAL_LENGTH;
  return (int) MAX_ANGLE_READING - (length / LENGTH_PER_READING);
}

double getAngle(){
   double reading = analogRead(angleSen);
   double temp = reading / 1023.0;
   return temp * 300;
}


void pressureOperation(int psi){
  int requiredReading = psiToReading(psi);
  int reading = analogRead(pressureSen);
  while(abs(reading - requiredReading) > 3){
    if(reading - requiredReading > 0){
      digitalWrite(blowoffRelay, 0);
      digitalWrite(inflatorRelay, 1);
    } else {
      digitalWrite(inflatorRelay, 0);
      digitalWrite(blowoffRelay, 1);
    }
  }
  sendCommand("p" + String(psi));
  digitalWrite(blowoffRelay, 1);
  digitalWrite(inflatorRelay, 1);
}

double getPsi(){
  double reading = analogRead(pressureSen);
  if(reading < MIN_PSI_READING){ return -1; }
  double length = (reading - MIN_PSI_READING) / PSI_READING_INTERVAL;
  return length * PSI_INTERVAL;
}

int psiToReading(int psi){
  if(psi <= MIN_PSI){ return MIN_PSI_READING; }
  if(psi >= MAX_PSI){ return MAX_PSI_READING; }
  double length = (psi / PSI_INTERVAL) * PSI_READING_INTERVAL;
  return (int) length + MIN_PSI_READING;
}

double getProximitySensor(){
  return analogRead(proxSen);
}


void sendCommand(String command){
  dataSend.chardata[0] = command[0];
  dataSend.chardata[1] = command[1];
  dataSend.chardata[2] = command[2];
  dataSend.chardata[3] = 0;
  ETout.sendData();
}


