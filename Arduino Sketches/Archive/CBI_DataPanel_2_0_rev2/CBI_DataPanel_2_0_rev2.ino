#include <LedControl.h> 

/*****
Still to add the following:

I2C of special messages e.g. I heart U

****/

//////////////////////////////////////////////////////////
// 5/23/2018 - Added logic for door switch sensors on Arduino Mini Breakout Board - S.Sloan
// 5/23/2018 - Added an ESB mode as created by Michael Erwin - S.Sloan
// 5/31/2014 - Renamed to CBI_DataPanel_2_0 - VAShadow 
// 5/30/2014 - Added CBI Support - VAShadow 
//
// 
// 12/05/2013 - CuriousMarc DataPort sketch v1.0
// Alternate sketch for Michael's Erwin DataPort 
// with a more organic feel
//
///////////////////////////////////////////////////////////

// change this to match which Arduino pins you connect your panel to,
// which can be any 3 digital pins you have available. 
// This is the pinout for the DroidConUino, same as the R series

// Pinout set for Mowee Arduino Mini Breakout Board
#define DATAIN_PIN 10 
#define CLOCK_PIN  11
#define LOAD_PIN   12
#define LeftDoorPin 2 //  Used for door open/closed signals on the Arduino Mini Breakout using 10K pulldown resistors
#define RightDoorPin 4 //  Used for door open/closed signals on the Arduino Mini Breakout using 10K pulldown resistors

// Uncomment this if you want the blocky ESB look of for the CBI lights created by Michael Erwin - S.Sloan
#define ESBmode

//Set this to which Analog Pin you use for the voltage in.
#define analoginput 0

#define greenVCC 12.5    // Green LED on if above this voltage
#define yellowVCC 12.0   // Yellow LED on if above this voltage
#define redVCC 11.5      // Red LED on if above this voltage

// For 15volts: R1=47k, R2=24k
// For 30volts: R1=47k, R2=9.4k
#define R1 47000.0     // >> resistance of R1 in ohms << the more accurate these values are
#define R2 24000.0     // >> resistance of R2 in ohms << the more accurate the measurement will be

// change this to match your hardware, I have dataport as the device 0 (first in chain)
// and I don't have a CBI, but it would be second in the chain
// Invert the 1 and the 0 if you have them in the other order
#define DATAPORT 1 // device 1 is second in chain
#define CBI      0 // device 0 is first in chain

// Number of Maxim chips that are connected
#define NUMDEV 2   // One for the dataport, one for the battery indicator/CBI

// the dataport is quite dim, so I put it to the maximum
#define DATAPORTINTENSITY 15  // 15 is max
#define CBIINTENSITY 15  // 15 is max

// uncomment this to test the LEDS one after the other at startup
//#define TEST
// This will revert to the old style block animation for comparison
//#define LEGACY

// If you are using the voltage monitor uncomment this
#define monitorVCC



// the timing values below control the various effects. Tweak to your liking.
// values are in ms. The lower the faster.
#define TOPBLOCKSPEED   70
#define BOTTOMLEDSPEED  200
#define REDLEDSPEED     500
#define BLUELEDSPEED    500
#define BARGRAPHSPEED   200
#define CBISPEED_RANDOM 50 // 50 for Curious Marc version

#define CBISPEED_ESB    300 // 50 for the blocky ESB version - S.Sloan

// Uncomment this if you want an alternate effect for the blue LEDs, where it moves
// in sync with the bar graph
//#define BLUELEDTRACKGRAPH

//============================================================================

float vout = 0.0;       // for voltage out measured analog input
int value = 0;          // used to hold the analog value coming out of the voltage divider
float vin = 0.0;        // voltage calulcated... since the divider allows for 15 volts

// Instantiate LedControl driver
LedControl lc=LedControl(DATAIN_PIN,CLOCK_PIN,LOAD_PIN,NUMDEV);   // RSeries FX i2c v5 Module Logics Connector 

