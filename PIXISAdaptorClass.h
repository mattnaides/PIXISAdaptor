#ifndef __PIXIS_ADAPTOR_HEADER__
#define __PIXIS_ADAPTOR_HEADER__

#include "mwadaptorimaq.h" // required header
#include <Windows.h>
#include "picam.h"

class PIXISAdaptorClass : public imaqkit::IAdaptor {

public:

	// Constructor and Destructor
	PIXISAdaptorClass(imaqkit::IEngine* engine,
		const imaqkit::IDeviceInfo* deviceInfo,
		const char* formatName);

	PicamHandle PIXISAdaptorClass::getCameraHandle() const;
	PicamCameraID PIXISAdaptorClass::getCameraID() const;
	PicamAvailableData PIXISAdaptorClass::getCameraData() const;
	PicamAcquisitionErrorsMask PIXISAdaptorClass::getCameraErrors() const;

	virtual ~PIXISAdaptorClass();


	// Adaptor and Image Information Functions
	virtual const char* getDriverDescription() const;
	virtual const char* getDriverVersion() const;
	virtual int getMaxWidth() const;
	//virtual int getMaxWidth();
	virtual int getMaxHeight() const;
	virtual int getNumberOfBands() const;
	virtual int getXOffset() const;
	virtual int getYOffset() const;
	virtual int getFramesPerReadout() const;
	virtual int getFrameStride() const;
	virtual imaqkit::frametypes::FRAMETYPE getFrameType() const;

	bool PIXISAdaptorClass::isKineticsMode() const;
	// Image Acquisition Functions
	virtual bool openDevice();
	virtual bool closeDevice();
	virtual bool startCapture();
	virtual bool stopCapture();


private:
	// Declereation of acquisition thread function
	static DWORD WINAPI acquireThread(void* param);
	bool PIXISAdaptorClass::isAcquisitionActive(void) const;
	void PIXISAdaptorClass::setAcquisitionActive(bool state);
	// Thread variable
	HANDLE _acquireThread;

	// Thread ID returned by Windows.
	DWORD _acquireThreadID;

	imaqkit::ICriticalSection* _driverGuard;

	imaqkit::ICriticalSection* _grabSection;
	/// The acquisition active critical section.
	imaqkit::ICriticalSection* _acquisitionActiveGuard;

	/// Handle to the engine property container.
	imaqkit::IPropContainer* _enginePropContainer;

	bool _acquisitionActive;

	PicamHandle _camera;
	PicamCameraID _id;
	PicamAvailableData _data;
	PicamAcquisitionErrorsMask _errors;
};
#endif