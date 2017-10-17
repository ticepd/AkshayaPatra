/*--------------JTAG Programmer--------------*/
/*        Version 4
        Started: December-07-2011
        Written By  : Rohit Dureja
                    : Rahul Sharma 
        Remarks     :  1. Only implements
                           -  XCOMPLETE
                           -  XTDOMASK
                           -  XSIR
                           -  XRUNTEST
                           -  XSDRSIZE
                           -  XSDRTDO
                           -  XSTATE
                           -  XREPEAT(i) 
/*----------------CEDT, NSIT----------------*/              
#include <avr/io.h>
#include <util/delay.h>
#define F_CPU 16000000UL

#define TCK  8 
#define TDI  6
#define TDO  7
#define TMS  5 
#define CLK 9

#define MAXSIZE 0x20

/*--------------------------------------------------------------------*/
/*Function Prototypes*/
void xcomplete();
void xtdomask();
void xsir();
void xruntest();
void xsdrsize();
void xsdrtdo();
void xstate();
void xrepeat();
void state(uint8_t);
uint8_t BITS(uint8_t);
void toggle(uint8_t);
void shiftout(uint8_t);
void _delay(uint32_t);
void target_reset();
void initialise_states();
uint8_t BYTES();
uint32_t size_bits();

/*--------------------------------------------------------------------*/
/*Global Variables*/
uint8_t instructionByte;
uint8_t flag=0;            /*To hold if XCOMPLETE is reached*/
uint8_t sdr_bytes=0;         /*Hold DR-size in bytes*/
uint8_t buffer[MAXSIZE];   /*To hold incoming bytes*/
uint8_t tdomask[MAXSIZE];
uint8_t tdo_expected[MAXSIZE];
uint8_t tdo_in[MAXSIZE];
uint32_t runtest_time=0;
uint8_t ir_size=0;
uint8_t current_state;
uint32_t dr_bits;
uint8_t repeat_times=0;

/*--------------------------------------------------------------------*/
/*Binary Encoding of XSVF Instruction*/

#define XCOMPLETE               0x00
#define XTDOMASK                0x01
#define XSIR                    0x02
#define XSDR                    0x03
#define XRUNTEST                0x04
#define XREPEAT                 0x07
#define XSDRSIZE                0x08
#define XSDRTDO                 0x09
#define XSETSDRMASKS            0x0a
#define XSDRINC                 0x0b
#define XSDRB                   0x0c
#define XSDRC                   0x0d
#define XSDRE                   0x0e
#define XSDRTDOB                0x0f
#define XSDRTDOC                0x10
#define XSDRTDOE                0x11
#define XSTATE                  0x12
#define XENDIR                  0x13
#define XENDDR                  0x14
#define XSIR2                   0x15
#define XCOMMENT                0x16
#define XWAIT                   0x17

/*--------------------------------------------------------------------*/
/*State values for XSTATE Command*/

#define STATE_TLR		0x00
#define STATE_RTI		0x01
#define STATE_SELECT_DR_	0x02
#define STATE_CAPTURE_DR	0x03
#define STATE_SHIFT_DR		0x04
#define STATE_EXIT1_DR		0x05
#define STATE_PAUSE_DR		0x06
#define STATE_EXIT2_DR		0x07
#define STATE_UPDATE_DR		0x08
#define STATE_SELECT_IR  	0x09
#define STATE_CAPTURE_IR	0x0a
#define STATE_SHIFT_IR		0x0b
#define STATE_EXIT1_IR		0x0c
#define STATE_PAUSE_IR		0x0d
#define STATE_EXIT2_IR		0x0e
#define STATE_UPDATE_IR		0x0f

/*--------------------------------------------------------------------*/
/*State Transitions Handler*/

