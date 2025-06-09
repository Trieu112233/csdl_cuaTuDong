// Khai báo chân kết nối
const int pirinPin = 2;      // OUT của HC-SR501 nối vào chân D2
const int piroutPin = 3;
const int ledinPin = 12;     // LED trên board Arduino (chân D13)
const int ledoutPin = 13;

void setup() {
  pinMode(pirinPin, INPUT);  // Cảm biến PIR là ngõ vào
  pinMode(ledinPin, OUTPUT); // LED là ngõ ra
  pinMode(piroutPin, INPUT);  // Cảm biến PIR là ngõ vào
  pinMode(ledoutPin, OUTPUT); // LED là ngõ ra
}

void loop() {
  int motionDetected = digitalRead(pirinPin);
  int motionDetected2 = digitalRead(piroutPin);
  if (motionDetected == HIGH) {
    digitalWrite(ledinPin, HIGH);  // Bật đèn LED
  } else {
    digitalWrite(ledinPin, LOW);   // Tắt đèn LED
  }

  if (motionDetected2 == HIGH){
    digitalWrite(ledoutPin, HIGH);
  } else{
    digitalWrite(ledoutPin, LOW);
  }

  delay(1000);  // Chờ để tránh nhiễu
}
