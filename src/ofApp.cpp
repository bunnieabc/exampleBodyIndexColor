// This example shows how to work with the BodyIndex image in order to create
// a green screen effect. Note that this isn't super fast, but is helpful
// in understanding how the different image types & coordinate spaces work
// together. If you need performance, you will probably want to do this with shaders!

#include "ofApp.h"

#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 424
#define DEPTH_SIZE DEPTH_WIDTH * DEPTH_HEIGHT

#define COLOR_WIDTH 1920
#define COLOR_HEIGHT 1080

int count_rotate;
int count_rotate2;
int count_time;
int count_pressed = 0;
int screen_x = 220, screen_y = 200;

//--------------------------------------------------------------
void ofApp::setup() {
	
	type = 5;
	kinect.open();
	kinect.initDepthSource();
	kinect.initColorSource();
	kinect.initBodySource();
	kinect.initBodyIndexSource();

	//ofImage testImage;
	testImage.load("test.jpg");

	if (type == 5) {
		bStitchFaces = false;
		bDrawBodies = true;

		if (ofIsGLProgrammableRenderer()) {
			shader.load("shaders_gl3/bodyIndex");
			
		}
		else {
			shader.load("shaders/bodyIndex");
			
		}
	}
	
	else {
		ofSetWindowShape(1024, 768);
		kinect.initInfraredSource();
		if (kinect.getSensor()->get_CoordinateMapper(&coordinateMapper) < 0) {
			ofLogError() << "Could not acquire CoordinateMapper!";
		}

		numBodiesTracked = 0;
		bHaveAllStreams = false;

		bodyIndexImg.allocate(DEPTH_WIDTH, DEPTH_HEIGHT, OF_IMAGE_COLOR);
		foregroundImg.allocate(DEPTH_WIDTH, DEPTH_HEIGHT, OF_IMAGE_COLOR);
		//colorImg.allocate(DEPTH_WIDTH, DEPTH_HEIGHT, OF_IMAGE_COLOR);

		colorCoords.resize(DEPTH_WIDTH * DEPTH_HEIGHT);
		testImgCoords.resize(DEPTH_WIDTH * DEPTH_HEIGHT);
		// init
		/*for (int y = 0; y < DEPTH_HEIGHT; y++) {
			for (int x = 0; x < DEPTH_WIDTH; x++) {
				int index = (y * DEPTH_WIDTH) + x;
				testImgCoords[index] = testImage.getColor(x, y);
			}
		}*/

		asciiCharacters = string("  ..,,,'''``--_:;^^**""=+<>iv%&xclrs)/){}I?!][1taeo7zjLunT#@JCwfy325Fp6mqSghVd4EgXPGZbYkOA8U$KHDBWNMR0Q");
		ofPushStyle();
		font.load("Courier New Bold.ttf", 9);
		ofSetColor(255, 255, 255);
		ofPopStyle();
	}
	mImageBG.load("oldtvwebcam.png");
	mImageBGWeb.load("oldtvwebcamOn.png");
	mImageDial.load("dial.png");
	mImageDial2.load("dial.png");
	mFuzz.load("chanelchange.mp4");
	mTrix.load("trix.mov");
	count_rotate = 0;
	count_rotate2 = 0;
	count_time = 0;
}

