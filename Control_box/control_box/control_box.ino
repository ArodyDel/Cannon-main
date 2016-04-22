#include <LiquidCrystal.h>
#include <EasyTransfer.h>

EasyTransfer ETin, ETout;

struct SEND_DATA_STRUCTURE{
  char chardata[4];
};

struct RECEIVE_DATA_STRUCTURE{
  char chardata[4];
};


SEND_DATA_STRUCTURE dataSend;
RECEIVE_DATA_STRUCTURE dataReceive;

#define RS	24	// RS pin for J204A LCD display
#define E  	22	// E pin for J204A LCD display
#define D4  36	
#define D5  34	// Data pins for J204A
#define D6  32
#define D7  30
#define LL  50  // LCD Backlight pin
#define BL  51  // Button Light pin 
#define AUP  25	// Angle up pin. Left joystick
#define ADW  23	// Angle down pin. Left joystick
#define PUP	 29	// Pressure up pin. Right joystick
#define PDW	 27	// Pressure down pin. Right joystick
#define ARM  2	// Arm/Disarm toggle switch pin
#define FIR  3	// Fire pin. BIG RED BUTTON

#define MAX_ANGLE 80
#define MIN_ANGLE 10
#define MAX_PRESSURE 50
#define MIN_PRESSURE 10

int joystickupheld = 0;
int joystickdwheld = 0;
int fired = 0;
int ignoreFlag = 0;					// flag is set if the cannon is locked. Ignore all interrupt input.
int angleLow = 0;					// flags to ignore input on conditions.
int angleHigh = 0;
int pressureLow = 0;
int pressureHigh = 0;
int CannonReadyFlag = 0;			// Is the Cannon ready to accept commands
int safe = 1;						// mode of the cannon for now only 1 or 0
int angle = -1;						// cannon angle. Initialized to -1 so it doesn't display at the start.
int pressure = 0;					// cannon pressure. Same as above.
char* command;						// pointer to the string we will get over serial and interpreted as a command
long debouncing_time = 100;			// I think it's in milisec. If interrupt is triggering more than once increase this value
volatile unsigned long last_micros = 1;

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);		// Creates an lcd object so that we can use the LiquidCrystal library

