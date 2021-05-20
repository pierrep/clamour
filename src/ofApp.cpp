#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    foreground.set(ofColor(0,128,128));
    background.set(ofColor(0,52,52));
    ofBackground(background);
    ofSetWindowTitle("Clamour");
    ofSetFrameRate(60);

    setupFFT();
    setupLinearAverages(numLinearAverages);
    setupLogAverages(22,3);
    setupGui();
    setupAudio();    
    setupXmlSettings();
    setupSerial();

    ofxOscSenderSettings oscSettings;
    oscSettings.host = "localhost";
    oscSettings.port = 12345;
    osc.setup(oscSettings);
}

//--------------------------------------------------------------
void ofApp::exit()
{
    ofLogNotice() << "Saving parameters...";
    gui.saveToFile("parameter-settings.xml");
}

//--------------------------------------------------------------
void ofApp::setupGui()
{
    ofLogNotice() << "log_averages.size() =  " << log_averages.size();
    for(int i = 0; i < log_averages.size(); i++) {
        ofParameter<float> slider;
        string name;
        float centerFrequency = getLogAverageCentreFreq(i);
        if(centerFrequency < 1000.0f) name = ofToString((int)centerFrequency);
        else name = ofToString((int)((float)centerFrequency/1000.0f))+"k";
        slider.set("Freq "+name+"Hz", 0.5f,0,1.0f);
        sliders.push_back(slider);
    }
    gain.set("Gain",10,0,100.0f);

    gui.setup("Parameters");
    gui.add(gain);
    for(int i = 0; i < log_averages.size(); i++) {
        gui.add(sliders[i]);
    }
    bSendOSC.set("Use OSC",false);
    gui.add(bSendOSC);
    bSendSerial.set("Send to Serial",false);
    gui.add(bSendSerial);
    gui.setPosition(ofGetWidth()-220,5);
    gui.loadFromFile("parameter-settings.xml");
    plotType = 1;
}

//--------------------------------------------------------------
void ofApp::setupFFT() {
    bIsSerialSetup = false;
    bufferSize = 2048;
    sampleRate = 44100;

    fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING, OF_FFT_FFTW);

    drawBins.resize(fft->getBinSize());
    middleBins.resize(fft->getBinSize());
    audioBins.resize(fft->getBinSize());
    plotHeight = 350;

    bandWidth = (2.0f / bufferSize) * ((float)sampleRate / 2.0f);
    numLinearAverages = 8;
}

//--------------------------------------------------------------
void ofApp::setupSerial()
{

    //serial.listDevices();
    //vector <ofSerialDeviceInfo> deviceList = serial.getDeviceList();

    // this should be set to whatever com port your serial device is connected to.
    // (ie, COM4 on a pc, /dev/tty.... on linux, /dev/tty... on a mac)
    // arduino users check in arduino app....
    int baud = baudRate;
    bool bSuccess = serial.setup(serialPortName, baud); //open the first device
    //serial.setup("COM4", baud); // windows example
    //serial.setup("/dev/tty.usbserial-A4001JEC", baud); // mac osx example
    //serial.setup("/dev/ttyUSB0", baud); //linux example
    if(bSuccess) {
        bIsSerialSetup = true;
        ofLogNotice() << "Set up serial port successfully...";
    }
}

//--------------------------------------------------------------
void ofApp::setupAudio()
{
    //soundStream.printDeviceList();
    ofSoundStreamSettings settings;

    // if you want to set the device id to be different than the default
    // auto devices = soundStream.getDeviceList();
    // settings.device = devices[4];

    // you can also get devices for an specific api
    // auto devices = soundStream.getDevicesByApi(ofSoundDevice::Api::PULSE);
    // settings.device = devices[0];

    // or by name
    auto devices = soundStream.getMatchingDevices("default");
    if(!devices.empty()){
        settings.setInDevice(devices[0]);
    }

    //settings.setApi(ofSoundDevice::PULSE);

    settings.setInListener(this);
    settings.sampleRate = sampleRate;
    settings.numOutputChannels = 0;
    settings.numInputChannels = 1;
    settings.bufferSize = bufferSize;
    soundStream.setup(settings);        
}

