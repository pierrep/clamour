
unsigned char fftData[30];

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

}

void loop() {
  


  
}

void serialEvent() {
  while (Serial.available()) {

      int numBytes = Serial.readBytes(fftData, 30);
      //Serial.write(fftData,numBytes); //send data back for verification
      

    
  }

}
  