void setup() 
{
  Serial.begin(57600);                    

  // initialize Maxim driver chips
  lc.shutdown(DATAPORT,false);                  // take out of shutdown
  lc.clearDisplay(DATAPORT);                    // clear
  lc.setIntensity(DATAPORT,DATAPORTINTENSITY);  // set intensity
  
  lc.shutdown(CBI,false);                       // take out of shutdown
  lc.clearDisplay(CBI);                         // clear
  lc.setIntensity(CBI,CBIINTENSITY);            // set intensity
  
#ifdef  TEST// test LEDs
  singleTest(); 
  delay(2000);
#endif

pinMode(LeftDoorPin, INPUT_PULLUP);  //Pin on the Arduino Mini Breakout Board connected to left door switch HIGH=Door closed (NC when door closed) - S.Sloan
pinMode(RightDoorPin, INPUT_PULLUP);  //Pin on the Arduino Mini Breakout Board connected to right door switch HIGH=Door closed (NC when door closed) - S.Sloan

#ifndef monitorVCC
  pinMode(analoginput, INPUT);
#endif

}


void loop() 
{ 

int LeftDoorStatus = digitalRead(LeftDoorPin);        // Open(Low)/Closed(High) Status of Dataport Door - S.Sloan
int RightDoorStatus = digitalRead(RightDoorPin);      // Open(Low)/Closed(High) Status of Charge Bay Indicator Door - S.Sloan

// this is the legacy algorythm. Super simple, but very blocky.
#ifdef LEGACY

 for (int row=0; row<6; row++) lc.setRow(DATAPORT,row,random(0,256));
 #ifdef monitorVCC
   for (int row=0; row<4; row++) lc.setRow(CBI,row,random(0,256));
   getVCC();
 #else
   for (int row=0; row<7; row++) lc.setRow(CBI,row,random(0,256));
 #endif
 delay(1000);
 
#else

  // Dataport LED sequence
  if (LeftDoorStatus == LOW) {                      //If the Dataport door is open then set the LEDs - S.Sloan
    // this is the new code. Every block of LEDs is handled independently
   // Serial.print("\nDataPort");
    updateTopBlocks();
    bargraphDisplay(0);
    updatebottomLEDs();
    updateRedLEDs();
    #ifndef BLUELEDTRACKGRAPH
      updateBlueLEDs();
    #endif
  }
  else {                                            //If the Dataport door is closed then switch off the LEDs - S.Sloan
    //switch off all data leds
    lc.setRow(DATAPORT,4,0); // top yellow blocks
    lc.setRow(DATAPORT,5,0); // top green blocks
    lc.setRow(DATAPORT,0,0); // blue LEDs
  }


  //Charge Bay Indicator LED sequence
  if (RightDoorStatus == LOW) {                     //If the CBI door is open then Set the LEDs - S.Sloan
   // Serial.print("\nCBI");
     updateCBILEDs();
     #ifdef monitorVCC
       getVCC();
     #endif
   }
   else {                                           //If the CBI door is closed then switch off the LEDs - S.Sloan
    //switch off all CBI leds
    for (int row=0; row<7; row++) lc.setRow(CBI,row,0);
   }
   
#endif
}


///////////////////////////////////////////////////
// Test LEDs, each Maxim driver row in turn
// Each LED blinks according to the col number
// Col 0 is just on
// Col 1 blinks twice
// col 2 blinks 3 times, etc...
//

#define TESTDELAY 30
void singleTest() 
{
  for(int row=0;row<6;row++) 
  {
    for(int col=0;col<7;col++) 
    {
      delay(TESTDELAY);
      lc.setLed(DATAPORT,row,col,true);
      delay(TESTDELAY);
      for(int i=0;i<col;i++) 
      {
        lc.setLed(DATAPORT,row,col,false);
        delay(TESTDELAY);
        lc.setLed(DATAPORT,row,col,true);
        delay(TESTDELAY);
      }
    }
  }
 
 
 for(int row=0;row<4;row++) 
  {
    for(int col=0;col<5;col++) 
    {
      delay(TESTDELAY);
      lc.setLed(CBI,row,col,true);
      delay(TESTDELAY);
      for(int i=0;i<col;i++) 
      {
        lc.setLed(CBI,row,col,false);
        delay(TESTDELAY);
        lc.setLed(CBI,row,col,true);
        delay(TESTDELAY);
      }
    }
  }
 
 lc.setLed(CBI,4,5,true);
 delay(TESTDELAY);
 lc.setLed(CBI,5,5,true);
 delay(TESTDELAY);
 lc.setLed(CBI,6,5,true);
 delay(TESTDELAY);
}



