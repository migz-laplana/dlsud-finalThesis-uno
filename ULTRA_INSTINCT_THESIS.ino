#include <SPI.h>
#include <MFRC522.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

#define SS_1_PIN 9
#define RST_PIN 7
#define SS_2_PIN 10
#define RST_PIN2 8

#define NR_OF_READERS   2

byte ssPins[] = {SS_1_PIN, SS_2_PIN};
byte rrPins[] = {RST_PIN, RST_PIN2};

MFRC522 mfrc522[NR_OF_READERS];

//NUMBER OF MAX JEEP PASSENGERS = ARRAY VALUES BELOW  
//e.g. current is "5" meaning 5 max passengers

String person[5];
double lat1[5];
double long1[5];
double lat2[5];
double long2[5];
int occupiedSlots = 0;
int gpsPass = 0; 
int sigPass = 0;
int hasDriver = 0;
String driverid;
int readerActive = 0;

//----------THESE ARE FROM THE TINYGPS-----------//
static const int RXPin = 4, TXPin = 3;
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
SoftwareSerial ss(RXPin, TXPin);


 
void setup() 
{
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  Serial.begin(9600);   // Initiate a serial communication
  ss.begin(GPSBaud); //-----------------------FROM TINYGPS, MIGHT CAUSE ERRORS?
  SPI.begin();      // Initiate  SPI bus
  
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
  mfrc522[reader].PCD_Init(ssPins[reader], rrPins[reader]); // Init each MFRC522 card
  }
  Serial.println("Welcome to PUJ Automatic Payment System!");
  Serial.println();
  Serial.println("Tap a Driver's card to assign to this prototype.");
  Serial.println();

  

}
void loop() 
{


//LET DRIVER REGISTER TO PROTOTYPE FIRST!!


while (hasDriver == 0) {
if (  mfrc522[0].PICC_IsNewCardPresent() &&  mfrc522[0].PICC_ReadCardSerial()) 
  {
         digitalWrite(5,HIGH);
         delay(1000);
         digitalWrite(5,LOW);
  } else if (  mfrc522[1].PICC_IsNewCardPresent() &&  mfrc522[1].PICC_ReadCardSerial()) 
  {
         digitalWrite(6,HIGH);
         delay(1000);
         digitalWrite(6,LOW);
  } else {
    return;
  }
  

 String content= "";
  byte letter;
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
  for (byte i = 0; i < mfrc522[reader].uid.size; i++) 
  {
   //  Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    // Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522[reader].uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522[reader].uid.uidByte[i], HEX));
  }
  }
 // Serial.println();
//  Serial.print("Message : ");
  content.toUpperCase();

  Serial.println("Checking if driver card is valid..");
  Serial.print("dvr");    //send a signal to rpi that this is a driverid check
  Serial.println(content.substring(1));
//  delay(3000);
while (Serial.available() <= 0) { delay(1); }

          if (Serial.available() > 0) {
            String data = Serial.readStringUntil('\n');
            if (data.indexOf("INVALID") > 0) {   //if driverid cant be found in database
            Serial.print("Arduino says: ");
            Serial.println(data);
            Serial.flush();
            Serial.println();
            delay(3000);
          }
          else if (data.indexOf("Accepted") > 0) {
            driverid = content.substring(1);
            Serial.print("Arduino says: ");
            Serial.println(data);
            Serial.flush();
            hasDriver = 1;
            Serial.print("Driverid registered: "); Serial.println(driverid);
            Serial.println();
            delay(3000);
            break;
          }
}
//          else {
//            Serial.println("No input from RPi. Check connections.");
//            Serial.println();
//            delay(3000);
//          }

}

  
//--------------MAKE SURE GPS IS WORKING FIRST!! CODE BELOW:
while (gpsPass == 0) {   //make a gps-availability check

  while (ss.available() > 0)
    if (gps.encode(ss.read())) {
      //displayInfo();
 }

  if (millis() > 5000 && gps.charsProcessed() < 10)
  {
    Serial.println("No GPS detected: check wiring and restart.");
    while(true);
  }

  else if (millis() > 5000 && gps.charsProcessed() >= 10)
  {
    Serial.println("GPS detected!");
    Serial.println("Now looking for signal...");
    gpsPass = 1;
  }
}

while (sigPass == 0) {  //make a signal-availability check

  while (ss.available() > 0)
    if (gps.encode(ss.read()))
      if (displayInfo() == true) {
      sigPass = 1; 
      Serial.println("Signal accquired! Ready to operate."); 
      Serial.println();
      break; }
      else { Serial.println("No signal, Searching for signal..");
      delay(2000); }

}

  //-----operational? LEZGO OPERATE

  
  // Look for new cards
  if (  mfrc522[0].PICC_IsNewCardPresent() &&  mfrc522[0].PICC_ReadCardSerial()) 
  {
   readerActive = 1;
  } else if (  mfrc522[1].PICC_IsNewCardPresent() &&  mfrc522[1].PICC_ReadCardSerial()) 
  {
    readerActive = 2;
  } else {
    return;
  }

  //Show UID on serial monitor  -------- REMOVE // WHEN NEEDING ORIGINAL TEXT OF CODE
 // Serial.print("UID tag :");
  String content= "";
  byte letter;
  
  for (byte i = 0; i < mfrc522[(readerActive-1)].uid.size; i++) 
  {
   //  Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    // Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522[(readerActive-1)].uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522[(readerActive-1)].uid.uidByte[i], HEX));
  }
  
 // Serial.println();
