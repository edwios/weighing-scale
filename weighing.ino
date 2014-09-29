/****************************************************************************
*   Electronic scale using HX711 as the ADC with built-in high-gain         *
*   OpAmp and display on a local OLED display.                              *
****************************************************************************/
extern char* itoa(int a, char* buffer, unsigned char radix);

// Time for each Deep sleep
#define PERIOD 300000
// Max size of the buffer to calculate the average
#define stackMax 12
// Set UPDATE2CLOUD to true if you want the measurement sent to the cloud
#define UPDATE2CLOUD false 

int version=120;

// Pin out defs
int led = D7;
int ADSK = D5;
int ADDO = D6;
int TARE = D4;

// Not so useful init defaults. Will be overrided in later stages
unsigned long offset = 8534000;
unsigned long scale = 419000/178;
unsigned long refWeight = 141;

// Common variables
unsigned long weighStack[stackMax];
unsigned long rawWeight = 0;
int read = 0;
bool isCalMode = false;
bool weightUpdated;
int stackIndex = 0;

// Thingspeak.com API
TCPClient client;
const char * WRITEKEY = "THINGSPEAK_API_WRITEKEY";
const char * serverName = "api.thingspeak.com";
IPAddress server = {184,106,153,149};

// 1.3" 12864 OLED with SH1106 and Simplified Chinese IC for Spark Core
int Rom_CS = A2;
unsigned long  fontaddr=0;
char dispCS[32];

/****************************************************************************
*****************************************************************************
****************************   OLED Driver  *********************************
*****************************************************************************
****************************************************************************/


/*****************************************************************************
 Funtion    :   OLED_WrtData
 Description:   Write Data to OLED
 Input      :   byte8 ucCmd  
 Output     :   NONE
 Return     :   NONE
*****************************************************************************/
void transfer_data_lcd(byte ucData)
{
   Wire.beginTransmission(0x78 >> 1);     
   Wire.write(0x40);      //write data
   Wire.write(ucData);
   Wire.endTransmission();
}

/*****************************************************************************
 Funtion    :   OLED_WrCmd
 Description:   Write Command to OLED
 Input      :   byte8 ucCmd  
 Output     :   NONE
 Return     :   NONE
*****************************************************************************/
void transfer_command_lcd(byte ucCmd)
{
   Wire.beginTransmission(0x78 >> 1);            //Slave address,SA0=0
   Wire.write(0x00);      //write command
   Wire.write(ucCmd); 
   Wire.endTransmission();
}


/* OLED Initialization */
void initial_lcd()
{
  digitalWrite(Rom_CS, HIGH);
  Wire.begin();
  delay(20);        
  transfer_command_lcd(0xAE);   //display off
  transfer_command_lcd(0x20); //Set Memory Addressing Mode  
  transfer_command_lcd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  transfer_command_lcd(0xb0); //Set Page Start Address for Page Addressing Mode,0-7
  transfer_command_lcd(0xc8); //Set COM Output Scan Direction
  transfer_command_lcd(0x00);//---set low column address
  transfer_command_lcd(0x10);//---set high column address
  transfer_command_lcd(0x40);//--set start line address
  transfer_command_lcd(0x81);//--set contrast control register
  transfer_command_lcd(0x7f);
  transfer_command_lcd(0xa1);//--set segment re-map 0 to 127
  transfer_command_lcd(0xa6);//--set normal display
  transfer_command_lcd(0xa8);//--set multiplex ratio(1 to 64)
  transfer_command_lcd(0x3F);//
  transfer_command_lcd(0xa4);//0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  transfer_command_lcd(0xd3);//-set display offset
  transfer_command_lcd(0x00);//-not offset
  transfer_command_lcd(0xd5);//--set display clock divide ratio/oscillator frequency
  transfer_command_lcd(0xf0);//--set divide ratio
  transfer_command_lcd(0xd9);//--set pre-charge period
  transfer_command_lcd(0x22); //
  transfer_command_lcd(0xda);//--set com pins hardware configuration
  transfer_command_lcd(0x12);
  transfer_command_lcd(0xdb);//--set vcomh
  transfer_command_lcd(0x20);//0x20,0.77xVcc
  transfer_command_lcd(0x8d);//--set DC-DC enable
  transfer_command_lcd(0x14);//
  transfer_command_lcd(0xaf);//--turn on oled panel 

}