///////////////////////////////////
// animates the two top left blocks
// (green and yellow blocks)
void updateTopBlocks()
{
  static unsigned long timeLast=0;
  unsigned long elapsed;
  elapsed=millis();
  if ((elapsed - timeLast) < TOPBLOCKSPEED) return;
  timeLast = elapsed; 

  lc.setRow(DATAPORT,4,randomRow(4)); // top yellow blocks
  lc.setRow(DATAPORT,5,randomRow(4)); // top green blocks

}

///////////////////////////////////
// animates the CBI
//
void updateCBILEDs()
{
  static unsigned long timeLast=0;
  unsigned long elapsed;
  elapsed=millis();

  #ifdef ESBmode
    if ((elapsed - timeLast) < CBISPEED_ESB) return;
  #else
    if ((elapsed - timeLast) < CBISPEED_RANDOM) return;
  #endif

  timeLast = elapsed; 

  #ifdef ESBmode
     ESBoperatingSEQ();               // If in ESB Blocky mode then jump to the code to do the LED changes (including the VCC monitor LEDs) - S.Sloan
  #else                               // If not in ESB mode then use the Randow LED function created by VAShadow - S.Sloan
    #ifdef monitorVCC
      lc.setRow(CBI,random(4),randomRow(random(4)));
    #else
      lc.setRow(CBI,random(7),randomRow(random(4)));
    #endif
  #endif
 
}

////////////////////////////////////
// Utility to generate random LED patterns
// Mode goes from 0 to 6. The lower the mode
// the less the LED density that's on.
// Modes 4 and 5 give the most organic feel
byte randomRow(byte randomMode)
{
  switch(randomMode)
  {
    case 0:  // stage -3
      return (random(256)&random(256)&random(256)&random(256));
      break;
    case 1:  // stage -2
      return (random(256)&random(256)&random(256));
      break;
    case 2:  // stage -1
      return (random(256)&random(256));
      break;
    case 3: // legacy "blocky" mode
      return random(256);
      break;
    case 4:  // stage 1
      return (random(256)|random(256));
      break;
    case 5:  // stage 2
      return (random(256)|random(256)|random(256));
      break;
    case 6:  // stage 3
      return (random(256)|random(256)|random(256)|random(256));
      break;
    default:
      return random(256);
      break;
  }
}

//////////////////////
// bargraph for the right column
// disp 0: Row 2 Col 5 to 0 (left bar) - 6 to 0 if including lower red LED, 
// disp 1: Row 3 Col 5 to 0 (right bar)

#define MAXGRAPH 2

void bargraphDisplay(byte disp)
{ 
  static byte bargraphdata[MAXGRAPH]; // status of bars
  
  if(disp>=MAXGRAPH) return;
  
  // speed control
  static unsigned long previousDisplayUpdate[MAXGRAPH]={0,0};

  unsigned long currentMillis = millis();
  if(currentMillis - previousDisplayUpdate[disp] < BARGRAPHSPEED) return;
  previousDisplayUpdate[disp] = currentMillis;
  
  // adjust to max numbers of LED available per bargraph
  byte maxcol;
  if(disp==0 || disp==1) maxcol=6;
  else maxcol=3;  // for smaller graph bars, not defined yet
  
  // use utility to update the value of the bargraph  from it's previous value
  byte value = updatebar(disp, &bargraphdata[disp], maxcol);
  byte data=0;
  // transform value into byte representing of illuminated LEDs
  // start at 1 so it can go all the way to no illuminated LED
  for(int i=1; i<=value; i++) 
  {
    data |= 0x01<<i-1;
  }
  // transfer the byte column wise to the video grid
  fillBar(disp, data, value, maxcol);   
}

/////////////////////////////////
// helper for updating bargraph values, to imitate bargraph movement
byte updatebar(byte disp, byte* bargraphdata, byte maxcol)
{
  // bargraph values go up or down one pixel at a time
  int variation = random(0,3);            // 0= move down, 1= stay, 2= move up
  int value=(int)(*bargraphdata);         // get the previous value
  //if (value==maxcol) value=maxcol-2; else      // special case, staying stuck at maximum does not look realistic, knock it down
  value += (variation-1);                 // grow or shring it by one step
#ifndef BLUELEDTRACKGRAPH
  if (value<=0) value=0;                  // can't be lower than 0
#else
  if (value<=1) value=1;                  // if blue LED tracks, OK to keep lower LED always on
#endif
  if (value>maxcol) value=maxcol;         // can't be higher than max
  (*bargraphdata)=(byte)value;            // store new value, use byte type to save RAM
  return (byte)value;                     // return new value
}

