/**
* @file:       PIXISAdaptorClass.cpp
*
* Purpose:     Implements adaptor Class functions.
*
* $Revision: 1.0$
*
* $Authors:    Matt Naides $
*
* $Date: 2014/31/01 14:26:41 $
*
* This program borrows heavily from the Mathworks demo adaptor kit for the
* Image Acquisition Toolbox.
*/

#include "PIXISPropGetListener.h"

#include "PIXISAdaptorClass.h"
#include "PIXISPropSetListener.h"

#include "mwadaptorimaq.h"
#include "picam.h"
#include <Windows.h>
#include <WinUser.h>
#include <iostream>
#include <fstream>

// Class constructor
PIXISAdaptorClass::PIXISAdaptorClass(imaqkit::IEngine* engine,
	const imaqkit::IDeviceInfo* deviceInfo,
	const char* formatName):imaqkit::IAdaptor(engine){

	if (Picam_OpenFirstCamera(&_camera) == PicamError_None)    //Attempts to open the first camera it sees
		Picam_GetCameraID(_camera, &_id);
	else                                                       //If no cameras found, connect a demo camera
	{
		Picam_ConnectDemoCamera(
			PicamModel_Pixis100F,
			"0008675309",
			&_id);
		Picam_OpenCamera(&_id, &_camera);
		printf("No Camera Detected, Creating Demo Camera\n");
	}

	_acquisitionActiveGuard = imaqkit::createCriticalSection();       //Creates critical sections which guards code from being
																	  //accessed simultaneously
	_grabSection = imaqkit::createCriticalSection();

	//Creates IPropContainer which contains all of the device properties added in
	//PIXISAdaptor_fncs
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();

	int numDeviceProps = propContainer->getNumberProps();
	const char **devicePropNames = new const char*[numDeviceProps];

	propContainer->getPropNames(devicePropNames);  //Gets the property names from propContainer and adds them to devicePropNames

	//Loop through each property and create a custom get and set listener for each
	for (int i = 0; i < numDeviceProps; i++){
		imaqkit::IPropInfo* propInfo = propContainer->getIPropInfo(devicePropNames[i]);
		int id = propInfo->getPropertyIdentifier();
		if (id){
			propContainer->addListener(devicePropNames[i], new PIXISPropSetListener(this));
			propContainer->setCustomGetFcn(devicePropNames[i], new PIXISPropGetListener(this));
		}
	}
	delete [] devicePropNames;
	}



// Class destructor
PIXISAdaptorClass::~PIXISAdaptorClass(){
	Picam_CloseCamera(_camera);
}

// Device driver information functions
PicamHandle PIXISAdaptorClass::getCameraHandle() const{
	return _camera;
}
PicamAcquisitionErrorsMask PIXISAdaptorClass::getCameraErrors() const{
	return _errors;
}
PicamAvailableData PIXISAdaptorClass::getCameraData() const{
	return _data;
}
PicamCameraID PIXISAdaptorClass::getCameraID() const{
	return _id;
}
const char* PIXISAdaptorClass::getDriverDescription() const{
	return "PIXISCamera_Driver";
}
const char* PIXISAdaptorClass::getDriverVersion() const {
	return "1.0.0";
}

//isAcquisitionActive returns _acquisitionActive, which flags whether we are actively acquiring.
bool PIXISAdaptorClass::isAcquisitionActive(void) const {
	return _acquisitionActive;
}
//isAcquisitionActive sets _acquisitionActive, which flags whether we are actively acquiring.
void PIXISAdaptorClass::setAcquisitionActive(bool state) {
	_acquisitionActive = state;
}

//isKineticsMode returns true if the current Readout Control Mode parameter is Kinetics
bool PIXISAdaptorClass::isKineticsMode() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	int* control = static_cast<int*>(propContainer->getPropValue("Readout_Control_Mode"));
	if (*control == PicamReadoutControlMode_Kinetics){
		return true;
	}
	else{
		return false;
	}
}
int PIXISAdaptorClass::getFramesPerReadout() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	//int* output = static_cast<int*>(propContainer->getPropValue("Frames_Per_Readout"));
	int* output = static_cast<int*>(propContainer->getPropValue("Frames_per_Readout"));
	return *output;
}
//getMaxWidth returns the ROI width
int PIXISAdaptorClass::getMaxWidth() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	//int* output = static_cast<int*>(propContainer->getPropValue("ROIWidth"));
	int* width = static_cast<int*>(propContainer->getPropValue("ROIWidth"));
	int* xBinning = static_cast<int*>(propContainer->getPropValue("ROIXBinning"));
	int output = *width / *xBinning;
	//return *output;
	return output;

}

//getMaxHeight returns the ROI height
int PIXISAdaptorClass::getMaxHeight() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	//int* output = static_cast<int*>(propContainer->getPropValue("ROIHeight"));
	int* height = static_cast<int*>(propContainer->getPropValue("ROIHeight"));
	int* yBinning = static_cast<int*>(propContainer->getPropValue("ROIYBinning"));
	int numFrames = getFramesPerReadout();
	//int numFrames = 1;
	int output = *height * numFrames / *yBinning;
	//return *output;
	return output;

}

//getXOffset returns the ROI X offset
int PIXISAdaptorClass::getXOffset() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	int* output = static_cast<int*>(propContainer->getPropValue("ROIXOffset"));
	return *output;
}

//getYOffset returns the ROI Y offset
int PIXISAdaptorClass::getYOffset() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	int* output = static_cast<int*>(propContainer->getPropValue("ROIYOffset"));
	return *output;
}