void lcd_address(byte page,byte column)
{

  transfer_command_lcd(0xb0 + column);   /* Page address */
  transfer_command_lcd((((page + 1) & 0xf0) >> 4) | 0x10);  /* 4 bit MSB */
  transfer_command_lcd(((page + 1) & 0x0f) | 0x00); /* 4 bit LSB */ 
}

void clear_screen()
{
  unsigned char i,j;
  digitalWrite(Rom_CS, HIGH); 
  for(i=0;i<8;i++)
  {
    transfer_command_lcd(0xb0 + i);
    transfer_command_lcd(0x00);
    transfer_command_lcd(0x10);
    for(j=0;j<132;j++)
    {
        transfer_data_lcd(0x00);
    }
  }

}

void display_128x64(byte *dp)
{
  unsigned int i,j;
  for(j=0;j<8;j++)
  {
    lcd_address(0,j);
    for (i=0;i<132;i++)
    { 
      if(i>=2&&i<130)
      {
          // Write data to OLED, increase address by 1 after each byte written
        transfer_data_lcd(*dp);
        dp++;
      }
    }

  }

}



void display_graphic_16x16(unsigned int page,unsigned int column,byte *dp)
{
  unsigned int i,j;

  digitalWrite(Rom_CS, HIGH);   
  for(j=2;j>0;j--)
  {
    lcd_address(column,page);
    for (i=0;i<16;i++)
    { 
      transfer_data_lcd(*dp);
      dp++;
    }
    page++;
  }
}


void display_graphic_8x16(unsigned int page,byte column,byte *dp)
{
  unsigned int i,j;
  
  for(j=2;j>0;j--)
  {
    lcd_address(column,page);
    for (i=0;i<8;i++)
    { 
      // Write data to OLED, increase address by 1 after each byte written
      transfer_data_lcd(*dp);
      dp++;
    }
    page++;
  }

}


/*
    Display a 5x7 dot matrix, ASCII or a 5x7 custom font, glyph, etc.
*/
    
void display_graphic_5x7(unsigned int page,byte column,byte *dp)
{
  unsigned int col_cnt;
  byte page_address;
  byte column_address_L,column_address_H;
  page_address = 0xb0 + page - 1;// 
  
  
  
  column_address_L =(column&0x0f);  // -1
  column_address_H =((column>>4)&0x0f)+0x10;
  
  transfer_command_lcd(page_address);     /*Set Page Address*/
  transfer_command_lcd(column_address_H); /*Set MSB of column Address*/
  transfer_command_lcd(column_address_L); /*Set LSB of column Address*/
  
  for (col_cnt=0;col_cnt<6;col_cnt++)
  { 
    transfer_data_lcd(*dp);
    dp++;
  }
}

/**** Send command to Character ROM ***/
void send_command_to_ROM( byte datu )
{
  SPI.transfer(datu);
}

/**** Read a byte from the Character ROM ***/
byte get_data_from_ROM( )
{
  byte ret_data=0;
  ret_data = SPI.transfer(255);
  return(ret_data);
}


/* 
*     Read continuously from ROM DataLen's bytes and 
*     put them into pointer pointed to by pBuff
*/

void get_n_bytes_data_from_ROM(byte addrHigh,byte addrMid,byte addrLow,byte *pBuff,byte DataLen )
{
  byte i;
  digitalWrite(Rom_CS, LOW);
  delayMicroseconds(100);
  send_command_to_ROM(0x03);
  send_command_to_ROM(addrHigh);
  send_command_to_ROM(addrMid);
  send_command_to_ROM(addrLow);

  for(i = 0; i < DataLen; i++ ) {
       *(pBuff+i) =get_data_from_ROM();
  }
  digitalWrite(Rom_CS, HIGH);
}


/******************************************************************/

