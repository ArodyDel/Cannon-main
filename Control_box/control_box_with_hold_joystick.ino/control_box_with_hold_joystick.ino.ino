#include <LiquidCrystal.h>

#define RS	24	// RS pin for J204A LCD display
#define E  	22	// E pin for J204A LCD display
#define D4  36	
#define D5  34	// Data pins for J204A
#define D6  32
#define D7  30
#define AUP  2	// Angle up pin. Left joystick
#define ADW  3	// Angle down pin. Left joystick
#define PUP	 18	// Pressure up pin. Right joystick
#define PDW	 19	// Pressure down pin. Right joystick
#define ARM  20	// Arm/Disarm toggle switch pin
#define FIR  21	// Fire pin. BIG RED BUTTON

	
int ignoreFlag = 0;					// flag is set if the cannon is locked. Ignore all interrupt input.
int angleLow = 0;					// flags to ignore input on conditions.
int angleHigh = 0;
int pressureLow = 0;
int pressureHigh = 0;
int CannonReadyFlag = 0;			// Is the Cannon ready to accept commands
int safe = 1;						// mode of the cannon for now only 1 or 0
int angle = -1;						// cannon angle. Initialized to -1 so it doesn't display at the start.
int pressure = -1;					// cannon pressure. Same as above.
char* command;						// pointer to the string we will get over serial and interpreted as a command
long debouncing_time = 150;			// I think it's in milisec. If interrupt is triggering more than once increase this value
volatile unsigned long last_micros_aup = 1;	// Used to store the previous time an interrupt was triggered used for debouncing
volatile unsigned long last_micros_adw = 1;
volatile unsigned long last_micros = 1;

LiquidCrystal lcd(RS, E, D4, D5, D6, D7);		// Creates an lcd object so that we can use the LiquidCrystal library