void setup() 
{
  pinMode(LL, OUTPUT);
  pinMode(BL, OUTPUT);
  digitalWrite(LL, HIGH);
  digitalWrite(BL, HIGH);
  lcd.begin(20, 4);                         // 20, 4 is our lcd size
	displayMenu();														// Initialize the lcd to display the Menu without values
	pinMode(AUP, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic. 
	pinMode(ADW, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(PUP, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(PDW, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(ARM, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(FIR, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	Serial.begin(19200);
  Serial2.begin(19200);
  ETout.begin(details(dataSend), &Serial2);
  ETin.begin(details(dataReceive), &Serial2);
  dataSend.chardata[0] = 'f';
  dataSend.chardata[1] = '3';
  dataSend.chardata[2] = '0';
  dataSend.chardata[3] = 0;
  ETout.sendData();
}

void loop() 
{
	if(CannonReadyFlag)                      // Wait until the cannon is ready for input to enable all input interrupts.
  {
    noInterrupts();           // disable all interrupts
    TCCR1A = 0;               //timer1 setup
    TCCR1B = 0;
    TCNT1  = 0;               // Reset the count in the timer

    OCR1A = 10000;            // compare match register I'm not sure exactly how much that value ammounts to but it is roughly 200-250 milisec
    TCCR1B |= (1 << WGM12);   // Clear Timer Count on interrupt mode
    TCCR1B |= (1 << CS12);    // 256 prescaler means that the timer wil tick 256 times slower than the clock speed
    TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
    interrupts();
 
    attachInterrupt(digitalPinToInterrupt(FIR), fireDebounce, RISING);
    updateMenu();
    CannonReadyFlag = 0;
  }

  if(ETin.receiveData())
  {
    displayMenu();
    updateMenu();
    Serial.print(String(dataReceive.chardata));
    switch(dataReceive.chardata[0])        // If it is a then the command is aXX where XX is the curent angle. Example: a17 is angle of 17 deg.
    {
      case 'a':   //angle
      {
        if(angle != (dataReceive.chardata[1] - 0x30)*10 + (dataReceive.chardata[2] - 0x30))
        {
          angle = (dataReceive.chardata[1] - 0x30)*10 + (dataReceive.chardata[2] - 0x30);   // I don't know if this is the best way to convert the 2nd and 3rd char to a number but it works.
          updateMenu();
        }
        break;
      }
      case 'p':   //pressure
      {
        pressure = (dataReceive.chardata[1] - 0x30)*10 + (dataReceive.chardata[2] - 0x30);
        updateMenu();
        break;
      }
      case 'f':   //firecode (error code)
      {
        switch(dataReceive.chardata[1])
        {
          case '0':     // Error State
          {
            if(dataReceive.chardata[2] == '1')
            {
              ignoreFlag = !ignoreFlag;
              if(ignoreFlag)
              {
                lcd.clear();
                lcd.print("Cannon Error");
              }
              else
              {
                displayMenu();
                updateMenu();
              }
              break;
            }
            break;
          }
          case '3':     // Cannon is ready.
          {
            CannonReadyFlag = 1;
            break;
          }
          case '4':     // Flip Cannon State
          {
            arm();
            break;
          }
        }
        break;
      }
    }
  }
}

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
  if(ignoreFlag)
    return;
  if((safe && !digitalRead(ARM)))
    arm();
  if((!safe && digitalRead(ARM)))
    arm();
  if(!safe)
    return;
  if(!digitalRead(AUP))
  {
    if(angle < MAX_ANGLE)
      angle++;
    updateMenu();
    joystickupheld = 1;
  }
  else if(joystickupheld == 1)
  {
    dataSend.chardata[0] = 'a';
    dataSend.chardata[1] = angle/10 + 0x30;
    dataSend.chardata[2] = angle%10 + 0x30;
    dataSend.chardata[3] = 0;
    ETout.sendData();
    joystickupheld =0;
  }
  if(!digitalRead(ADW))
  {
    if(angle > MIN_ANGLE)
      angle--;
    updateMenu();
    joystickdwheld = 1;
  }
  else if(joystickdwheld == 1)
  {
    dataSend.chardata[0] = 'a';
    dataSend.chardata[1] = angle/10 + 0x30;
    dataSend.chardata[2] = angle%10 + 0x30;
    dataSend.chardata[3] = 0;
    ETout.sendData();
    joystickdwheld =0;
  }
  if(!digitalRead(PUP))
    pressureUp();
  if(!digitalRead(PDW))
    pressureDown();
}
// It's the deboun-cing functionw that are called during interrupt. It simply buffers up the interrupts only allowing once interrupt every debouncing_time milisec
// It calls the actual Interrupt service routine only after the debouncing.


void fireDebounce()
{
  if(ignoreFlag)
    return;
	if((long)(micros() - last_micros) >= debouncing_time*4000)
	{
		fire();
		last_micros = micros();
	}
}

// The ISRs (Interrupt Service Routine).
void pressureUp()
{	
    pressure = 1;
    dataSend.chardata[0] = 'p';
    dataSend.chardata[1] = 'i';
    dataSend.chardata[2] = 'n';
    dataSend.chardata[3] = 0;
    Serial.print(String(dataSend.chardata));
    ETout.sendData();
    updateMenu();
}

void pressureDown()
{	
    pressure = 0;
    dataSend.chardata[0] = 'p';
    dataSend.chardata[1] = 'd';
    dataSend.chardata[2] = 'w';
    dataSend.chardata[3] = 0;
    Serial.print(String(dataSend.chardata));
    ETout.sendData();
    updateMenu();
}

//These two will be heavily modified. I don't know exactly how we will do this yet.
void arm()
{	
  dataSend.chardata[0] = 'f';
  dataSend.chardata[1] = '1';
  dataSend.chardata[2] = '0';
  dataSend.chardata[3] = 0;
  Serial.print(String(dataSend.chardata));
  ETout.sendData();
	safe = !safe;
  digitalWrite(BL, safe);
  fired = 0;
	//do extra things here like switching ignore flags, etc.
}

void fire()
{	
  if(!safe && !fired)
  {
    fired = 1;
    dataSend.chardata[0] = 'f';
    dataSend.chardata[1] = '2';
    dataSend.chardata[2] = '0';
    dataSend.chardata[3] = 0;
    Serial.print(String(dataSend.chardata));
    ETout.sendData();
  }
}

void displayMenu()
{
	lcd.clear();
	lcd.setCursor(0, 0);			// Sets the lcd cursor to 1st line beggining
	lcd.print("Cannon state:");
	lcd.setCursor(0, 1);			// 2nd line beginning and etc.
	lcd.print("Angle:");
	lcd.setCursor(0, 2);
	lcd.print("Pressure:");
//	lcd.setCursor(0, 3);
//	lcd.print("Speed:");
}

// The X values for setCursor were made so the values will be outputted at the end of each row without overflowwing into the next line. It assumes only 2 decimal values.
// 7 will show up as 07
void updateMenu()
{
	lcd.setCursor(15, 0);
	lcd.print(String((safe)?" SAFE":"ARMED"));			//Depending on the state variable choose between the 2 strings around the : character
	if(angle > 0)
	{
		lcd.setCursor(17, 1);
		lcd.print(String(angle) + char(223));              	//decmal value 223 is the value for the degree sign for the J204A degree character.
	}
  lcd.setCursor(17, 2);
	lcd.print(String((pressure)?" ON":"OFF"));
}