//--------------------------------------------------------------
void ofApp::update() {
	kinect.update();
	if (type!= 5) {
		// Get pixel data
		auto& depthPix = kinect.getDepthSource()->getPixels();
		auto& bodyIndexPix = kinect.getBodyIndexSource()->getPixels();
		auto& colorPix = kinect.getColorSource()->getPixels();

		// Make sure there's some data here, otherwise the cam probably isn't ready yet
		if (!depthPix.size() || !bodyIndexPix.size() || !colorPix.size()) {
			bHaveAllStreams = false;
			return;
		}
		else {
			bHaveAllStreams = true;
		}


		// Count number of tracked bodies
		numBodiesTracked = 0;
		auto& bodies = kinect.getBodySource()->getBodies();
		for (auto& body : bodies) {
			if (body.tracked) {
				numBodiesTracked++;
			}
		}

		// Do the depth space -> color space mapping
		// More info here:
		// https://msdn.microsoft.com/en-us/library/windowspreview.kinect.coordinatemapper.mapdepthframetocolorspace.aspx
		// https://msdn.microsoft.com/en-us/library/dn785530.aspx
		coordinateMapper->MapDepthFrameToColorSpace(DEPTH_SIZE, (UINT16*)depthPix.getPixels(), DEPTH_SIZE, (ColorSpacePoint*)colorCoords.data());
		for (int k = 0; k < DEPTH_SIZE; k++) {

		}


		if (type == 0) {
			for (int y = 0; y < DEPTH_HEIGHT; y++) {
				for (int x = 0; x < DEPTH_WIDTH; x++) {
					int index = (y * DEPTH_WIDTH) + x;
					ofColor c2 = ofColor(0, 0, 0);
					bodyIndexImg.setColor(x, y, c2);
					foregroundImg.setColor(x, y, ofColor::white);

					// This is the check to see if a given pixel is inside a tracked  body or part of the background.
					// If it's part of a body, the value will be that body's id (0-5), or will > 5 if it's
					// part of the background
					// More info here:
					// https://msdn.microsoft.com/en-us/library/windowspreview.kinect.bodyindexframe.aspx
					float val = bodyIndexPix[index];
					if (val >= bodies.size()) {
						continue;
					}

					// Give each tracked body a color value so we can tell
					// them apart on screen
					//ofColor c = ofColor::fromHsb(val * 255 / bodies.size(), 200, 255);
					ofColor c = ofColor::white;
					bodyIndexImg.setColor(x, y, c);
					//bodyIndexImg2 = bodyIndexImg;
					//bodyIndexImg.setColor(x, y, c2);

					// For a given (x,y) in the depth image, lets look up where that point would be
					// in the color image
					ofVec2f mappedCoord = colorCoords[index];

					// Mapped x/y coordinates in the color can come out as floats since it's not a 1:1 mapping
					// between depth <-> color spaces i.e. a pixel at (100, 100) in the depth image could map
					// to (405.84637, 238.13828) in color space
					// So round the x/y values down to ints so that we can look up the nearest pixel
					mappedCoord.x = floor(mappedCoord.x);
					mappedCoord.y = floor(mappedCoord.y);

					// Make sure it's within some sane bounds, and skip it otherwise
					if (mappedCoord.x < 0 || mappedCoord.y < 0 || mappedCoord.x >= COLOR_WIDTH || mappedCoord.y >= COLOR_HEIGHT) {
						continue;
					}

					// Finally, pull the color from the color image based on its coords in
					// the depth image
					foregroundImg.setColor(x, y, colorPix.getColor(mappedCoord.x, mappedCoord.y));

				}
			}
			bodyIndexImg2 = bodyIndexImg;
			for (int y = 0; y < DEPTH_HEIGHT; y++) {
				for (int x = 0; x < DEPTH_WIDTH; x++) {
					bodyIndexImg.setColor(x, y, ofColor::black);
				}
			}
		}
		// Loop through the depth image

		else if (type == 1) {
			for (int y = 0; y < DEPTH_HEIGHT; y++) {
				ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));
				float c_num = ofRandom(25, 200);
				ofColor c5 = ofColor(c_num, c_num, c_num);
				for (int x = 0; x < DEPTH_WIDTH; x++) {
					int index = (y * DEPTH_WIDTH) + x;
					ofColor c2 = ofColor(0, 0, 0);
					bodyIndexImg.setColor(x, y, c5);
					foregroundImg.setColor(x, y, ofColor::white);

					// This is the check to see if a given pixel is inside a tracked  body or part of the background.
					// If it's part of a body, the value will be that body's id (0-5), or will > 5 if it's
					// part of the background
					// More info here:
					// https://msdn.microsoft.com/en-us/library/windowspreview.kinect.bodyindexframe.aspx
					float val = bodyIndexPix[index];
					if (val >= bodies.size()) {
						continue;
					}

					// Give each tracked body a color value so we can tell
					// them apart on screen
					
					bodyIndexImg.setColor(x, y, c4);
					

					// For a given (x,y) in the depth image, lets look up where that point would be
					// in the color image
					ofVec2f mappedCoord = colorCoords[index];

					// Mapped x/y coordinates in the color can come out as floats since it's not a 1:1 mapping
					// between depth <-> color spaces i.e. a pixel at (100, 100) in the depth image could map
					// to (405.84637, 238.13828) in color space
					// So round the x/y values down to ints so that we can look up the nearest pixel
					mappedCoord.x = floor(mappedCoord.x);
					mappedCoord.y = floor(mappedCoord.y);

					// Make sure it's within some sane bounds, and skip it otherwise
					if (mappedCoord.x < 0 || mappedCoord.y < 0 || mappedCoord.x >= COLOR_WIDTH || mappedCoord.y >= COLOR_HEIGHT) {
						continue;
					}

					// Finally, pull the color from the color image based on its coords in
					// the depth image
					foregroundImg.setColor(x, y, colorPix.getColor(mappedCoord.x, mappedCoord.y));

				}
			}
		}
		else if (type == 2) {
			for (int y = 0; y < DEPTH_HEIGHT; y++) {
				//ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));
				
				for (int x = 0; x < DEPTH_WIDTH; x++) {
					int index = (y * DEPTH_WIDTH) + x;
					ofColor c2 = ofColor(0, 0, 0);
					float c_num = ofRandom(25, 200);
					ofColor c5 = ofColor(c_num, c_num, c_num);
					bodyIndexImg.setColor(x, y, c5);
					foregroundImg.setColor(x, y, ofColor::white);

					// This is the check to see if a given pixel is inside a tracked  body or part of the background.
					// If it's part of a body, the value will be that body's id (0-5), or will > 5 if it's
					// part of the background
					// More info here:
					// https://msdn.microsoft.com/en-us/library/windowspreview.kinect.bodyindexframe.aspx
					float val = bodyIndexPix[index];
					if (val >= bodies.size()) {
						continue;
					}

					// Give each tracked body a color value so we can tell
					// them apart on screen
					
					ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));
					bodyIndexImg.setColor(x, y, c4);
					//bodyIndexImg2 = bodyIndexImg;
					//bodyIndexImg.setColor(x, y, c2);

					// For a given (x,y) in the depth image, lets look up where that point would be
					// in the color image
					ofVec2f mappedCoord = colorCoords[index];

					// Mapped x/y coordinates in the color can come out as floats since it's not a 1:1 mapping
					// between depth <-> color spaces i.e. a pixel at (100, 100) in the depth image could map
					// to (405.84637, 238.13828) in color space
					// So round the x/y values down to ints so that we can look up the nearest pixel
					mappedCoord.x = floor(mappedCoord.x);
					mappedCoord.y = floor(mappedCoord.y);

					// Make sure it's within some sane bounds, and skip it otherwise
					if (mappedCoord.x < 0 || mappedCoord.y < 0 || mappedCoord.x >= COLOR_WIDTH || mappedCoord.y >= COLOR_HEIGHT) {
						continue;
					}

					// Finally, pull the color from the color image based on its coords in
					// the depth image
					foregroundImg.setColor(x, y, colorPix.getColor(mappedCoord.x, mappedCoord.y));

				}
			}
		}
		else if (type == 3) {
			for (int y = 0; y < DEPTH_HEIGHT; y++) {
				//ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));

				for (int x = 0; x < DEPTH_WIDTH; x++) {
					int index = (y * DEPTH_WIDTH) + x;
					ofColor c2 = ofColor(0, 0, 0);
					float c_num = ofRandom(25, 200);
					ofColor c5 = ofColor(c_num, c_num, c_num);
					bodyIndexImg.setColor(x, y, c5);

					foregroundImg.setColor(x, y, testImage.getColor(x, y));

					// This is the check to see if a given pixel is inside a tracked  body or part of the background.
					// If it's part of a body, the value will be that body's id (0-5), or will > 5 if it's
					// part of the background
					// More info here:
					// https://msdn.microsoft.com/en-us/library/windowspreview.kinect.bodyindexframe.aspx
					float val = bodyIndexPix[index];
					if (val >= bodies.size()) {
						continue;
					}

					// Give each tracked body a color value so we can tell
					// them apart on screen

					ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));
					bodyIndexImg.setColor(x, y, c4);
					//bodyIndexImg2 = bodyIndexImg;
					//bodyIndexImg.setColor(x, y, c2);

					// For a given (x,y) in the depth image, lets look up where that point would be
					// in the color image
					ofVec2f mappedCoord = colorCoords[index];

					mappedCoord.x = floor(mappedCoord.x);
					mappedCoord.y = floor(mappedCoord.y);

					// Make sure it's within some sane bounds, and skip it otherwise
					if (mappedCoord.x < 0 || mappedCoord.y < 0 || mappedCoord.x >= COLOR_WIDTH || mappedCoord.y >= COLOR_HEIGHT) {
						continue;
					}

					// Finally, pull the color from the color image based on its coords in
					// the depth image
					foregroundImg.setColor(x, y, colorPix.getColor(mappedCoord.x, mappedCoord.y));
				}
			}
		}

		else if (type == 4) {
			for (int y = 0; y < DEPTH_HEIGHT; y = y+5) {
				//ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));

				for (int x = 0; x < DEPTH_WIDTH; x = x+5) {
					int index = (y * DEPTH_WIDTH) + x;
					ofColor c2 = ofColor(0, 0, 0);
					float c_num = ofRandom(0, 200);

					ofColor c5 = ofColor(c_num, c_num, c_num);
					bodyIndexImg.setColor(x, y, c5);

					for (int s = 0; s < 5; s++) {
						for (int t = 0; t < 5; t++) {
							bodyIndexImg.setColor(x+s, y+t, c5);
						}
					}

					foregroundImg.setColor(x, y, testImage.getColor(x, y));

					// This is the check to see if a given pixel is inside a tracked  body or part of the background.
					// If it's part of a body, the value will be that body's id (0-5), or will > 5 if it's
					// part of the background
					// More info here:
					// https://msdn.microsoft.com/en-us/library/windowspreview.kinect.bodyindexframe.aspx
					float val = bodyIndexPix[index];
					if (val >= bodies.size()) {
						continue;
					}

					// Give each tracked body a color value so we can tell
					// them apart on screen

					ofColor c4 = ofColor(ofRandom(25, 255), ofRandom(25, 255), ofRandom(25, 255));
					bodyIndexImg.setColor(x, y, c4);
					for (int s = 0; s < 5; s++) {
						for (int t = 0; t < 5; t++) {
							bodyIndexImg.setColor(x + s, y + t, c4);
						}
					}
					//bodyIndexImg2 = bodyIndexImg;
					//bodyIndexImg.setColor(x, y, c2);

					// For a given (x,y) in the depth image, lets look up where that point would be
					// in the color image
					ofVec2f mappedCoord = colorCoords[index];

					mappedCoord.x = floor(mappedCoord.x);
					mappedCoord.y = floor(mappedCoord.y);

					// Make sure it's within some sane bounds, and skip it otherwise
					if (mappedCoord.x < 0 || mappedCoord.y < 0 || mappedCoord.x >= COLOR_WIDTH || mappedCoord.y >= COLOR_HEIGHT) {
						continue;
					}

					// Finally, pull the color from the color image based on its coords in
					// the depth image
					foregroundImg.setColor(x, y, colorPix.getColor(mappedCoord.x, mappedCoord.y));
				}
			}
		}
		// Update the images since we manipulated the pixels manually. This uploads to the
		// pixel data to the texture on the GPU so it can get drawn to screen
		bodyIndexImg.update();
		foregroundImg.update();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	if (type == 5) {
		cam.begin();
		ofSetBackgroundColor(ofColor::black);
		ofPushMatrix();
		ofScale(80, 80, 80);

		shader.begin();
		shader.setUniform1i("uWidth", kinect.getBodyIndexSource()->getWidth());
		if (ofIsGLProgrammableRenderer()) {
			shader.setUniform1f("time", ofGetFrameNum() * 0.001);
			shader.setUniformTexture("uBodyIndexTex", kinect.getBodyIndexSource()->getTexture(), 1);
			shader.setUniformTexture("uColorTex", kinect.getColorSource()->getTexture(), 2);
		}
		else {
			// TEMP: Until OF master fixes texture binding for old pipeline.
			shader.setUniform1i("uBodyIndexTex", 1);
			kinect.getBodyIndexSource()->getTexture().bind(1);
			shader.setUniform1i("uColorTex", 2);
			kinect.getColorSource()->getTexture().bind(2);
		}

		ofSetColor(255);
		ofMesh mesh = kinect.getDepthSource()->getMesh(bStitchFaces, ofxKFW2::Source::Depth::PointCloudOptions::ColorCamera);
		mesh.draw();

		if (!ofIsGLProgrammableRenderer()) {
			// TEMP: Until OF master fixes texture binding for old pipeline.
			kinect.getColorSource()->getTexture().unbind(2);
			kinect.getBodyIndexSource()->getTexture().unbind(1);
		}
		shader.end();

		if (bDrawBodies) {
			kinect.getBodySource()->drawWorld();
		}

		ofPopMatrix();
		cam.end();

		ofSetColor(0);
		stringstream ss;
		ss << ofToString(ofGetFrameRate(), 2) << " FPS" << endl;
		ss << "Stitch [F]aces: " << (bStitchFaces ? "ON" : "OFF") << endl;
		ss << "Draw [B]odies: " << (bDrawBodies ? "ON" : "OFF") << endl;
		ofDrawBitmapString(ss.str(), 10, 20);
	}

	else {
		ofPushStyle();
		ofDrawRectangle(0, 0, 1024, 768);
		ofSetColor(ofColor::black);
		ofPopStyle();
		bodyIndexImg.draw(screen_x, screen_y);
		
		//bodyIndexImg.draw(DEPTH_WIDTH, DEPTH_HEIGHT);
		//bodyIndexImg.draw(0, 0);
		//foregroundImg.draw(DEPTH_WIDTH, 0);
		if (type == 0) {
			for (int i = 0; i < DEPTH_WIDTH; i += 7) {


				for (int j = 0; j < DEPTH_HEIGHT; j += 9) {

					if (bodyIndexImg2.getColor(i, j) == ofColor::white) {
						//bodyIndexImg.draw(0, 0);
						int character = ofRandom(asciiCharacters.size());
						// draw the character at the correct location
						ofPushStyle();
						ofSetColor(28, 255, 251);
						font.drawString(ofToString(asciiCharacters[character]), i+ screen_x, j+ screen_y);

						//font.load("Courier New Bold.ttf", 9);

						ofPopStyle();
					}
					else {
						int character = ofRandom(asciiCharacters.size());
						// draw the character at the correct location
						ofPushStyle();
						ofSetHexColor(0x41605f);
						font.drawString(ofToString(asciiCharacters[character]), i + screen_x, j + screen_y);

						//font.load("Courier New Bold.ttf", 9);

						ofPopStyle();
					}
					//bodyIndexImg.draw(0, 0);
				}
			}
		}
		else if(type == 3) {
			foregroundImg.draw(screen_x, screen_y);
			//testImage.draw(screen_x, screen_y);
		}
	}

	// TV part
	count_time++;
	//ofBackground(0);
	//mImageBG.draw(0, 0);

	if (count_time > 600) {
		count_time = 0;
	}

	//play movie on channel 1 & 6

	if (count_rotate == 0 || count_rotate == 6) {
		mTrix.play();
		mTrix.draw(200, 225);
	}
	else {
		mTrix.stop();
	}

	//alternate BG image to show blinking light

	if (count_time < 300) {
		mImageBG.draw(0, 0);
	}
	else {
		mImageBGWeb.draw(0, 0);
	}



	//rotate first dial


	ofPushMatrix();
	ofTranslate(815 + (mImageDial.getWidth() / 2), 200 + (mImageDial.getHeight() / 2));
	ofRotate(30 * count_rotate, 0, 0, 1);
	mImageDial.draw(-mImageDial.getHeight() / 2, -mImageDial.getHeight() / 2);
	ofPopMatrix();
	ofLog(OF_LOG_NOTICE, "Click count is" + ofToString(count_rotate));


	//rotate second dial


	ofPushMatrix();
	ofTranslate(815 + (mImageDial2.getWidth() / 2), 300 + (mImageDial2.getHeight() / 2));
	ofRotate(30 * count_rotate2, 0, 0, 1);
	mImageDial2.draw(-mImageDial2.getHeight() / 2, -mImageDial2.getHeight() / 2);
	ofPopMatrix();
	ofLog(OF_LOG_NOTICE, "Click count2 is" + ofToString(count_rotate2));


}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

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
	int mDialOneX = 815 + (mImageDial.getWidth() / 2);
	int mDialOneY = 200 + (mImageDial.getHeight() / 2);
	int mDialTwoX = 815 + (mImageDial2.getWidth() / 2);
	int mDialTwoY = 300 + (mImageDial2.getHeight() / 2);


	//Finding the center point of the first dial and adding to the rotate count with each click
	if (abs(mouseX - mDialOneX) < (mImageDial.getWidth() / 2) && abs(mouseY - mDialOneY) < (mImageDial.getHeight() / 2)) {
		count_rotate++;
		if (count_rotate>11) {
			count_rotate = 0;
		}
	}

	//Finding the center point of the second dial and adding to the rotate count with each click
	if (abs(mouseX - mDialTwoX) < (mImageDial2.getWidth() / 2) && abs(mouseY - mDialTwoY) < (mImageDial2.getHeight() / 2)) {
		count_rotate2++;
		if (count_rotate2>11) {
			count_rotate2 = 0;
		}

	}
	
	type = count_pressed;

	count_pressed++;
	if (count_pressed == 5) {
		count_pressed = 0;
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

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
