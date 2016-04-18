
// Angle control Related
#define aFF2 14
#define aFF1 15
#define aPWM 12
#define aDIR 16
#define aPOS A0

// Pressure control Related
#define fireRelay 17
#define blowoffRelay 18
#define inflatorRelay 19

// Sensor
#define deadmanSwi 20
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
char com[4];


void setup(){
  pinMode(aFF2, INPUT);      
  pinMode(aFF1, INPUT);
  pinMode(aPWM, OUTPUT);
  pinMode(aDIR, OUTPUT);
  pinMode(aPOS, INPUT);
  pinMode(fireRelay, OUTPUT);
  pinMode(blowoffRelay, OUTPUT);
  pinMode(inflatorRelay, OUTPUT);
  pinMode(deadmanSwi, INPUT);
  pinMode(angleSen, INPUT);
  pinMode(pressureSen, INPUT);
  pinMode(proxSen, INPUT);

  Serial.begin(9600);
  Serial1.begin(14400);
  aDirection = 0;
  armed = false;
     
  // Angle initialization
  digitalWrite(aDIR, 1);
  analogWrite(aPWM, 255);
  while(analogRead(aPOS) <= MAX_ANGLE_READING);
  analogWrite(aPWM, 0);
  digitalWrite(aDIR, aDirection);
  angle = MIN_ANGLE;
  
  // Pressure initialization
  digitalWrite(fireRelay, 1);
  digitalWrite(inflatorRelay, 1);
  digitalWrite(blowoffRelay, 1);
  //while(!analogRead(deadmanSwi));
  //pressureOperation(5);
}


void loop(){
  // Check pressure/ proximity sensors/ deadman switch
  safe = true;

  psi = readingToPsi(analogRead(pressureSen));
  safe &= (psi <= 50);
  sprintf(com, "p%02dz", readingToPsi(analogRead(pressureSen)));
  Serial1.print(com);
  if(!safe){
    while(readingToPsi(analogRead(pressureSen)) > 50){
      digitalWrite(blowoffRelay, 1);
    }
  }
  
//  safe &= (analogRead(proxSen) < 30);
//  safe &= (digitalRead(deadmanSwi) == 1);
  
  //Check angle & Send angle
  Serial1.print("a" + String(angle) + "z");
  
}


void serialEvent1(){
  char buf[24] = "";
  if(Serial1.available() >= 4){
    Serial1.readBytes(buf, 4); 
    Serial.println(buf);
    command = buf;
    
    if(safe){
      switch(command[0]){
        // Angle
        case 'a':{
          if(command[1] == 'i' && command[2] == 'n' && angle < MAX_ANGLE && !armed){
            angleOperation(++angle);
          } else if(command[1] == 'd' && command[2] == 'w' && angle > MIN_ANGLE && !armed){
            angleOperation(--angle);
          } else {
            // Error
          }
          break;
        }
          
        // Pressure
        case 'p':{
          int psi = readingToPsi(analogRead(pressureSen));
          if(command[1] == 'i' && command[2] == 'n' && psi < MAX_PSI && !armed){
            pressureOperation(++psi);
          } else if(command[1] == 'd' && command[2] == 'w' && psi > MIN_PSI && !armed){
            pressureOperation(--psi);
          } else {
            // Error
          }
          break;
        }

        // Fire code
        case 'f':{
          if(command[1] == '3' && command[2] == '0'){
            Serial1.print("a" + String(angle) + "z");
            sprintf(com, "p%02dz", readingToPsi(analogRead(pressureSen)));
            Serial1.print(com);
            Serial1.print("f30z");
          } else if(command[1] == '2' && command[2] == '0' && armed){
            digitalWrite(fireRelay, 0);
            delay(2000);
            digitalWrite(fireRelay, 1);
          } else if(command[1] == '1' && command[2] == '0'){
            armed = !armed;
          } else {
            // Error
          }
        }
        
        default:
            if(command[3] == 'z'){ break; }
            do{
              if(Serial1.available()){
                Serial1.readBytes(buf, 1);
                if(buf[0] == 'z'){ break; }
              }
            } while(true);
      }
    }
  }  
}


void angleOperation(int angle){
  int requiredReading = angleToReading(angle);
  int reading = analogRead(aPOS);
  while(abs(reading - requiredReading) > 1){
    aDirection = (reading - requiredReading) < 0 ? 1 : 0;
    digitalWrite(aDIR, aDirection);
    analogWrite(aPWM, 100);
    reading = analogRead(aPOS);
  }
  analogWrite(aPWM, 0);
}

int angleToReading(int angle){
  Serial.println("ANGLE: " + String(angle));
  if(angle <= MIN_ANGLE){ return MAX_ANGLE_READING; }
  if(angle >= MAX_ANGLE){ return MIN_ANGLE_READING; }
  double length = ((angle - MIN_ANGLE) / ANGLE_INTERVAL) * TOTAL_LENGTH;
  return (int) MAX_ANGLE_READING - (length / LENGTH_PER_READING);
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
  digitalWrite(blowoffRelay, 1);
  digitalWrite(inflatorRelay, 1);
}

int readingToPsi(int reading){
  if(reading < MIN_PSI_READING){ return -1; }
  double length = (reading - MIN_PSI_READING) / PSI_READING_INTERVAL;
  return (int) length * PSI_INTERVAL;
}

int psiToReading(int psi){
  Serial.println("PRESSURE: " + String(psi));
  if(psi <= MIN_PSI){ return MIN_PSI_READING; }
  if(psi >= MAX_PSI){ return MAX_PSI_READING; }
  double length = (psi / PSI_INTERVAL) * PSI_READING_INTERVAL;
  return (int) length + MIN_PSI_READING;
}

