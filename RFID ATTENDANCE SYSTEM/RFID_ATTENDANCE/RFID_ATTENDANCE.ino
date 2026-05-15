#include <SPI.h>
#include <MFRC522.h>

constexpr uint8_t RST_PIN = D3;  // 🔄 Reset pin for RFID
constexpr uint8_t SS_PIN = D4;   // 🛠 Slave Select pin for RFID

MFRC522 mfrc522(SS_PIN, RST_PIN);  // 📡 Create MFRC522 instance
MFRC522::MIFARE_Key key;           // 🔐 Key for authentication

int blockNum = 4;  // 💾 Block to store data (16 bytes max)

byte readBlockData[18];  // 📥 Buffer for reading data (18 bytes needed)
byte bufferLen = 18;
MFRC522::StatusCode status;  // 🚦 Status code for RFID operations

// ✍️ Function to write data to RFID card
void WriteDataToBlock(int blockNum, String data) {
byte blockData[16];  
memset(blockData, 0, sizeof(blockData));  // 🧹 Clear the buffer
data.getBytes(blockData, 16);  // 🔣 Convert string to byte array

// 🔐 Authenticate before writing
status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
if (status != MFRC522::STATUS_OK) {
Serial.print("❌ Authentication failed (Write): ");
Serial.println(mfrc522.GetStatusCodeName(status));
return;
}

// 💾 Write data to block
status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
if (status != MFRC522::STATUS_OK) {
Serial.print("❌ Writing failed: ");
Serial.println(mfrc522.GetStatusCodeName(status));
return;
}
    
Serial.println("✅ Data written successfully!");
}

// 📖 Function to read data from RFID card
void ReadDataFromBlock(int blockNum) {
// 🔐 Authenticate before reading
status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
if (status != MFRC522::STATUS_OK) {
Serial.print("❌ Authentication failed (Read) on block ");
Serial.print(blockNum);
Serial.print(": ");
Serial.println(mfrc522.GetStatusCodeName(status));
return;
}

// 📥 Read data from block
status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
if (status != MFRC522::STATUS_OK) {
Serial.print("❌ Reading failed on block ");
Serial.print(blockNum);
Serial.print(": ");
Serial.println(mfrc522.GetStatusCodeName(status));
return;
}

// 🔣 Convert bytes to string and clean up the data
String cardData = String((char*)readBlockData);
cardData.trim();

Serial.print("📄 Card Data: ");
Serial.println(cardData);
}

// 🚀 Setup function
void setup() {
Serial.begin(115200);
SPI.begin();          // 🛠 Initialize SPI communication
mfrc522.PCD_Init();   // 🚀 Initialize RFID reader
Serial.println("🔄 Ready to scan RFID Tag...");

// 🔐 Set default authentication key (0xFF for most cards)
for (byte i = 0; i < 6; i++) {
key.keyByte[i] = 0xFF;
}
}

// 🔄 Main loop function
void loop() {

// 🚫 Return if no card is present or failed to read
if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
return;
}

Serial.println("\n🎴 **Card Detected**");

// 🔑 Display Card UID
Serial.print(F("🔹 UID: "));
for (byte i = 0; i < mfrc522.uid.size; i++) {
Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
Serial.print(mfrc522.uid.uidByte[i], HEX);
}
Serial.println();

// 📌 Display Card Type
Serial.print(F("🔸 Type: "));
Serial.println(mfrc522.PICC_GetTypeName(mfrc522.PICC_GetType(mfrc522.uid.sak)));

// ✍️ Write data to card
Serial.println("📝 Writing data to card...");
WriteDataToBlock(blockNum, "401125025-HARRIS T WILLIAMS");  // Example data

// 📖 Read data back from card
Serial.println("📖 Reading data from card...");
ReadDataFromBlock(blockNum);

mfrc522.PICC_HaltA();    // ✋ Halt card
mfrc522.PCD_StopCrypto1();  // 🛑 Stop encryption
delay(2000);  // ⏱ Wait for 2 seconds before next loop
}