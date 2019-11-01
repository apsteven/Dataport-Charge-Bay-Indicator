#include <LedControl.h> 

//////////////////////////////////////////////////////////
// CuriousMarc DataPort sketch v1.0
// 12/05/2013
// Alternate sketch for Michael's Erwin DataPort 
// with a more organic feel
// This is just a sketch for the data panel, and it assumes
// it is first in the chain. 
// There is no code for the CBI yet (I don't have one!)
// so someone that has it should write that part...
///////////////////////////////////////////////////////////

// change this to match which Arduino pins you connect your panel to,
// which can be any 3 digital pins you have available. 
// This is the pinout for the DroidConUino, same as the R series
#define DATAIN_PIN 2 
#define CLOCK_PIN  4
#define LOAD_PIN   8

// change this to match your hardware, I have dataport as the device 0 (first in chain)
// and I don't have a CBI, but it would be second in the chain
// Invert the 1 and the 0 if you have them in the other order
#define DATAPORT 0 // the dataport one is first in the chain (device 0)
#define CBI      1 // I don't have a CBI, so I put it in second position (device 1)

// Number of Maxim chips that are connected
#define NUMDEV 2   // One for the dataport, one for the battery indicator

// the dataport is quite dim, so I put it to the maximum
#define DATAPORTINTENSITY 15  // 15 is max


// uncomment this to test the LEDS one after the other at startup
//#define TEST
// This will revert to the old style block animation for comparison
//#define LEGACY

// the timing values below control the various effects. Tweak to your liking.
// values are in ms. The lower the faster.
#define TOPBLOCKSPEED 70
#define BOTTOMLEDSPEED  200
#define REDLEDSPEED     500
#define BLUELEDSPEED    500
#define BARGRAPHSPEED   200

// Uncomment this if you want an alternate effect for the blue LEDs, where it moves
// in sync with the bar graph
//#define BLUELEDTRACKGRAPH

//============================================================================

// Instantiate LedControl driver
LedControl lc=LedControl(DATAIN_PIN,CLOCK_PIN,LOAD_PIN,NUMDEV);   // RSeries FX i2c v5 Module Logics Connector 

void setup() 
{
  Serial.begin(9600);                    

  // initialize Dataport Maxim driver chip
  lc.shutdown(DATAPORT,false);                  // take out of shutdown
  lc.clearDisplay(DATAPORT);                    // clear
  lc.setIntensity(DATAPORT,DATAPORTINTENSITY);  // set intensity
  
#ifdef  TEST// test LEDs
  singleTest(); 
  delay(2000);
#endif

}


void loop() 
{ 

// this is the legacy algorythm. Super simple, but very blocky.
#ifdef LEGACY

 for (int row=0; row<6; row++) lc.setRow(DATAPORT,row,random(0,256));
 delay(1000);
 
#else
// this is the new code. Every block of LEDs is handled independently
 updateTopBlocks();
 bargraphDisplay(0);
 updatebottomLEDs();
 updateRedLEDs();
 #ifndef BLUELEDTRACKGRAPH
 updateBlueLEDs(); 
 #endif
 
#endif
}


///////////////////////////////////////////////////
// Test LEDs, each Maxim driver row in turn
// Each LED blinks according to the col number
// Col 0 is just on
// Col 1 blinks twice
// col 2 blinks 3 times, etc...
//

#define TESTDELAY 300
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
