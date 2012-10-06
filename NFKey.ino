//This example reads a MIFARE memory block. It is tested with a new MIFARE 1K cards. Uses default keys.
//Contributed by Seeed Technology Inc (www.seeedstudio.com)

#include <PN532.h>
/*
 * Corrected MISO/MOSI/SCK for Mega from Jonathan Hogg (www.jonathanhogg.com)
 * SS is the same, due to NFC Shield schematic
 */
#define SS 10
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define MISO 50
#define MOSI 51
#define SCK 52
#else
#define MISO 12
#define MOSI 11
#define SCK 13
#endif

//Skip Ndef headers
#define OFFSET 11

//Define LEDS
#define LED_GREEN 17
#define LED_RED 18

//Define Buffer sizes for message
#define ID_BUFFER_SIZE 32
#define EXTRA_BUFFER_SIZE 1024
// TODO
// This should be bigger relative to what we store in extra
#define READER_BUFFER_SIZE 256

//Our id
#define MYID "room42"

PN532 nfc(SCK, MISO, MOSI, SS);

void parse(char buf[], char id[ID_BUFFER_SIZE], int* start, int* stop, char extra[EXTRA_BUFFER_SIZE])
{
  int segment=0;
  //WE have 4 segments to read, id, start, stop, and extra string
  int index = 0;
  int start_index = 0;
  int stop_index = 0;
  for(int i=0; i<4; i++){
    for(int j=0;buf[index] != '\0' && buf[index] != ',' && index < READER_BUFFER_SIZE;index++,j++){
      switch(segment){
      case 0: // ID segment
        if (index > ID_BUFFER_SIZE){
          Serial.println("Id received was too long or data is corrupted");
          return;
        }
        id[j] = buf[index];
        break;
      case 1: // Store start index for start segment
        if(j == 0){
          start_index = index;
        }
        // This is wrong
        break;
      case 2: // end segment
        if(j == 0){
          //Replace comma by null char so atoi works
          buf[index-1] = '\0';
          *start = atoi(&buf[start_index]);
          stop_index = index;
        }

        break;
      case 3: // extra segment
        if(j == 0){
          //Replace comma by null char so atoi works
          buf[index-1] = '\0';
          *stop = atoi(&buf[stop_index]);
        }
        extra[j] = buf[index];
        break;
      }
    }
    //Skip the end character and go to next segment
    if (buf[index] == ','){
      index++;
      //Go to next segment
      segment ++;
    }
    else{
      return;
    }
  }
} 

boolean is_valid(char buf[]){
  //Static to empty arrays before use
  static char id[ID_BUFFER_SIZE];
  int start=0, stop=0;
  static char extra[EXTRA_BUFFER_SIZE];

  //TODO
  //current_time should be set in some other way
  int current_time = 0;

  parse(buf, id, &start, &stop, extra);

  if(strcmp(id, MYID) == 0 && start <= current_time && stop >= current_time){
    return true;
  }
  return false;
}


void setup(void) {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); 
  Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); 
  Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); 
  Serial.println((versiondata>>8) & 0xFF, DEC);
  Serial.print("Supports "); 
  Serial.println(versiondata & 0xFF, HEX);

  // configure board to read RFID tags and cards
  nfc.SAMConfig();
}


void loop(void) {
  uint32_t id;
  // look for MiFare type cards
  id = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A);

  if (id != 0) 
  {
    Serial.print("Read card #"); 
    Serial.println(id);


    //read memory block 0x08
    char buf[READER_BUFFER_SIZE];
    int counter = 0;
    boolean finish = false;
    for (int j=0x04;j<0x20 && !finish;j++)
    {
      uint8_t keys[]= {
        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF                        };
      if(nfc.authenticateBlock(1, id ,j ,KEY_B,keys)) //authenticate block 0x08
      {
        //if authentication successful
        uint8_t block[16];
        if(nfc.readMemoryBlock(1, j, block))
        {
          //if read operation is successful
          for(uint8_t i=0;i<16;i++)
          {
            //Data ends with FE character
            if (block[i] == 0xFE){
              finish=true;
              break; 
            }
            //print memory block
            buf[counter] = block[i];
            counter++;
            Serial.print(block[i],HEX);
            Serial.print(" ");
          }
          Serial.println();
        }
        Serial.println();
      }
      else{
        Serial.println("Cannot authenticate.");
      }
    }
    buf[counter] = '\0';
    //Skip Ndef Header
    char* buf2;
    buf2 = buf + OFFSET;
    Serial.println(buf2);
    if(finish){
      if(is_valid(buf2)){
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
      }
      else{
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
      }
      delay(2000);
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, LOW);
    }
  }

  delay(500);
}