void display_string_5x7(byte y,byte x,const char *text)
{
  unsigned char i= 0;
  unsigned char addrHigh,addrMid,addrLow ;
  while((text[i]>0x00))
  {
    
    if((text[i]>=0x20) &&(text[i]<=0x7e)) 
    {           
      unsigned char fontbuf[8];     
      fontaddr = (text[i]- 0x20);
      fontaddr = (unsigned long)(fontaddr*8);
      fontaddr = (unsigned long)(fontaddr+0x3bfc0);     
      addrHigh = (fontaddr&0xff0000)>>16;
      addrMid = (fontaddr&0xff00)>>8;
      addrLow = fontaddr&0xff;

      get_n_bytes_data_from_ROM(addrHigh,addrMid,addrLow,fontbuf,8);/*取8个字节的数据，存到"fontbuf[32]"*/
      
      display_graphic_5x7(y,x+1,fontbuf);/*显示5x7的ASCII字到LCD上，y为页地址，x为列地址，fontbuf[]为数据*/
      i+=1;
      x+=6;
    }
    else
    i++;  
  }
  
}


/****************************************************************************
*****************************************************************************
****************************  Data Upload   *********************************
*****************************************************************************
****************************************************************************/

void sendToThingSpeak(const char * key, String mesg)
{
    client.stop();
    String outMesg = String("field1="+mesg);
    RGB.control(true);
    RGB.color(0,255,0);
    if (client.connect(server, 80)) {
        client.print("POST /update");
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(serverName);
        client.println("User-Agent: Spark");
        client.println("Connection: close");
        client.print("X-THINGSPEAKAPIKEY: ");
        client.println(key);
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.print("Content-length: ");
        client.println(outMesg.length());
        client.println();
        client.print(outMesg);
        client.flush();
        RGB.control(false);
    } 
    else{
        RGB.color(255,0,0);
    }
}

/****************************************************************************
*****************************************************************************
*************************  Weighing functions  ******************************
*****************************************************************************
****************************************************************************/


/****************************************************************
*
*   resetADOutput()
*   Physically read the output from the ADC.
*   Details see the datasheet.
*
*****************************************************************/

unsigned long readADOutput()
{
    noInterrupts();
    unsigned long Count;
    digitalWrite(ADSK, LOW); //Start measuring (PD_SCK LOW)
    Count=0;
    while(digitalRead(ADDO)); //Wait until ADC is ready
    for (int i=0;i<24;i++)
    {
        digitalWrite(ADSK, HIGH); //PD_SCK HIGH (Start) 
        Count=Count<<1; //Shift Count left at falling edge
        digitalWrite(ADSK, LOW); //PD_SCK LOW
        if(digitalRead(ADDO)) Count++;
    }
    digitalWrite(ADSK, HIGH); 
    delayMicroseconds(5);
    Count = Count^0x800000; //This is the 25th pulses, gain set to 128
    digitalWrite(ADSK, LOW);
    interrupts();

    return(Count);
}

int myVersion(String command)
{
    return version;
}


/****************************************************************
*
*   resetAD()
*   Long pulse will cause the ADC to reset
*   Details see the datasheet.
*
*****************************************************************/

void resetAD()
{
    digitalWrite(ADSK, HIGH); 
    delayMicroseconds(80);
    digitalWrite(ADSK, LOW); 
}

void pushWeighStack(unsigned long val)
{
    weighStack[stackIndex++] = val;
    if (stackIndex >= stackMax) stackIndex = 0;
}

/****************************************************************
*
*   avgWeighStack()
*   Calculate the average weight from the values stored in
*   the stack.
*
*****************************************************************/

unsigned long avgWeighStack()
{
    int sum = 0;
    int i = 0;
    for (i = 0; i < stackMax; i++) {
        sum += weighStack[i];
    }
    return (sum / i);
}

/****************************************************************
*
*   ADRead()
*   Read the average value from ADC and adjust the reading by
*   the necessary offset, then scale it to the correct weight
*
*****************************************************************/

