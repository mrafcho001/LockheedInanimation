void setup() {
	Serial.begin(9600);
}
void loop() {
	if(Serial.available() > 0)
	{
		char byte = Serial.read();
		Serial.print("Recieved: ");
		Serial.println(byte);
	}
}