void setup() 
{
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  lcd.begin(20, 4);                         // 20, 4 is our lcd size
	displayMenu();														// Initialize the lcd to display the Menu without values
	pinMode(AUP, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic. 
	pinMode(ADW, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(PUP, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(PDW, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(ARM, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	pinMode(FIR, INPUT_PULLUP);											// Making the interrupt pin INPUT_PULLUP makes it less sensitive to noise, but switches the logic.
	Serial.begin(9600);
	Serial.print("f30");												// Fire code for ready.
  Serial2.begin(9600);
	}

void loop() 
{
	if(CannonReadyFlag)                      // Wait until the cannon is ready for input to enable all input interrupts.
  {
    attachInterrupt(digitalPinToInterrupt(AUP), aUpDebounce, LOW); // use of digitalPinToInterrupt is safer. debounce is the name of the function called when triggered.
    attachInterrupt(digitalPinToInterrupt(ADW), aDwDebounce, LOW);
    attachInterrupt(digitalPinToInterrupt(PUP), pUpDebounce, RISING);
    attachInterrupt(digitalPinToInterrupt(PDW), pDwDebounce, RISING);
    attachInterrupt(digitalPinToInterrupt(ARM), armDebounce, RISING);
    attachInterrupt(digitalPinToInterrupt(FIR), fireDebounce, RISING);
  }
}

// It's the debouncing functionw that are called during interrupt. It simply buffers up the interrupts only allowing once interrupt every debouncing_time milisec
// It calls the actual Interrupt service routine only after the debouncing.
void aUpDebounce()
{
  last_micros_aup++;
	if(last_micros_aup%3500 == 0)
	{
		angleUp();
	}
}

void aDwDebounce()
{
  last_micros_adw++;
	if(last_micros_adw%3500 == 0)
	{
		angleDown();
	}
}

void pUpDebounce()
{
	if((long)(micros() - last_micros) >= debouncing_time*1000)
	{
		pressureUp();
		last_micros = micros();
	}
}

void pDwDebounce()
{
	if((long)(micros() - last_micros) >= debouncing_time*1000)
	{
		pressureDown();
		last_micros = micros();
	}
}

void armDebounce()
{
	if((long)(micros() - last_micros) >= debouncing_time*1000)
	{
		arm();
		last_micros = micros();
	}
}

void fireDebounce()
{
	if((long)(micros() - last_micros) >= debouncing_time*1000)
	{
		fire();
		last_micros = micros();
	}
}

// The ISRs (Interrupt Service Routine).
void angleUp()
{	
	if(!angleHigh)
	{
		Serial2.print("aup");
    Serial.print("aup");
    angle++;
    updateMenu();
	}
	angleLow = 0;
}

void angleDown()
{	
	if(!angleLow)
	{
		Serial2.print("adw");
    Serial.print("adw");
    angle--;
    updateMenu();
	}
	angleHigh = 0;
}

void pressureUp()
{	
	if(!pressureHigh)
	{
		Serial.print("pup");
    pressure++;
    updateMenu();
  }
	pressureLow = 0;
}

void pressureDown()
{	
	if(!pressureLow)
	{
		Serial.print("pdw");
	  pressure--;
    updateMenu();
	}
	pressureHigh = 0;
}

//These two will be heavily modified. I don't know exactly how we will do this yet.
void arm()
{	
	Serial.print("arm");
	if(safe == 0)
		safe = 1;
	else
		safe = 0;
  updateMenu();
	//do extra things here like switching ignore flags, etc.
}

void fire()
{	
	Serial.print("fir");
	safe = 1;		//I think.
	// Might want to just impremment an arm/disarm function that takes care of flags
}

// Only called when there is something transmitted to the Arduino
void serialEvent()
{
	char buf[512] = "";					// Set up a small buffer for readBytes
  
	while(Serial.available() < 3);		// Poll until we have 3 characters(1 command)

	Serial.readBytes(buf, 3);			// Read the command

	command = buf;						

	Serial.print(command);				// Echo the command for testing purposes
	Serial.write(0xA);
	Serial.write(0xD);
										// Rudamentary switch statement for hangling input from the cannon. The commands can be changed later.
	switch(command[0])				// If it is a then the command is aXX where XX is the curent angle. Example: a17 is angle of 17 deg.
	{
		case 'a':		//angle
		{
			angle = (command[1] - 0x30)*10 + (command[2] - 0x30);		// I don't know if this is the best way to convert the 2nd and 3rd char to a number but it works.
			updateMenu();
			break;
		}
		case 'p':		//pressure
		{
			pressure = (command[1] - 0x30)*10 + (command[2] - 0x30);
			updateMenu();
			break;
		}
		case 'f':		//firecode (error code)
		{
			switch(command[1])
			{
				case '0':			// General Error. Something happened to the cannon and it blocked all input.
				{
					ignoreFlag = 1;
					break;
				}
				case '1':			// Angle Error.
				{
					if(command[2] == '0')	//Angle too high
					{
						lcd.setCursor(16, 1);	//Put Cursor left of the angle value;
						lcd.write('!');	
						angleHigh = 1;
					}
					else					//Angle too low
					{
						lcd.setCursor(16, 1);
						lcd.write('!');
						angleLow = 1;
					}
					break;
				}
				case '2':			//Pressure error
				{
					if(command[2] == '0')	//Pressure too high
					{
						lcd.setCursor(13, 2);	//Put Cursor left of the pressure value;
						lcd.write('!');	
						pressureHigh = 1;
					}
					else					//Pressure too low
					{
						lcd.setCursor(13, 2);
						lcd.write('!');
						pressureLow = 1;
					}
					break;
				}
				case '3':			// Cannon is ready.
				{
					CannonReadyFlag = 1;
					break;
				}
			}
			break;
		}
		default:			//Empty for now. Implement buffer syncronization if needed later.
		{
			break;
		}
		
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
	lcd.setCursor(0, 3);
	lcd.print("Speed:");
}

// The X values for setCursor were made so the values will be outputted at the end of each row without overflowwing into the next line. It assumes only 2 decimal values.
// 7 will show up as 07
void updateMenu()
{
	lcd.setCursor(15, 0);
	lcd.print(String( (safe)?" SAFE":"ARMED"));			//Depending on the state variable choose between the 2 strings around the : character
	if(angle > 0)
	{
		lcd.setCursor(17, 1);
		lcd.print(String(angle) + char(223));              	//decmal value 223 is the value for the degree sign for the J204A degree character.
	}
	if(pressure > 0)
	{
		lcd.setCursor(14, 2);
		lcd.print(String(pressure/10) + String(pressure%10) +" PSI");
		int velocity = pressure/3.5;
		lcd.setCursor(14, 3);
		lcd.print(String(velocity/10) + String(velocity%10) + " m/s");	// Just randomply put m/s. I don't know what unit we will use.
	}
}