//state_route[current_state][next_state];
uint8_t state_route[16][16]={
        /*  0     1     2     3     4     5     6     7     8     9    10    11    12    13    14    15*/
  /*0*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00),  
  /*1*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*2*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*3*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*4*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*5*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*6*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*7*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*8*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*9*/ (0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*10*/(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*11*/(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*12*/(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*13*/(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*14*/(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 
  /*15*/(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
};

void initialise_states()
{
  state_route[STATE_TLR][STATE_RTI]=0x02;
  state_route[STATE_RTI][STATE_SHIFT_DR]=0x09;
  state_route[STATE_SHIFT_DR][STATE_RTI]=0x0B;
  state_route[STATE_RTI][STATE_SHIFT_IR]=0x13;
  state_route[STATE_SHIFT_IR][STATE_RTI]=0x0B;
  state_route[STATE_EXIT1_DR][STATE_RTI]=0b101;
  state_route[STATE_EXIT1_IR][STATE_RTI]=0b101;
}

/*--------------------------------------------------------------------*/
/*Function to determine instruction recieved and perform suitable operation*/

void decodeInstruction()
{  
  switch(instructionByte)
   {
    case XCOMPLETE    :  xcomplete();
                         break;
    case XTDOMASK     :  xtdomask();
                         break;
    case XSIR         :  xsir();
                         break;
    case XRUNTEST     :  xruntest();
                         break;
    case XSDRSIZE     :  xsdrsize();
                         break;
    case XSDRTDO      :  xsdrtdo();
                         break;
    case XSTATE       :  xstate();
                         break;
    case XREPEAT      :  xrepeat();
                         break;
    default           :  break;
  }   
}

/*--------------------------------------------------------------------*/
/*Function to perform instructed state change*/
void state(uint8_t nextstate)
{
  if(nextstate==STATE_TLR)
  {
    target_reset();
    current_state=STATE_TLR;
  }
  else if(current_state!=nextstate)
  {
    uint8_t val;
    int tms;
    int bits;
    int i;
    /*Refers to transition matrix to find TMS conditions required for state change*/
    val=state_route[current_state][nextstate];
    bits=BITS(val);
    digitalWrite(TDI, LOW);
   _delay(1);
    for(i=0;i<bits;i++)
    {
      /*Get TMS value starting from LSB of val*/
      tms=(1<<i) & (val & (1<<i));
      toggle(tms);
    }
    digitalWrite(TMS, LOW);
    _delay(1);
    current_state=nextstate;
  }
}

/*--------------------------------------------------------------------*/
/*Function to toggle states*/
void toggle(uint8_t tms)
{
  digitalWrite(TMS, tms);
  _delay(1);
  digitalWrite(TCK, LOW);
  _delay(1);
  digitalWrite(TCK, HIGH);
  _delay(1);
  digitalWrite(TCK, LOW);
  _delay(1);
}

/*--------------------------------------------------------------------*/
/*Function to return number of BITS in the parameter passed*/
/*MSB of every parameter is 1, it is ignored*/
/*Number of bits = Actual Bits - 1*/
uint8_t BITS(uint8_t val)
{
  if(val<2)
  return 0;
  else if(val<4)
  return 1;
  else if(val<8)
  return 2;
  else if(val<16)
  return 3;
  else if(val<32)
  return 4;
  else if(val<64)
  return 5;
  else if(val<128)
  return 6;
  else if(val<256)
  return 7;
}

/*--------------------------------------------------------------------*/
/*Shift data to DR or IR*/
void shiftout(uint8_t bytes, uint32_t num_bits)
{
  int i,j;
  uint8_t temp;
  uint8_t tdo=0;
  uint8_t in;
  digitalWrite(TMS, LOW);
  _delay(1);
  for(i=bytes-1;i>=0;i--)
  {
    in=0;
    for(j=0;j<8;j++)
    {
      if(num_bits>0)
      {
        if(num_bits==1)
        digitalWrite(TMS, HIGH);
        _delay(1);
        temp=(1<<j) & (buffer[i] & (1<<j));
        
        tdo=digitalRead(TDO);
        _delay(1);
        digitalWrite(TDI, temp);
        _delay(1);
        digitalWrite(TCK, LOW);
        _delay(1);
        digitalWrite(TCK,HIGH);
        _delay(1);
        digitalWrite(TCK, LOW); 
        _delay(1);        
        in|=(tdo<<j);
        num_bits-=1;
      }
    }
    tdo_in[i]=in;
  }
}

/*--------------------------------------------------------------------*/
/*Function to return number of bytes required by x bits*/
uint8_t BYTES()
{
  uint8_t bytes=0;
  bytes = 0;
  bytes |= (buffer[2])<<8;
  bytes |= (buffer[3]);
  bytes = (bytes+7)>>3;
  return bytes;
} 

/*--------------------------------------------------------------------*/
/*Function to return size of IR or DR in bits*/
uint32_t size_bits()
{
  return ((16777216*buffer[0])+(65536*buffer[1])+(256*buffer[2])+buffer[3]);
}
/*--------------------------------------------------------------------*/
/*Functions controlling XSVF instruction*/

void xcomplete()        /*One byte instruction with no parameters*/
{
  flag=1;
  tone(CLK,65535);
  Serial.write('S');
  Serial.end();
}

void xsdrsize()         /*One byte instruction with 1 byte parameter*/
{
  int i=0;
  dr_bits=0;
  while(Serial.available()<4);
  while(i<4)
  {
    buffer[i]=Serial.read();
    i=i+1;
  }
  sdr_bytes=BYTES();
  dr_bits=size_bits();
}

void xtdomask()         /*One byte instruction with sdr_bytes parameter*/
{
  int i=0;
  while(Serial.available()<sdr_bytes);
  while(i<sdr_bytes)
  {
    tdomask[i]=Serial.read();
    i=i+1;
  }
}

void xstate()
{
  while(Serial.available()<1);
  buffer[0]=Serial.read();
    state(buffer[0]);  
}

void xruntest()         /*One byte instruction with 4 byte parameter*/
{
  int i=0;
  while(Serial.available()<4);
  while(i<4)
  {
    buffer[i]=Serial.read();
    i=i+1;
  }
  runtest_time=size_bits(); 
}

void xsir()            /*One byte instruction followed by ir_size bytes parameters*/
{
  int i=0;
  uint32_t ir_bits=0;
  while(i<4)
  {
    if(Serial.available())
    {
      buffer[i]=Serial.read();
      i=i+1;
    }
  }

  ir_size=BYTES();
  ir_bits=size_bits();
  i=0;
  while(i<ir_size)
  {
    if(Serial.available())
    {
      buffer[i]=Serial.read();
      i=i+1;
    }
  }
  state(STATE_SHIFT_IR);
  shiftout(ir_size,ir_bits);
  current_state=STATE_EXIT1_IR;
  state(STATE_RTI);
  _delay(runtest_time);
}

void xsdrtdo()        /*One byte instruction followed by sdr_bytes*2 parameters*/
{
  uint8_t actual;
  uint8_t expected;
  uint8_t fail_times=0;
  int i=0;
  while(Serial.available()<sdr_bytes);
  _delay(1000);
  while(i<sdr_bytes)
  {
    buffer[i]=Serial.read();
    i=i+1;
  }
  i=0;
  while(Serial.available()<sdr_bytes);
  _delay(1000);
  while(i<sdr_bytes)
  {
    tdo_expected[i]=Serial.read();
    i=i+1;
  }
  while(fail_times<repeat_times)
  {
    flag=0;
    state(STATE_SHIFT_DR);
    shiftout(sdr_bytes, dr_bits);
    current_state=STATE_EXIT1_DR;
    i=0;
    while(i<sdr_bytes)
    {
      actual=tdo_in[i]&tdomask[i];
      expected=tdo_expected[i]&tdomask[i];
      i=i+1;
      if(actual!=expected)
      {
        flag=1;
        fail_times=fail_times+1;
        state(STATE_RTI);
        _delay(runtest_time);
        break;
      }
    }
    if(flag==0)
    {
      state(STATE_RTI);
      _delay(runtest_time);
      break;
    }
  }     
}

void xrepeat()
{
  while(Serial.available()<1);
  repeat_times=Serial.read();
}

/*--------------------------------------------------------------------*/
/*Wait atleast the specified number of microseconds*/
void _delay(uint32_t microseconds)
{
  microseconds*=1;
  while(microseconds > 0)
  {
    _delay_us(1);
    microseconds-=1;
  }
}

/*--------------------------------------------------------------------*/
/*Guaranteed TAP sequence to Test-Logic-Reset*/
void target_reset()
{
   int i=0;
  digitalWrite(TMS, HIGH);
  _delay(1);
  for(i=0;i<6;i++)
  {
    digitalWrite(TCK, LOW);
    _delay(1);
    digitalWrite(TCK, HIGH);
    _delay(1);
    digitalWrite(TCK, LOW);
    _delay(1);
  }
  digitalWrite(TMS, LOW);
  _delay(1);
  current_state=STATE_TLR;
}

/*--------------------------------------------------------------------*/
void setup()
{
  pinMode(CLK, OUTPUT);
  pinMode(TCK, OUTPUT);
  pinMode(TMS, OUTPUT);
  pinMode(TDI, OUTPUT);
  pinMode(TDO, INPUT);
  Serial.begin(250000);
  while (!Serial) ;
  target_reset();
  initialise_states();
  Serial.write('S');
}

/*--------------------------------------------------------------------*/
void loop()
{
  while(Serial.available()<1);
  instructionByte=Serial.read();
  //_delay(1000);
  decodeInstruction();
  if(flag==1)
  {
    Serial.write('F');
    Serial.end();
  }
  else if(flag==0)
  Serial.write('S');
}