//  Serial.print("Message : ");
  content.toUpperCase();

  
  //Serial.print(content.substring(1));   ----THIS IS THE STRING OF THE CARD ID



if (content.substring(1) == driverid)    //if driver signs off while prototype is running
{ hasDriver = 0; driverid = ""; Serial.println("Driver logged out."); delay(3000); return; }

bool wrongReader = true;
for (int x=0;x<5;x++)
  {
    if (  person[x] == content.substring(1) && readerActive==2)  //check for free slots
    {
    wrongReader = false;
    break;
    }
  }
  
    

  for (int x=0;x<5;x++) //CHECK NATIN IF NAKASAKAY NA SIYA
{
  if (person[x] == content.substring(1) && readerActive==2 && ! wrongReader)
  {
         digitalWrite(5,HIGH);
         delay(1000);
         digitalWrite(5,LOW);
      Serial.print(content.substring(1));
      Serial.println("  has left.");
    occupiedSlots -= 1;
    person[x] = "";
    Serial.print("Occupied seats: ");
    Serial.println(occupiedSlots);
                        while (ss.available() > 0) {
                      while (!gps.encode(ss.read())) {
                        delay(1);
                      }

                                    lat2[x] = gps.location.lat();
                                    long2[x] = gps.location.lng();
                                    Serial.print("Location from GPS: ");
                                    Serial.print(lat2[x], 6); Serial.print("///"); Serial.print(long2[x], 6);
        

                                  Serial.println();
                        break;
                    }
    Serial.print("byebye");    //send a signal to rpi that this is the passenger's second tap
    Serial.print(lat1[x], 6); Serial.print("#"); Serial.print(long1[x], 6); Serial.print("#"); Serial.print(lat2[x], 6); Serial.print("#"); Serial.print(long2[x], 6); Serial.print("#"); Serial.print(content.substring(1)); Serial.print("#"); Serial.println(driverid);
    lat1[x] = 0; long1[x] = 0; lat2[x] = 0; long2[x] = 0;
    delay(1500);

    
        //getting data from rpi
  if (Serial.available() > 0) {
    String data = Serial.readStringUntil('\n');
    Serial.print("You sent me: ");
    Serial.println(data);
  }
  Serial.flush();
     Serial.println(); 
    delay(3000);
    return;
  }
}

if (occupiedSlots == 5)  //SET MAX SEATS
{
  Serial.println("JEEP IS FULL. LUMAYAS KA.");
  delay(3000);
  return;
}


// for boarding
bool wrongReader2 = false;
for (int x=0;x<5;x++) //CHECK NATIN IF NAKASAKAY NA SIYA
{
  if (person[x] == content.substring(1) && readerActive==1 )
  {
    wrongReader2 = true;
  }
}
    for (int x=0;x<5;x++)
  {
    if (person[x] == "" && readerActive==1 && ! wrongReader2)  //check for free slots
    {
         digitalWrite(6,HIGH);
         delay(1000);
         digitalWrite(6,LOW);
        
      person[x] = content.substring(1);
      Serial.print(content.substring(1));
      Serial.println("  has boarded.");
      occupiedSlots += 1;
       Serial.print("Occupied seats: ");
      Serial.println(occupiedSlots);
                        while (ss.available() > 0) {
                      while (!gps.encode(ss.read())) {
                        delay(1);
                      }

                                    lat1[x] = gps.location.lat();
                                    long1[x] = gps.location.lng();
                                    Serial.print("Location from GPS: ");
                                    Serial.print(lat1[x], 6); Serial.print("///"); Serial.print(long1[x], 6);
                                                                      

                                  Serial.println();
                        break;
                    }
      Serial.print("firsttap");   //sending a signal to rpi that this is the passenger's first tap
      Serial.println();
      delay(5000);
             //getting data from rpi
          if (Serial.available() > 0) {
            String data = Serial.readStringUntil('\n');
            if (data.indexOf("GET") > 0) {   //unsave passenger if invalid passenger
            occupiedSlots -= 1;
            Serial.print("Occupied seats: ");
            Serial.println(occupiedSlots);
            person[x] = "";
            lat1[x] = 0; long1[x] = 0; lat2[x] = 0; long2[x] = 0;
            }
            Serial.print("Arduino says: ");
            Serial.println(data);
            Serial.flush();
            break;
          }
      Serial.println();
      delay(3000);
      return;
    }
  }


readerActive = 0;
content="";
wrongReader = false;
wrongReader2 = false; 
  
}

  

  



bool displayInfo()
{
  //Serial.print(F("Location: ")); 
  if (gps.location.isValid())
  {
    //Serial.print(gps.location.lat(), 6);
    //Serial.print(F(","));
    //Serial.print(gps.location.lng(), 6);

    sigPass = 1;
    return true;
  }
  else
  {
    return false;
  }

}
