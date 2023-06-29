#include <WiFi.h>         //library for wifi
#include <PubSubClient.h> //library for MQtt
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <ESP32Servo.h>
Servo s;

const byte KEYPAD_ROWS = 4;
const byte KEYPAD_COLS = 4;
byte rowPins[KEYPAD_ROWS] = {12, 13, 14, 15};
byte colPins[KEYPAD_COLS] = {26, 25, 27, 23};
char keys[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, KEYPAD_ROWS, KEYPAD_COLS);

#define lcd_ADDR 0x27
#define LCD_ADDR 0x26
#define LED 2
#define OUT 4
#define BUZZER_PIN 19
#define UltraTrig 18
#define UltraEcho 5
#define KEY_C 262 // 261.6256 Hz (middle C)
#define KEY_E 330 // 329.6276 Hz

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(LCD_ADDR, 20, 4);
LiquidCrystal_I2C lcd(lcd_ADDR, 16, 2);

String result = "";

void callback(char *subscribetopic, byte *payload, unsigned int payloadLength);

#define ORG "bko7qo"       // IBM ORGANITION ID
#define DEVICE_TYPE "abcd" // Device type mentioned in ibm watson IOT Platform
#define DEVICE_ID "1234"   // Device ID mentioned in ibm watson IOT Platform
#define TOKEN "12345678"   // Token
String data;

char server[] = ORG ".messaging.internetofthings.ibmcloud.com"; // Server Name
char publishTopic[] = "iot-2/evt/Data/fmt/json";                // topic name and type of event perform and format in which data to be send
char publishTopic1[] = "iot-2/evt/Data/fmt/String";
char subscribetopic[] = "iot-2/cmd/command/fmt/String"; // cmd  REPRESENT command type AND COMMAND IS TEST OF FORMAT STRING
char authMethod[] = "use-token-auth";                   // authentication method
char token[] = TOKEN;
char clientId[] = "d:" ORG ":" DEVICE_TYPE ":" DEVICE_ID; // client id

WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);

int pos;