//--------------------------------------------------------------
void ofApp::update(){
    soundMutex.lock();
	drawBins = middleBins;
	soundMutex.unlock();

    doLinearAverage(drawBins);
    doLogAverage(drawBins);

    for(int i = 0; i < log_averages.size(); i++)
    {
        log_averages[i] = log_averages[i]*sliders[i].get();
    }

    //Send to Arduino
    if(bIsSerialSetup && bSendSerial && (ofGetFrameNum() > 60)) {
        sendSerialData();
    }

    if(bSendOSC) {
        for(int i = 0; i < log_averages.size() -1; i++) // last band is NaN for some reason
        {
            ofxOscMessage m;
            m.setAddress("/fft/band"+ofToString(i));
            m.addFloatArg(log_averages[i]);
            osc.sendMessage(m, false);
        }
    }

}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(255);

    int margin = 250;
    ratio = (float) (ofGetWidth()-margin) / (float) fft->getBinSize();

    if(plotType == 1) {
        plotFFT(drawBins, plotHeight, 5);
    } else if (plotType ==2) {
        plotLinearAverages(drawBins,plotHeight,5);
    } else if (plotType == 3) {
        plotLogAverages(drawBins,plotHeight,5);
    }
    plotLinLogAverages(drawBins,plotHeight,ofGetHeight() - plotHeight-40);

    ofDrawBitmapString("OSC address range: \n/fft/band0 to /fft/band28", ofGetWidth() - 220, ofGetHeight() - 45);
    ofDrawBitmapString(ofToString((int) ofGetFrameRate()) + " fps", ofGetWidth() - 60, ofGetHeight() - 15);

    gui.draw();
}

//--------------------------------------------------------------
void ofApp::audioReceived(float* input, int bufferSize, int nChannels)
{
    // Set gain
    for(int i = 0; i < bufferSize; i++) {
        input[i] *= gain.get();
    }

    fft->setSignal(input);

    float* curFft = fft->getAmplitude();
    memcpy(&audioBins[0], curFft, sizeof(float) * fft->getBinSize());

    soundMutex.lock();
    //middleBins = audioBins;
    for(int i = 0; i < fft->getBinSize(); i++) {
        middleBins[i] = 0.5f*middleBins[i] + 0.5f*audioBins[i];
    }
    soundMutex.unlock();
}

//--------------------------------------------------------------
void ofApp::setupXmlSettings()
{
    if(!xml.load("settings.xml")){
        ofLogError() << "Couldn't load settings file";
    }

    auto settings = xml.getChild("settings");
    if(!settings){
        settings = xml.appendChild("settings");
    }

    ofXml serialport = settings.findFirst("serialport");
    if(!serialport){
        settings.removeChild("serialport");
        serialport = settings.appendChild("serialport");
        serialport.set("/dev/ttyUSB0");
        xml.save("settings.xml");
    }

    ofXml baudrate = settings.findFirst("baudrate");
    if(!baudrate){
        settings.removeChild("baudrate");
        baudrate = settings.appendChild("baudrate");
        baudrate.set("9600");
        xml.save("settings.xml");
    }

    auto serial = serialport.findFirst("//serialport");
    if(serial) {
        serialPortName = serial.getValue();
    }
    auto baud = serialport.findFirst("//baudrate");
    if(baud) {
        baudRate = baud.getIntValue();
    }
}