int PIXISAdaptorClass::getFrameStride() const{
	imaqkit::IPropContainer* propContainer = getEngine()->getAdaptorPropContainer();
	int* output = static_cast<int*>(propContainer->getPropValue("Frames_Stride"));
	return *output;
}
int PIXISAdaptorClass::getNumberOfBands() const { return 1; }

imaqkit::frametypes::FRAMETYPE PIXISAdaptorClass::getFrameType()
const {
	return imaqkit::frametypes::MONO16;
}

// The startCapture() method posts a message to this ThreadProc to see if 
// the acquisition is complete. This method calls the 
// imaqkit::IAdaptor::isAcquisitionNotComplete method to see if the requested 
// number of frames have been acquired. It also checks _acquisitionActive
DWORD WINAPI PIXISAdaptorClass::acquireThread(void* param) {

	PIXISAdaptorClass* adaptor = reinterpret_cast<PIXISAdaptorClass*>(param);

	MSG msg;
	pi64s NUM_FRAMES = 1;        //The PIXIS camera will only acquire one frame per trigger/readout
	piint TIMEOUT =3000;      //We set the timeout to 3s so we do not get stuck in Picam_Acquire() waiting for a trigger
	
	// While the msg is not WM_QUIT
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		switch (msg.message) {
		case WM_USER:
			// Create the autoCriticalSection
			std::auto_ptr<imaqkit::IAutoCriticalSection> acquisitionActiveGuard(imaqkit::createAutoCriticalSection(adaptor->_acquisitionActiveGuard, true));
			PicamHandle _camera = adaptor->getCameraHandle();
			PicamAvailableData _data;
			PicamAcquisitionErrorsMask _errors = adaptor->getCameraErrors();

			//While we still need to acquire
			while (adaptor->isAcquisitionNotComplete() && adaptor->isAcquisitionActive()) {
				//Enter the autoCriticalSection
				acquisitionActiveGuard->enter();

				//Calls Picam_Acquire.  If Picam_Acquire does not time out, go on to sendFrame, otherwise continue through the loop
				if (PicamError_TimeOutOccurred != Picam_Acquire(_camera, NUM_FRAMES, TIMEOUT, &_data, &_errors)){
					if (adaptor->isSendFrame()) {
						// Get frame type & dimensions.
						imaqkit::frametypes::FRAMETYPE frameType =
							adaptor->getFrameType();
						int imWidth = adaptor->getMaxWidth();
						int imHeight = adaptor->getMaxHeight();						

						int imBands = adaptor->getNumberOfBands();
						int XOffset = adaptor->getXOffset();
						int YOffset = adaptor->getYOffset();
						// Create a frame object.
						imaqkit::IAdaptorFrame* frame =
							adaptor->getEngine()->makeFrame(frameType,
							imWidth,
							imHeight);

						// Copy data from buffer into frame object.
						frame->setImage((pibyte*)_data.initial_readout,
							imWidth,
							imHeight,
							0, // X Offset from origin
							0); // Y Offset from origin

						// Set image's timestamp.
						frame->setTime(imaqkit::getCurrentTime());

						// Send frame object to engine.
						adaptor->getEngine()->receiveFrame(frame);
					}
					else{
						imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Not sending that frame!");
					}
					// if isSendFrame()
					// Increment the frame count.
					adaptor->incrementFrameCount();
				}
				if (adaptor->getFrameCount() >= adaptor->getTotalFramesPerTrigger()){
					adaptor->setAcquisitionActive(false);
				}
				acquisitionActiveGuard->leave();   //Leave the criticalSection
			} // while(isAcquisitionNotComplete() 
			break;
		} //switch-case WM_USER

	} //while message is not WM_QUIT

	return 0;
}

// Set up the device for acquisition.
bool PIXISAdaptorClass::openDevice() { 

	// Checks if connection is already open.  If so, returns true
	if (isOpen()){
		return true;
	}
	_acquireThread = CreateThread(NULL, 0, acquireThread, this, 0, &_acquireThreadID);  //Creates the image acquisition thread
	if (_acquireThread == NULL){
		closeDevice();
		return false;
	}
	while (PostThreadMessage(_acquireThreadID, WM_USER + 1, 0, 0) == 0)
		Sleep(1);
	return true;
}

//Closes the camera
bool PIXISAdaptorClass::closeDevice(){

	if (!isOpen())
		return true;
	if (_acquireThread){
		//Send WM_QUIT message to thread
		PostThreadMessage(_acquireThreadID, WM_QUIT, 0, 0);
		// Give the thread a chance to finish
		WaitForSingleObject(_acquireThread, 10000);
		//Close thread handle
		CloseHandle(_acquireThread);
		_acquireThread = NULL;
	}
	return true;
}

//Starts the capture
bool PIXISAdaptorClass::startCapture(){
	//Check if device is already acquiring frames.
	if (isAcquiring())
		return false;

	PostThreadMessage(_acquireThreadID, WM_USER, 0, 0);
	setAcquisitionActive(true);

	return true; 
}

//Stops capture
bool PIXISAdaptorClass::stopCapture(){ 
	if (!isOpen()){
		return true;
	}
		
	std::auto_ptr<imaqkit::IAutoCriticalSection> GrabSection(imaqkit::createAutoCriticalSection(_grabSection, true));
	setAcquisitionActive(false);
	GrabSection->leave();
	return true;
}