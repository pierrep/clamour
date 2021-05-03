#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofBackground(0, 0, 0);

    bufferSize = 512;
    sampleRate = 44100;       

    fft = ofxFft::create(bufferSize, OF_FFT_WINDOW_HAMMING, OF_FFT_FFTW);    

    ofLogVerbose() << "Bin Size: " << fft->getBinSize();

    drawBins.resize(fft->getBinSize());
	middleBins.resize(fft->getBinSize());
	audioBins.resize(fft->getBinSize());
    plotHeight = 256;

    bandWidth = (2.0f / bufferSize) * ((float)sampleRate / 2.0f);
    numAverages = 5;

	setupLinearAverages(numAverages);
    setupLogAverages(22,3);

    ofLogNotice() << "log_averages.size() =  " << log_averages.size();
    for(int i = 0; i < log_averages.size(); i++)
    {
        ofParameter<float> slider;

        string name;
        float centerFrequency = getLogAverageCentreFreq(i);
        if(centerFrequency < 1000.0f)
            name = ofToString((int)centerFrequency);
        else
            name = ofToString((int)((float)centerFrequency/1000.0f))+"k";

        //slider.set("freq "+ofToString(i), 0.5f,0,1.0f);
        slider.set("Freq "+name+"Hz", 0.5f,0,1.0f);
        sliders.push_back(slider);
    }
    gain.set("Gain",10,0,100.0f);

    gui.setup("Parameters");

    gui.add(gain);
    for(int i = 0; i < log_averages.size(); i++)
    {
        gui.add(sliders[i]);
    }
    gui.setPosition(ofGetWidth()-220,10);

    gui.loadFromFile("parameter-settings.xml");

    setupAudio();
}

void ofApp::exit()
{
    ofLogNotice() << "Saving parameters...";
    gui.saveToFile("parameter-settings.xml");
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

    // or get the default device for an specific api:
    // settings.api = ofSoundDevice::Api::PULSE;

    // or by name
//    auto devices = soundStream.getMatchingDevices("default");
//    if(!devices.empty()){
//        settings.setInDevice(devices[0]);
//    }

    settings.setApi(ofSoundDevice::PULSE);

    settings.setInListener(this);
    settings.sampleRate = sampleRate;
    settings.numOutputChannels = 0;
    settings.numInputChannels = 2;
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


}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(255);

    int margin = 250;
    ratio = (float) (ofGetWidth()-margin) / (float) fft->getBinSize();

	plotFFT(drawBins, plotHeight, 16);
    //plotLinearAverages(drawBins, plotHeight, 16 + plotHeight+50);
    //plotLogAverages(drawBins,plotHeight,16 + plotHeight*2+50*2);
    //plotLinLogAverages(drawBins,plotHeight,16 + plotHeight*3+50*3);
    plotLinLogAverages(drawBins,plotHeight,ofGetHeight() - plotHeight-40);

    //ofDrawBitmapString(ofToString((int) ofGetFrameRate()) + " fps", ofGetWidth() - 80, ofGetHeight() - 20);

    gui.draw();
}

//--------------------------------------------------------------
void ofApp::audioReceived(float* input, int bufferSize, int nChannels)
{
//	float maxValue = 0;
//	for(int i = 0; i < bufferSize; i++) {
//		if(abs(input[i]) > maxValue) {
//			maxValue = abs(input[i]);
//		}
//	}
//	for(int i = 0; i < bufferSize; i++) {
//		input[i] /= maxValue;
//	}

        for(int i = 0; i < bufferSize; i++) {
            input[i] *= gain.get();
        }

    fft->setSignal(input);

    float* curFft = fft->getAmplitude();
    memcpy(&audioBins[0], curFft, sizeof(float) * fft->getBinSize());

//    float maxValue = 0;
//    for(int i = 0; i < fft->getBinSize(); i++) {
//        if(abs(audioBins[i]) > maxValue) {
//            maxValue = abs(audioBins[i]);
//        }
//    }
//    for(int i = 0; i < fft->getBinSize(); i++) {
//        audioBins[i] /= maxValue;
//    }


    soundMutex.lock();
    //middleBins = audioBins;
    for(int i = 0; i < fft->getBinSize(); i++) {
        middleBins[i] = 0.95f*middleBins[i] + 0.05f*audioBins[i];
    }
    soundMutex.unlock();
}

//--------------------------------------------------------------
void ofApp::plotFFT(vector<float>& buffer, float height, float offset)
{
	ofPushMatrix();
	ofTranslate(16, 16+offset);
    ofSetColor(255,0,0);
    ofDrawBitmapString("Frequency Domain", 0, height+30);
    ofSetColor(255);

	ofNoFill();
    ofDrawRectangle(0, 0, buffer.size()*ratio, height);
	glPushMatrix();
	glTranslatef(0, height, 0);
	ofBeginShape();

    for (unsigned int i = 1; i < buffer.size(); i++) {
        ofVertex(i*ratio, buffer[i] * -height);
	}
	ofEndShape();
	glPopMatrix();

    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::plotLinearAverages(vector<float>& buffer, float height, float offset)
{
    ofPushMatrix();
	ofTranslate(16, offset);
    ofDrawBitmapString("Linear Averages", 0, 0);

    ofDrawRectangle(0, 0, buffer.size()*ratio, height);

    int w = int( buffer.size()/averages.size());
    for(unsigned int i = 0; i < averages.size(); i++)
    {
        ofDrawRectangle(i*w*ratio, height - averages[i]*height, w*ratio, averages[i]*height);
    }

    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::plotLogAverages(vector<float>& buffer, float height, float offset)
{
    ofPushMatrix();
	ofTranslate(16,offset);
    ofDrawBitmapString("Log Averages", 0, 0);

    ofDrawRectangle(0, 0, buffer.size()*ratio, height);

    for(unsigned int i = 1; i < log_averages.size(); i++)
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
}

//--------------------------------------------------------------
void ofApp::plotLinLogAverages(vector<float>& buffer, float height, float offset)
{
    ofPushMatrix();
	ofTranslate(16,offset);
    ofSetColor(255,0,0);
    ofDrawBitmapString("Linear Log Averages", 0, height + 30);
    ofSetColor(255);
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
void ofApp::keyPressed(int key){
    if(key == '[') numAverages--;
    if(key == ']') numAverages++;
    if(numAverages < 1) numAverages = 1;
    if(numAverages > fft->getBinSize()) numAverages = fft->getBinSize();
    setupLinearAverages(numAverages);
    //cout << "numAverages = " << numAverages <<  " binSize=" << fft->getBinSize() << endl;
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

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
