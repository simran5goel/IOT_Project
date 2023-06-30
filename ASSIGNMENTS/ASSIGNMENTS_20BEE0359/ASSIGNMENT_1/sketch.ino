void setup() {
  pinMode(26,OUTPUT);
  pinMode(2,OUTPUT);
  pinMode(5,INPUT);
  Serial.begin(9600);

}

void loop() {
  digitalWrite(2,0);
  digitalWrite(2,1);
  delayMicroseconds(10); 
  digitalWrite(2,0);
  float a = pulseIn(5,1);
  float dis = (a*0.0343)/2;
  Serial.println(dis);
  if(dis<100) 
    digitalWrite(26,1); //glowing LED for distance<100cm
  else
    digitalWrite(26,0);
  }
