#pragma once

#include "ofMain.h"
#include "ofxFft.h"
#include "ofxGui.h"
#include "ofxOsc.h"

class ofApp : public ofBaseApp{

	public:

		void setup();
		void update();
		void draw();
        void exit();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void audioReceived(float* input, int bufferSize, int nChannels);

        void setupAudio();
		void plotFFT(vector<float>& buffer, float scale, float offset);
		void plotLinearAverages(vector<float>& buffer, float scale, float offset);
		void plotLogAverages(vector<float>& buffer, float height, float offset);
		void plotLinLogAverages(vector<float>& buffer, float height, float offset);
		void doLinearAverage(vector<float>& spectrum);
		void doLogAverage(vector<float>& spectrum);
		void setupLinearAverages(int numAvg);
        void setupLogAverages(int minBandwidth, int bandsPerOctave);
        float calcAvg(float lowFreq, float hiFreq);
        //int freqToIndex(float freq);
        float getLogAverageCentreFreq(int index);
        float getLogAverageBandwidth(int index);
        void setupSerial();
        void setupGui();
        void setupFFT();
        void setupXmlSettings();
        void sendSerialData();

        ofSoundStream soundStream;
        int bufferSize;
        ofxFft* fft;

        ofMutex soundMutex;
        vector<float> drawBins, middleBins, audioBins;
        int plotHeight;

        vector<float> averages;
        vector<float> log_averages;
        int octaves;
        int avgPerOctave;
        int sampleRate;
        float bandWidth;
        int numLinearAverages;

        float ratio;

        // GUI
        ofxPanel gui;
        vector<ofParameter<float>> sliders;
        ofParameter<float> gain;        
        int plotType;
        ofColor background;
        ofColor foreground;

        // Serial comms
        ofSerial serial;
        bool bIsSerialSetup;
        ofParameter<bool> bSendSerial;
        int baudRate;
        unsigned char buf[30];
        bool bVerifyData;

        //OSC
        ofxOscSender osc;
        ofParameter<bool> bSendOSC;
        string serialPortName;

        //XML
        ofXml xml;
};