void setup(){
    Serial.begin(115200);
    s.attach(32);
    lcd.init();
    LCD.init();
    lcd.backlight();
    startMessage();
    pinMode(OUT, INPUT);
    pinMode(LED, OUTPUT);
    pinMode(UltraTrig, OUTPUT);
    pinMode(UltraEcho, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    wificonnect();
    mqttconnect();
}

void startMessage(){
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Hello!");
    lcd.setCursor(5, 1);
    lcd.print("Press *");
}

void lock(int pos){
    if (pos == -1){
        s.write(90);
        lcd.clear();
        lcd.print("Door locked!");
    }
    else if (pos = 89){
        s.write(0);
        lcd.clear();
        lcd.print("Door Unlocked!");
    }
}

String inputCode(){
    lcd.setCursor(5, 0);
    lcd.print("New Code");
    lcd.setCursor(5, 1);
    lcd.print("[____]");
    lcd.setCursor(6, 1);
    while (result.length() < 4){
        char key = keypad.getKey();
        if (key >= '0' && key <= '9'){
            lcd.print('*');
            result += key;
        }
    }
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("...........");
    delay(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("New Code saved!");
    delay(1000);
    return result;
}

void actualCode(){
    String realCode = result;
    int i = 0;
    String code = "";
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Enter Code:");
    lcd.setCursor(5, 1);
    lcd.print("[....]");
    lcd.setCursor(6, 1);
    while (code.length() < 4){
        char key = keypad.getKey();
        if (key >= '0' && key <= '9'){
            lcd.print('*');
            code += key;
        }
    }
    if (code == realCode){
        pos = s.read();
        lock(pos);
        startMessage();
    }

    else{
        lcd.clear();
        lcd.setCursor(4, 0);
        lcd.print("Wrong Code!");
        lcd.setCursor(3, 1);
        lcd.print("BREAK IN!!");
        delay(1000);
        startMessage();
    }
}

void loop(){
    char key = keypad.getKey();
    if (key == '*'){
        lcd.clear();
        inputCode();
        actualCode();
    }
    pos = s.read();
    digitalWrite(18, LOW);
    digitalWrite(18, HIGH);
    delayMicroseconds(10);
    digitalWrite(18, LOW);
    float dur = pulseIn(5, HIGH);
    float dis = (dur * 0.0343) / 2;
    int state = digitalRead(OUT);
    PublishData(result, dis, pos);
    PublishData(state);
    if (state == 1){
        tone(BUZZER_PIN, KEY_E, 500);
        delay(100);
        tone(BUZZER_PIN, KEY_C, 700);
        LCD.backlight();
        LCD.setCursor(0, 0);
        LCD.print("Waiting...");
    }
    delay(1000);
    LCD.noBacklight();
    if (!client.loop())
    {
        mqttconnect();
    }
}

// for doorbell
void PublishData(int value){
    mqttconnect();
    String payload = String(value);

    Serial.print("Sending payload: ");
    Serial.println(payload);

    if (client.publish(publishTopic1, (char *)payload.c_str()))
        Serial.println("Publish ok");
    else
        Serial.println("Publish failed");
}

// for doorlock and alarm
void PublishData(String result, float dis, int pos){
    mqttconnect();

    // creating the String in in form JSon to update the data to ibm cloud
    String payload = "{\"Code\":";
    payload += result;
    payload += ", \"Distance\":";
    payload += dis;
    payload += ", \"ServoPos\":";
    payload += pos;
    payload += "}";

    Serial.print("Sending payload: ");
    Serial.println(payload);

    if (client.publish(publishTopic, (char *)payload.c_str()))
        Serial.println("Publish ok");
    else
        Serial.println("Publish failed");
}

void mqttconnect(){
    if (!client.connected()){
        Serial.print("Reconnecting client to ");
        Serial.println(server);
        while (!!!client.connect(clientId, authMethod, token)){
            Serial.print(".");
            delay(500);
        }
        initManagedDevice();
        Serial.println();
    }
}

void wificonnect(){
    Serial.println();
    Serial.print("Connecting to ");

    WiFi.begin("Wokwi-GUEST", "", 6);
    while (WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void initManagedDevice() {
    if (client.subscribe(subscribetopic)) {
        Serial.println(subscribetopic);
        Serial.println("subscribe to cmd OK");
    } else {
        Serial.println("subscribe to cmd FAILED");
    }
}

void callback(char* subscribetopic, byte* payload, unsigned int payloadLength) {
    Serial.print("callback invoked for topic: ");
    Serial.println(subscribetopic);
    
    for (int i = 0; i < payloadLength; i++) {
        // Serial.print((char)payload[i]);
        data += (char)payload[i];
    }
    
    Serial.println("data: " + data);
    
    if (data == "{\"command\":\"lighton\"}") {
        Serial.println(data);
        digitalWrite(LED, HIGH);
    } else if (data == "{\"command\":\"lightoff\"}") {
        Serial.println(data);
        digitalWrite(LED, 0);
    } else if (data == "{\"command\":\"codecorrect\"}") {
        int pos = s.read();
        lock(pos);
        startMessage();
    } else if (data.substring(12, 22) == "codechange") {
        result = data.substring(22, 26);
    } else if (data == "{\"command\":\"buzzeron\"}") {
        tone(BUZZER_PIN, 1000);
    } else if (data == "{\"command\":\"buzzeroff\"}") {
        noTone(BUZZER_PIN);
    } else {
        String text = data;
        LCD.clear();
        LCD.backlight();
        int len = sizeof(text);
        int diff = 80 - len;
        char str = ' ';
        
        for (int i = 0; i < diff; i++)
            text = text + str;
        
        for (int line = 0; line < 4; line++) {
            LCD.setCursor(0, line);
            LCD.print(text.substring(line * 20, (line + 1) * 20));
        }
        
        Serial.println("data: " + text);
        delay(5000);
        LCD.clear();
        LCD.noBacklight();
        Serial.println(data);
    }
    data = "";
}
