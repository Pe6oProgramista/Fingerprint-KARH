// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)

#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3);

const int greenLedPin = A5;
const int redLedPin = A3;
const int yellowLedPin = A4;
const int buttonPin = A2;
const int activate = A1;

int pushCount;
int buttonState = 0;
int lastButtonState = 1;
int stat = 0;
unsigned long milsec;

int powerOn = 0;

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

void setup()  
{
  pinMode(greenLedPin, OUTPUT); // Green
  pinMode(redLedPin,OUTPUT); // Red
  pinMode(yellowLedPin, OUTPUT); // Yellow
  pinMode(buttonPin, INPUT);
  pinMode(activate, OUTPUT);

  digitalWrite(activate, HIGH);
  
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("Adafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
}

void loop()
{
  pushCount = 0;
  stat = 0;
  Serial.println("Push button.");
  while(1) {
    buttonState = digitalRead(buttonPin);
    if (buttonState != lastButtonState) {
      if (buttonState == HIGH) {
        Serial.println("button");
        milsec = millis();
        stat = 1;
        pushCount++;
      }
      delay(110);
    }
    lastButtonState = buttonState;
    if(stat == 1  && (millis() - milsec > 1110)) {
       break;
    }
  }
  switch(pushCount) {
    case 1: //unlock
      verify(0);
      break;
    case 2: //add fingerprint
      add();
      break;
    case 3: //delete fingerprint
      deleteTemplate();
      break;
    case 4: //show all fingerprints
      showFingerpintTemplate();
      break;
    case 5: //delete all fingerprints
      //finger.emptyDatabase();
      for(int id = 3; id <= 127; id++) {
        deleteFingerprint(id);
      }
      digitalWrite(greenLedPin, HIGH);
      digitalWrite(redLedPin, LOW);
      digitalWrite(yellowLedPin, LOW);
      delay(2500);
      digitalWrite(greenLedPin, LOW);
      break;
  }
}

void add() {
  if(verify(1)) {
    int freeID = 0;
    for(int id = 1; id <= 127; id++) {
      uint8_t p = finger.loadModel(id);
      if(p != FINGERPRINT_OK) {
        freeID = id;
        break;
      }
    }
    if(!freeID) {
      Serial.println("DB is full!");
    }
    else {
      getFingerprintEnroll(freeID);
    }
  }
}

int getFingerprintEnroll(int id) {

  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    int p = finger.getImage();
    if (p == FINGERPRINT_OK)  {
      Serial.println("Image taken");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK)  return -1;
  Serial.println("Image converted!");
  
  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    int p = finger.getImage();
    if (p == FINGERPRINT_OK)  {
      Serial.println("Image taken");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK)  return -1;
  Serial.println("Image converted!");
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else {
    digitalWrite(redLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    delay(2500);
    digitalWrite(redLedPin, LOW);
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
      Serial.println("Fingerprints did not match");
      return p;
    } else {
      Serial.println("Unknown error");
      return p;
    }
  }
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    delay(2500);
    digitalWrite(greenLedPin, LOW);
  } else {
    digitalWrite(redLedPin, HIGH);
    digitalWrite(greenLedPin, LOW);
    digitalWrite(yellowLedPin, LOW);
    delay(2500);
    digitalWrite(redLedPin, LOW);
    if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
      return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
      Serial.println("Could not store in that location");
      return p;
    } else if (p == FINGERPRINT_FLASHERR) {
      Serial.println("Error writing to flash");
      return p;
    } else {
      Serial.println("Unknown error");
      return p;
    }
  }
}

void deleteTemplate() {
  if(verify(1)) {
    while(1) {
      int id = getFingerprintID();
      if(id > 0) {
        if(id == 1 || id == 2) {
          Serial.println("Can't delete master fingerprint!");
          digitalWrite(redLedPin, HIGH);
          digitalWrite(greenLedPin, LOW);
          digitalWrite(yellowLedPin, LOW);
          delay(2500);
          digitalWrite(redLedPin, LOW);
        }
        else {
          deleteFingerprint(id);
          digitalWrite(greenLedPin, HIGH);
          digitalWrite(redLedPin, LOW);
          digitalWrite(yellowLedPin, LOW);
          delay(2500);
          digitalWrite(greenLedPin, LOW);
          break;
        }
      }
      else if(id == -2) {
        Serial.println("Nothing to delete!");
        digitalWrite(redLedPin, HIGH);
        digitalWrite(greenLedPin, LOW);
        digitalWrite(yellowLedPin, LOW);
        delay(2500);
        digitalWrite(redLedPin, LOW);
        break;
      }
    }
  }
}

int deleteFingerprint(int id) {
  int p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   
}

int verify(int isMaster) {
    int tries = 3;
    Serial.println("Waiting for valid finger...");
    while(1) {
      if(!tries) {
        digitalWrite(yellowLedPin, LOW);
        break;
      }
      int result = getFingerprintID();
      if(result > 0) {
        if(isMaster) {
          if(result == 1 || result == 2) {
            digitalWrite(greenLedPin, HIGH);
            digitalWrite(redLedPin, LOW);
            digitalWrite(yellowLedPin, LOW);
            delay(2000);
            digitalWrite(greenLedPin, LOW);
            return 1;
          }
          else {
            Serial.println("Not master!");
            digitalWrite(redLedPin, HIGH);
            digitalWrite(greenLedPin, LOW);
            digitalWrite(yellowLedPin, LOW);
            delay(2500);
            Serial.println("Waiting for valid finger...");
            tries--;
          }
        }
        else {
          digitalWrite(greenLedPin, HIGH);
          digitalWrite(redLedPin, LOW);
          digitalWrite(yellowLedPin, LOW);
          digitalWrite(activate, LOW);
          delay(200);
          if(powerOn) {
            powerOn = 0;
            delay(4800);
          }
          else {
            powerOn = 1;
          }
          digitalWrite(activate, HIGH);
          delay(2300);
          digitalWrite(greenLedPin, LOW);
          break;
        }
      }
      else if(result == -2){
        digitalWrite(redLedPin, HIGH);
        digitalWrite(greenLedPin, LOW);
        digitalWrite(yellowLedPin, LOW);
        delay(2500);
        Serial.println("Waiting for valid finger...");
        tries--;
      }
      digitalWrite(yellowLedPin, HIGH);
      digitalWrite(greenLedPin, LOW);
      digitalWrite(redLedPin, LOW);
      
      delay(500);
  }
  return 0;
}

// returns -1 if failed, otherwise returns ID #
int getFingerprintID() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;
  Serial.println("Image taken");
  
  p = finger.image2Tz(); 
  if (p != FINGERPRINT_OK)  return -1;
  Serial.println("Image converted!");
  
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  {
    Serial.println("Not correct!");    return -2;
  }
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  return finger.fingerID;
}


// Show DB
void showFingerpintTemplate()
{
  for(int id = 1; id <= 127; id++) {
    uint8_t p = finger.loadModel(id);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.print("\ntemplate "); Serial.print(id); Serial.println(" loaded");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error ID #"); Serial.println(id);
        break;
      default:
        Serial.print("Empty ID #"); Serial.println(id);
        break;
    }
  }
}
