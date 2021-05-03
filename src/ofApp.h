#pragma once

#include "ofMain.h"
#include "ofxFft.h"
#include "ofxGui.h"

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
        int numAverages;

        float ratio;

        // GUI
        ofxPanel gui;
        vector<ofParameter<float>> sliders;
        ofParameter<float> gain;
        int plotType;
        ofColor background;
        ofColor foreground;

};