/////////////////////////////////////////
// helper for lighting up a bar of LEDs based on a value
void fillBar(byte disp, byte data, byte value, byte maxcol)
{
  byte row;
  
  // find the row of the bargraph
  switch(disp)
  {
    case 0:
      row = 2;
      break;
    case 1:
      row = 3;
      break;
    default:
      return;
      break;
  }
  
  for(byte col=0; col<maxcol; col++)
  {
    // test state of LED
    byte LEDon=(data & 1<<col);
    if(LEDon)
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,true);  // set column bit
      lc.setLed(DATAPORT,2,maxcol-col-1,true);      // set column bit
      lc.setLed(DATAPORT,3,maxcol-col-1,true);      // set column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,true);      // set blue column bit
    }
    else
    {
      //lc.setLed(DATAPORT,row,maxcol-col-1,false); // reset column bit
      lc.setLed(DATAPORT,2,maxcol-col-1,false);     // reset column bit
      lc.setLed(DATAPORT,3,maxcol-col-1,false);     // reset column bit
      //lc.setLed(DATAPORT,0,maxcol-col-1,false);     // set blue column bit
    }
  }
#ifdef BLUELEDTRACKGRAPH
  // do blue tracking LED
  byte blueLEDrow=B00000010;
  blueLEDrow=blueLEDrow<<value;
  lc.setRow(DATAPORT,0,blueLEDrow);
#endif
}

/////////////////////////////////
// This animates the bottom white LEDs
void updatebottomLEDs()
{
  static unsigned long timeLast=0;
  unsigned long elapsed=millis();
  if ((elapsed - timeLast) < BOTTOMLEDSPEED) return;
  timeLast = elapsed;  
  
  // bottom LEDs are row 1, 
  lc.setRow(DATAPORT,1,randomRow(4));
}

////////////////////////////////
// This is for the two red LEDs
void updateRedLEDs()
{
  static unsigned long timeLast=0;
  unsigned long elapsed=millis();
  if ((elapsed - timeLast) < REDLEDSPEED) return;
  timeLast = elapsed;  
  
  // red LEDs are row 2 and 3, col 6, 
  lc.setLed(DATAPORT,2,6,random(0,2));
  lc.setLed(DATAPORT,3,6,random(0,2));
}

//////////////////////////////////
// This animates the blue LEDs
// Uses a random delay, which never exceeds BLUELEDSPEED 
void updateBlueLEDs()
{
  static unsigned long timeLast=0;
  static unsigned long variabledelay=BLUELEDSPEED;
  unsigned long elapsed=millis();
  if ((elapsed - timeLast) < variabledelay) return;
  timeLast = elapsed;  
  variabledelay=random(10, BLUELEDSPEED);
  
  /*********experimental, moving dots animation
  static byte stage=0;
  stage++;
  if (stage>7) stage=0;
  byte LEDstate=B00000011;
  // blue LEDs are row 0 col 0-5 
  lc.setRow(DATAPORT,0,LEDstate<<stage);
  *********************/
  
  // random
  lc.setRow(DATAPORT,0,randomRow(4));   
}

void getVCC()
{
  value = analogRead(analoginput); // this must be between 0.0 and 5.0 - otherwise you'll let the blue smoke out of your arduino
  vout= (value * 5.0)/1024.0;  //voltage coming out of the voltage divider
  vin = vout / (R2/(R1+R2)); //voltage to display

  lc.setLed(CBI,6,5,(vin >= greenVCC));
  lc.setLed(CBI,5,5,(vin >= yellowVCC));
  lc.setLed(CBI,4,5,(vin >= redVCC));
  //Serial.print("Volt Out = ");                                  // DEBUG CODE
  //Serial.print(vout, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE
  //Serial.print("\tVolts Calc = ");                             // DEBUG CODE
  //Serial.println(vin, 1);   //Print float "vin" with 1 decimal   // DEBUG CODE
}