//--------------------------------------------------------------
void ofApp::plotFFT(vector<float>& buffer, float height, float offset)
{
    ofPushStyle();
	ofPushMatrix();
    ofTranslate(16, offset);
    ofSetColor(255,0,0);
    ofDrawBitmapString("Frequency Domain", 0, height+25);
    ofSetColor(foreground);
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);
    ofSetColor(255);
	ofNoFill();
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);
	glPushMatrix();
	glTranslatef(0, height, 0);
	ofBeginShape();

    for (unsigned int i = 0; i < buffer.size(); i++) {
        ofVertex(i*ratio, buffer[i] * -height);
	}
	ofEndShape();
	glPopMatrix();

    ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::plotLinearAverages(vector<float>& buffer, float height, float offset)
{
    ofPushStyle();
    ofPushMatrix();
	ofTranslate(16, offset);
    ofSetColor(255,0,0);
    ofDrawBitmapString("Linear Averages", 0, height + 25);
    ofSetColor(foreground);
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);
    ofSetColor(255);
    ofNoFill();
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);

    int w = int( buffer.size()/averages.size());
    for(unsigned int i = 0; i < averages.size(); i++)
    {
        ofDrawRectangle(i*w*ratio, height - averages[i]*height, w*ratio, averages[i]*height);
    }

    ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::plotLogAverages(vector<float>& buffer, float height, float offset)
{
    ofPushStyle();
    ofPushMatrix();
	ofTranslate(16,offset);
    ofSetColor(255,0,0);
    ofDrawBitmapString("Log Averages", 0, height + 25);
    ofSetColor(foreground);
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);
    ofSetColor(255);
    ofNoFill();
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);

    for(unsigned int i = 0; i < log_averages.size(); i++)
    {
        float centerFrequency = getLogAverageCentreFreq(i);
        // how wide is this average in Hz?
        float averageWidth = getLogAverageBandwidth(i);

        // we calculate the lowest and highest frequencies
        // contained in this average using the center frequency
        // and bandwidth of this average.
        float lowFreq  = centerFrequency - averageWidth/2.0f;
        float highFreq = centerFrequency + averageWidth/2.0f;

        // freqToIndex converts a frequency in Hz to a spectrum band index
        // that can be passed to getBand. in this case, we simply use the
        // index as coordinates for the rectangle we draw to represent
        // the average.
        int xl = (int)fft->getBinFromFrequency(lowFreq,sampleRate);
        int xr = (int)fft->getBinFromFrequency(highFreq,sampleRate);

        ofDrawRectangle( xl*ratio, height - log_averages[i]*height, (xr-xl)*ratio, log_averages[i]*height );
    }

    ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::plotLinLogAverages(vector<float>& buffer, float height, float offset)
{
    ofPushStyle();
    ofPushMatrix();
	ofTranslate(16,offset);
    ofSetColor(255,0,0);
    ofDrawBitmapString("Linear Log Averages", 0, height + 25);
    ofSetColor(foreground);
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);
    ofSetColor(255);
    ofNoFill();
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);

	int w = int( buffer.size()/log_averages.size());
    for(unsigned int i = 0; i < log_averages.size(); i++)
    {
        float centerFrequency = getLogAverageCentreFreq(i);
        // how wide is this average in Hz?
        float averageWidth = getLogAverageBandwidth(i);

        // we calculate the lowest and highest frequencies
        // contained in this average using the center frequency
        // and bandwidth of this average.
        float lowFreq  = centerFrequency - averageWidth/2.0f;
        float highFreq = centerFrequency + averageWidth/2.0f;

        // freqToIndex converts a frequency in Hz to a spectrum band index
        // that can be passed to getBand. in this case, we simply use the
        // index as coordinates for the rectangle we draw to represent
        // the average.
        //int xl = (int)fft->getBinFromFrequency(lowFreq,sampleRate);
        //int xr = (int)fft->getBinFromFrequency(highFreq,sampleRate);

        ofDrawRectangle(i*w*ratio, height - log_averages[i]*height, w*ratio, log_averages[i]*height );
        if(centerFrequency < 1000.0f)
            ofDrawBitmapString(ofToString((int)centerFrequency),(i*w+2)*ratio,height+10);
        else
            ofDrawBitmapString(ofToString((int)((float)centerFrequency/1000.0f))+"k",(i*w+2)*ratio,height+10);
    }

    ofPopMatrix();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::doLinearAverage(vector<float>& spectrum)
{
    int avgWidth = (int) spectrum.size() / averages.size();
    for (unsigned int i = 0; i < averages.size(); i++)
    {
        float avg = 0;
        int j;
        for (j = 0; j < avgWidth; j++)
        {
            int offset = j + i * avgWidth;
            if (offset < spectrum.size() )
            {
                avg += spectrum[offset];
            }
            else
            {
                break;
            }
        }
        avg /= j + 1;
        averages[i] = avg;
    }
}

//--------------------------------------------------------------
void ofApp::doLogAverage(vector<float>& spectrum)
{
    for (int i = 0; i < octaves; i++)
    {
        float lowFreq, hiFreq, freqStep;
        if (i == 0)
        {
            lowFreq = 0.0f;
        }
        else
        {
            lowFreq = ((float) sampleRate / 2.0f) / (float) pow(2, octaves - i);
        }
        hiFreq = ((float)sampleRate / 2.0f) / (float) pow(2, octaves - i - 1);
        freqStep = (hiFreq - lowFreq) / (float) avgPerOctave;

        float f = lowFreq;
        for (int j = 0; j < avgPerOctave; j++)
        {
            int offset = j + i * avgPerOctave;
            log_averages[offset] = calcAvg(f, f + freqStep);
            f += freqStep;
        }
    }
}

//--------------------------------------------------------------
float ofApp::calcAvg(float lowFreq, float hiFreq)
{

    int lowBound = fft->getBinFromFrequency(lowFreq,sampleRate);
    int hiBound = fft->getBinFromFrequency(hiFreq,sampleRate);

    float avg = 0;
    for (int i = lowBound; i <= hiBound; i++)
    {
      avg += drawBins[i];
    }
    avg /= (float)(hiBound - lowBound + 1);
    return avg;
}

