    #include <SoftwareSerial.h>
    #include <Arduino.h>
    #include <LiquidCrystal_I2C.h>
    #include <MFRC522.h>
    #include <SPI.h>

    const int i2c_addr = 0x27;
    LiquidCrystal_I2C lcd(i2c_addr, 16, 2);
    int scrollPos = 0;

    const int bRx = 2;  
    const int bTx = 3;  
    SoftwareSerial BTSerial(bRx, bTx);

    #define RST_PIN 9    
    #define SS_PIN 10    
    MFRC522 mfrc522(SS_PIN, RST_PIN);  
    MFRC522::MIFARE_Key key;

    void printSlowly(String text, int delayTime) {
    for (int i = 0; i < text.length(); i++) {
        lcd.print(text.charAt(i));
        delay(delayTime);
    }
    }

    void hexArrayToString(const byte* hexArray, size_t size, String& output) {
        output = ""; // Clear the output string

        for (size_t i = 0; i < size; i += 2) {
            byte highNibble = hexArray[i];
            byte lowNibble = hexArray[i + 1];

            byte asciiValue = (highNibble << 4) | lowNibble;
            output += (char)asciiValue;
        }
    }

    void printUIDBytes(byte *buffer, byte bufferSize) {
    for (byte i = 0; i < bufferSize; i++) {
        lcd.print(buffer[i] < 0x10 ? " 0" : " ");
        lcd.print(buffer[i], HEX);
    }
    }

    void readCardData() {
    byte block = 4;
    byte hexData[32]; 
    byte textData[18]; 
    byte trailer = 7;
    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailer, &key, &(mfrc522.uid)) == MFRC522::STATUS_OK) {
        byte buffer[32]; 
        byte bufferSize = sizeof(buffer);
        if (mfrc522.MIFARE_Read(block, buffer, &bufferSize) == MFRC522::STATUS_OK) {
        Serial.print("Card Data: "); 
        for (byte i = 0; i < bufferSize; i++) {
            hexData[i] = buffer[i];
            Serial.print(hexData[i], HEX);
        }  
        Serial.println();

        String cardDataString;
        hexArrayToString(hexData, bufferSize, cardDataString);
        
        lcd.clear();
        lcd.setCursor(0, 0);
        printSlowly("Read Text :", 200);
        delay(600);
        lcd.setCursor(0, 1);
        printSlowly(cardDataString, 200); 
        delay(5000);
        lcd.clear();
        } else {
        Serial.println("Failed to read card data.");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Reading failed");
        delay(600);
        lcd.clear();
        }
    } else {
        Serial.println("Authentication failed.");
    }
    }

   void writeDataToCard(String data) {
    byte dataBlock[16];
    byte defBlock[] = {
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00};
        
    byte blockAddr = 4;
    byte trailerBlock = 7;
    byte status;

    Serial.println(F("Authenticating using key A..."));
    status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    Serial.print(F("Writing default block into block "));
    lcd.noBacklight();
    delay(100);
    lcd.backlight();
    Serial.print(blockAddr);
    status = mfrc522.MIFARE_Write(blockAddr, defBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    delay(2000);
    Serial.print(F("Writing data block into block "));
    Serial.print(blockAddr);
    for (size_t i = 0; i < data.length(); i++) {
        byte asciiValue = (byte)data.charAt(i);
        dataBlock[2 * i] = asciiValue >> 4;       // High nibble
        dataBlock[2 * i + 1] = asciiValue & 0x0F; // Low nibble
    }

    // Print the dataBlock
    for (size_t i = 0; i < 16; i++) {
        Serial.print(dataBlock[i], HEX);
    }
    Serial.println();

    status = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);

    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    }

    Serial.println();
    delay(2000);
    return;
}



    void setup() {
    Serial.begin(9600);       
    BTSerial.begin(9600);     
    SPI.begin();                  
    mfrc522.PCD_Init();   
    lcd.init();  
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("RFID AND BLUETOOTH READER");
    lcd.setCursor(0, 1);
    delay(1000);
    lcd.clear();
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;  
    }
    }

    void loop() {
    lcd.clear();
    lcd.setCursor(scrollPos, 0);
    lcd.print("RFID AND BLUETOOTH READER");
    delay(300);
    lcd.setCursor(scrollPos, 1);
    delay(200);
    scrollPos = (scrollPos + 1) % 40;  

    if (BTSerial.available()) {
        String receivedString = BTSerial.readStringUntil('\n');
        Serial.print("Received: ");
        Serial.println(receivedString);
        lcd.noAutoscroll();
        lcd.clear();
        lcd.setCursor(0, 0);
        if (receivedString.startsWith("add to card:")) {
            String dataToAdd = receivedString.substring(13);
            delay(3000);  
            if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
                writeDataToCard(dataToAdd);
            }
        SPI.begin();                  
        mfrc522.PCD_Init(); 
            
        }    
        printSlowly("Received message !", 200);
        delay(600);
        lcd.setCursor(0, 1);
        printSlowly(receivedString, 200);

        delay(5000);  

        lcd.clear();  
        scrollPos = 0;  
    }

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        lcd.clear();
        lcd.print("Card UID: ");
        printUIDBytes(mfrc522.uid.uidByte, mfrc522.uid.size);
        delay(2000);
        Serial.println();
        readCardData();
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
    }
    }