void heartSEQ() 
  {
  // Step 0
  lc.setRow(0,0,B01110000);
  lc.setRow(0,1,B00100000);
  lc.setRow(0,2,B00100000);
  lc.setRow(0,3,B01110000);
  delay(1000);
  // Step 1
  lc.setRow(0,0,B01010000);
  lc.setRow(0,1,B11111000);
  lc.setRow(0,2,B01110000);
  lc.setRow(0,3,B00100000);
  delay(1000);
  // Step 1
  lc.setRow(0,0,B01010000);
  lc.setRow(0,1,B01010000);
  lc.setRow(0,2,B01010000);
  lc.setRow(0,3,B01110000);
  delay(1000);
  } 


//Charge Bay Indicator ESB LED mode based on Michael Erwin original code - S.Sloan   
void ESBoperatingSEQ() {          // used when ESBmode == true

 static unsigned long ESBCounter = 0;

 switch (ESBCounter)
 {
  case 0:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00000000);
  lc.setRow(CBI,2,B00010000);
  lc.setRow(CBI,3,B00010000);
  break;

  case 1:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00000000);
  lc.setRow(CBI,2,B00010000);
  lc.setRow(CBI,3,B00000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;

  case 2: 
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000);
  lc.setRow(CBI,3,B00000000);
  break;
 
   case 3:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B11000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;
 
  case 4:
  lc.setRow(CBI,0,B00010000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B11000000);
  break;

  case 5:
  lc.setRow(CBI,0,B00010000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B01000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,true); green led on
  #endif
  break;

   case 6:
  lc.setRow(CBI,0,B00010000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000);
  lc.setRow(CBI,3,B00000000);
  break;

  case 7:
  lc.setRow(CBI,0,B00010000);
  lc.setRow(CBI,1,B00000000);
  lc.setRow(CBI,2,B00010000);
  lc.setRow(CBI,3,B00010000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;

  case 8:
  lc.setRow(CBI,0,B00010000);
  lc.setRow(CBI,1,B00110000);
  lc.setRow(CBI,2,B00000000);
  lc.setRow(CBI,3,B00110000);
  break;

  case 9:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00000000); 
  lc.setRow(CBI,3,B00000000);
  break;

  case 10:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00000000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B00000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,true); green led on
    lc.setLed(0,5,5,true); yellow led on
  #endif
  break;

  case 11:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B00000000);
  break;

  case 12:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00000000); 
  lc.setRow(CBI,3,B00010000);
  #ifndef monitorVCC 
      lc.setLed(0,4,5,false); green led off
      lc.setLed(0,5,5,false); yellow led off
  #endif  
  break;

  case 13:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B10110000);
  break;

  case 14:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B01010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B10100000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,true); green led on
  #endif
  break;

  case 15:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B01010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B10000000);
  break;

 case 16:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B00100000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B11000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;

  case 17:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B11000000);
  break;

  case 18:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00000000); 
  lc.setRow(CBI,3,B00000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,true); green led on
  #endif
  break;

  case 19:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B00010000); 
  lc.setRow(CBI,3,B01100000);
  break;

  case 20:
  lc.setRow(CBI,0,B10000000);
  lc.setRow(CBI,1,B10000000);
  lc.setRow(CBI,2,B01010000); 
  lc.setRow(CBI,3,B00000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;

  case 21:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00100000);
  lc.setRow(CBI,2,B01011000); 
  lc.setRow(CBI,3,B01000000);
  break;

  case 22:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00011000);
  lc.setRow(CBI,2,B01011000); 
  lc.setRow(CBI,3,B01000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;

  case 23:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B01011000); 
  lc.setRow(CBI,3,B01000000);
  break;

  case 24:
  lc.setRow(CBI,0,B10001000);
  lc.setRow(CBI,1,B00100000);
  lc.setRow(CBI,2,B01001000); 
  lc.setRow(CBI,3,B01000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,false); green led off
  #endif
  break;

  case 25:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00000000);
  lc.setRow(CBI,2,B00001000); 
  lc.setRow(CBI,3,B00000000);
  break;

  case 26:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B01001000); 
  lc.setRow(CBI,3,B01000000);
  #ifndef monitorVCC 
    lc.setLed(0,4,5,true); green led on
  #endif
  break;

  case 27:
  lc.setRow(CBI,0,B00000000);
  lc.setRow(CBI,1,B00010000);
  lc.setRow(CBI,2,B01001000); 
  lc.setRow(CBI,3,B01000000);
  ESBCounter = -1; // Reset the counter and start sequence again
  break;
  
  default:
  return;
  break;
  }

 ESBCounter++;

}

// =======================================================================================