//--------------------------------------------------------------
float ofApp::getLogAverageCentreFreq(int index)
{
    // which "octave" is this index in?
    int octave = index / avgPerOctave;
    // which band within that octave is this?
    int offset = index % avgPerOctave;
    float lowFreq, hiFreq, freqStep;
    // figure out the low frequency for this octave
    if (octave == 0)
    {
        lowFreq = 0;
    }
    else
    {
        lowFreq = (sampleRate / 2) / (float) pow(2, octaves - octave);
    }
    // and the high frequency for this octave
    hiFreq = (sampleRate / 2) / (float) pow(2, octaves - octave - 1);
    // each average band within the octave will be this big
    freqStep = (hiFreq - lowFreq) / avgPerOctave;
    // figure out the low frequency of the band we care about
    float f = lowFreq + offset*freqStep;
    // the center of the band will be the low plus half the width
    return f + freqStep/2;
}

//--------------------------------------------------------------
float ofApp::getLogAverageBandwidth(int index)
{
      // which "octave" is this index in?
      int octave = index / avgPerOctave;
      float lowFreq, hiFreq, freqStep;
      // figure out the low frequency for this octave
      if (octave == 0)
      {
        lowFreq = 0;
      }
      else
      {
        lowFreq = ((float)sampleRate / 2.0f) / (float) pow(2, octaves - octave);
      }
      // and the high frequency for this octave
      hiFreq = ((float)sampleRate / 2.0f) / (float) pow(2, octaves - octave - 1);
      // each average band within the octave will be this big
      freqStep = (hiFreq - lowFreq) / avgPerOctave;

      return freqStep;
}

//--------------------------------------------------------------
//int ofApp::freqToIndex(float freq)
//{
//    // special case: freq is lower than the bandwidth of spectrum[0]
//    if (freq < bandWidth / 2) return 0;
//    // special case: freq is within the bandwidth of spectrum[spectrum.length - 1]
//    if (freq > sampleRate / 2 - bandWidth / 2) return fft->getBinSize() - 1;
//    // all other cases
//    float fraction = freq / (float) sampleRate;
//    int i = floor(bufferSize * fraction + 0.5f);
//    return i;
//}

//--------------------------------------------------------------
void ofApp::setupLinearAverages(int numAvg)
{
    if (numAvg > fft->getBinSize() / 2)
    {
      ofLogError() << "The number of averages for this transform can be at most " << fft->getBinSize() / 2;
      return;
    }
    else
    {
        averages.clear();
        averages.resize(numAvg);
    }
}

//--------------------------------------------------------------
void ofApp::setupLogAverages(int minBandwidth, int bandsPerOctave)
  {
    float nyq = (float) sampleRate / 2.0f;
    octaves = 1;
    while ((nyq /= 2.0f) > minBandwidth)
    {
      octaves++;
    }
    ofLogVerbose() << "Number of octaves = " << octaves;
    avgPerOctave = bandsPerOctave;
    log_averages.clear();
    log_averages.resize(octaves * bandsPerOctave);
  }

//--------------------------------------------------------------
void ofApp::sendSerialData()
{
    //To verify data, set bVerifyData=true and send data back via Arduino. Note this is for debugging purposes only
    bVerifyData = false;

    if(bVerifyData) {
        unsigned char bytesReturned[30];
        int nRead  = 0;
        nRead = serial.readBytes( bytesReturned, 30);
        if(nRead == OF_SERIAL_NO_DATA) {
            ofLogNotice() << ".... OF_SERIAL_NO_DATA";
        } else {
            cout << "Read: " << nRead << " bytes: " << std::flush;
            for(int i = 0; i < nRead;i++)
            {
                 cout << " " << (int) bytesReturned[i] << std::flush;
            }
            cout << endl;
        }
        ofLogNotice() << "----------------------------------------------------------------------------------------------";
    }

    for(int i = 0; i < log_averages.size(); i++)
    {
        buf[i] = (unsigned char)(ofClamp(log_averages[i],0.0f,1.0f)*255);
    }

    serial.writeBytes(buf,log_averages.size());

    if(bVerifyData) {
        cout << "Sent: " << log_averages.size() << " bytes: " << std::flush;
        for(int i = 0; i < log_averages.size(); i++)
        {
            cout << " " << (int)buf[i] << std::flush;
        }
        cout << endl;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key == '[') numLinearAverages--;
    if(key == ']') numLinearAverages++;
    if(numLinearAverages < 1) numLinearAverages = 1;
    if(numLinearAverages > fft->getBinSize()) numLinearAverages = fft->getBinSize();
    setupLinearAverages(numLinearAverages);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    if(key == '1') plotType = 1;
    if(key == '2') plotType = 2;
    if(key == '3') plotType = 3;
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