unsigned long ADRead()
{
    char dp[32];
    weightUpdated = false;
    pushWeighStack(readADOutput());
    unsigned long avg = avgWeighStack();
    unsigned long weight = abs(avg - offset) / 1000 * scale / 1000;
    if (weight < 5000) {
        weightUpdated = true;
        noInterrupts();
        display_string_5x7(5,1,"              ");
        interrupts();
    } else {
        sprintf(dp, "Error: %d      ", weight);
        noInterrupts();
        display_string_5x7(5,1,dp);
        dp[0] = '\0';
        interrupts();
    }
    return weight;
}

/****************************************************************
*
*   ADReadAssured()
*   Read the average weight by trying to read from the average
*   weight at least 4 times and upto 100 times or when value
*   read become stable.
*
*****************************************************************/

unsigned long ADReadAssured()
{
    unsigned long lastRead = 0;
    int delta = 0;
    int i = 0;
    lastRead = ADRead();
    while ((i < 4) || ((i < 100) && abs(ADRead() - lastRead) > 0)) {
        lastRead = ADRead();
        i++;
    }
    return lastRead;
}

void tare()
{
    offset = avgWeighStack();
}

int cal(String cmd)
{
  // Calibrate the scale using the value supplied in cmd
  // Login below dictates the calibre must be heavier than 50g

    // First, prevent scale update
    isCalMode = true;
    
    char dp[32];
    
    int v = cmd.toInt();
    if (v != 0) refWeight = v;
    
    // then set zero (tare)
    while (ADRead() > 5) { // weight not zero, something still on it?
        display_string_5x7(4,1,"Remove things on top");
        delay(500);
    }
    tare();
    sprintf(dp, "Put %u g on    ", refWeight);
    display_string_5x7(4,1,    dp);
    unsigned long oldReading = ADReadAssured();
    while (oldReading < 50) { // we need more than 50g for calibration
        display_string_5x7(4,1,dp);
        delay(500);
        oldReading = ADReadAssured();
    }
    scale = refWeight * 1000 / oldReading * scale / 1000;
    sprintf(dp, "Scale: %u      ", scale);
    display_string_5x7(4,1,dp);
    isCalMode = false;
    
    return 1;
}

/****************************************************************************
*****************************************************************************
**************************  Initialization  *********************************
*****************************************************************************
****************************************************************************/

void setup()
{
    Time.zone(8);
  // Register a Spark variable here
    Spark.variable("version", &version, INT);
    Spark.function("version", myVersion);
    Spark.function("calibrate", cal);

  // Connect the temperature sensor to A7 and configure it
  // to be an input
    pinMode(ADDO, INPUT);
    pinMode(ADSK, OUTPUT);
    pinMode(led, OUTPUT);
    pinMode(TARE, INPUT);

 
    SPI.begin();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE3);
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    digitalWrite(Rom_CS, HIGH);
    attachInterrupt(TARE, tare, FALLING);

    resetAD();
    initial_lcd();  
    clear_screen();    //clear all dots
    digitalWrite(led, LOW);
    display_string_5x7(4,1,"(c)2014 ioStation");
    sprintf(dispCS, "Version: %d", version);
    display_string_5x7(6,1,dispCS);
    delay(1000);
    clear_screen();    //clear all dots
    sprintf(dispCS, "v%d", version);
    display_string_5x7(7,1,dispCS);

}

/****************************************************************************
*****************************************************************************
**************************  Main Proc Loop  *********************************
*****************************************************************************
****************************************************************************/

void loop()
{
        digitalWrite(led, HIGH);
        if (!isCalMode) {
            sprintf(dispCS, "Value: %u g      ", ADReadAssured());
            if (weightUpdated) {
                noInterrupts();
                display_string_5x7(4,1,dispCS);
                String sWeight = String(dispCS);
                if (UPDATE2CLOUD) {
                    display_string_5x7(1,1, "Updating            ");
                    sendToThingSpeak(WRITEKEY, sWeight);
                }
            }
        }
        Time.timeStr().substring(4,19).toCharArray(dispCS, 16);
        display_string_5x7(1,1,"     ");
        display_string_5x7(1,20,dispCS);
        interrupts();
        digitalWrite(led, LOW);
        delay(500);
//        Spark.sleep(SLEEP_MODE_DEEP, PERIOD);
}


