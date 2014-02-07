/**
* @file:       PIXISPropSetListener.cpp
*
* Purpose:     Implements custom set function to update parameters on PIXIS camera.
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

#include "PIXISPropSetListener.h"
#include "picam_advanced.h"

void PIXISPropSetListener::notify(imaqkit::IPropInfo* propertyInfo, void* newValue) {
	if (newValue) {
		// Store a handle to the imaqkit::IPropInfo object passed in.
		_propInfo = propertyInfo;

		// Cast newValue to its appropriate type by checking the property type.
		switch (_propInfo->getPropertyStorageType()) {

			// _lastDoubleValue contains the value for double properties.
		case imaqkit::propertytypes::DOUBLE:
			_lastDoubleValue = *reinterpret_cast<piflt*>(newValue);
			break;

			// _lastIntValue contains value for integer properties.
			// For enumerated properties, _lastIntValue returns the ID number
			// associated with the property value.
		case imaqkit::propertytypes::INT:
			_lastIntValue = *reinterpret_cast<piint*>(newValue);
			break;

			// _lastStrValue contains value for string properties.
		case imaqkit::propertytypes::STRING:
			_lastStrValue = reinterpret_cast<pichar*>(newValue);
			break;

			// Since this adaptor only uses double, integer, or string
			// value properties, anything else should cause an assertion error.
		}

		// Do not re-configure the property value unless the device is already
		// opened.
		if (_parent->isOpen()) {
			// Apply the value to the hardware.
			applyValue();
		}
		applyValue();
	}
}

//Applies the parameter value to the PIXIS camera
void PIXISPropSetListener::applyValue() {

	// If device cannot be configured while acquiring data, stop the device,
	// configure the feature, then restart the device.

	bool wasAcquiring = _parent->isAcquiring();
	if (wasAcquiring) {
		// Note: calling stop() will change the acquiring flag to false.
		// When the device tries to restart it invokes
		// PIXISAdaptorClass::startCapture() which triggers notification for all
		// property listeners. Since the device is not acquiring data during
		// this second notification, the device will not stop and restart
		// again.
		_parent->stop();
	}

	// Get the property name and ID
	char* propName = const_cast<char*>(_propInfo->getPropertyName());
	int propertyID = _propInfo->getPropertyIdentifier();
	PicamParameter parameter = static_cast<PicamParameter>(propertyID);

	PicamValueType type;
	PicamHandle camera = _parent->getCameraHandle();      //Get the camera handle from the adaptor

	//Check to see if the parameter is a ROI parameter
	if (PicamParameter_Rois == propertyID ||
		PicamParameter_Rois == propertyID - 1 ||
		PicamParameter_Rois == propertyID - 2 ||
		PicamParameter_Rois == propertyID - 3 ||
		PicamParameter_Rois == propertyID - 4 ||
		PicamParameter_Rois == propertyID - 5 ||
		PicamParameter_Rois == propertyID - 6){
		type = PicamValueType_Rois;
	}
	else{
		//If not ROI, get parameter value type and name
		Picam_GetParameterValueType(camera, parameter, &type);
		const char* paramName;
		Picam_GetEnumerationString(PicamEnumeratedType_Parameter, parameter, &paramName);
	}
	
	const PicamParameter *failedParameterArray;
	piint failedParameterCount;

	switch (type){
	case PicamValueType_Integer:
		Picam_SetParameterIntegerValue(camera, parameter, _lastIntValue);
		Picam_CommitParameters(camera, &failedParameterArray,&failedParameterCount);
		if (failedParameterCount){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Failed to Commit a Parameter");
		}
		break;

	case PicamValueType_Boolean:

		
		Picam_SetParameterIntegerValue(camera, parameter, _lastIntValue);
		Picam_CommitParameters(camera, &failedParameterArray, &failedParameterCount);
		if (failedParameterCount){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Failed to Commit a Parameter");
		}
		break;

	case PicamValueType_Enumeration:
		if (_lastIntValue == PicamReadoutControlMode_Kinetics && parameter == PicamParameter_ReadoutControlMode){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "It's a Kinetics Update!");
			Picam_SetParameterIntegerValue(camera, PicamParameter_TriggerResponse, 5);

			Picam_SetParameterIntegerValue(camera, parameter, _lastIntValue);
			Picam_SetParameterFloatingPointValue(camera, PicamParameter_ExposureTime, 10);
			Picam_SetParameterIntegerValue(camera, PicamParameter_KineticsWindowHeight, 128);
			const PicamRoisConstraint   *ptrConstraint;	 // Constraints			

			PicamRois				    roiSetup;		 // region of interest  setup
			PicamRoi					theKineticsROI;
			piint framesPerReadout = 0;
			// Variables to compute data blob sizes 
			piint readoutHeight = 0;      // Height of Readout (in pixels)
			piint readoutWidth = 0;      // Width  of Readout (in pixels)
			piint singleFrameHeight = 0;      // Height of single Kinetics frame (in pixels)

			Picam_GetParameterRoisConstraint(camera, PicamParameter_Rois, PicamConstraintCategory_Required, &ptrConstraint);

			// Get width and height of accessible region from current constraints 
			readoutWidth = (piint)ptrConstraint->width_constraint.maximum;
			singleFrameHeight = (piint)ptrConstraint->height_constraint.maximum;

			// Deallocate constraints after using access 
			Picam_DestroyRoisConstraints(ptrConstraint);
			Picam_GetParameterIntegerValue(camera,PicamParameter_FramesPerReadout, &framesPerReadout);
			readoutHeight = singleFrameHeight * framesPerReadout;

			// setup the roiSetup object count and pointer 
			roiSetup.roi_count = 1;
			roiSetup.roi_array = &theKineticsROI;

			// The ROI structure should hold the size of a single kinetics capture ROI 
			theKineticsROI.height = singleFrameHeight;
			theKineticsROI.width = readoutWidth;

			// The ROI structure should hold the location of this ROI (upper left corner of window)
			theKineticsROI.x = 0;
			theKineticsROI.y = 0;

			// The vertical and horizontal binning 
			theKineticsROI.x_binning = 1;
			theKineticsROI.y_binning = 1;
			Picam_SetParameterRoisValue(camera, PicamParameter_Rois, &roiSetup);
		}
		else{
			Picam_SetParameterIntegerValue(camera, parameter, _lastIntValue);
		}
		
		Picam_CommitParameters(camera, &failedParameterArray, &failedParameterCount);
		if (failedParameterCount){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Failed to Commit a Parameter");
		}

		if (_lastIntValue == PicamReadoutControlMode_Kinetics && parameter == PicamParameter_ReadoutControlMode){
			const PicamCollectionConstraint* capable;
			const pichar* stringEnum;
			Picam_GetParameterCollectionConstraint(camera, PicamParameter_TriggerResponse, PicamConstraintCategory_Capable, &capable);
			
			PicamHandle newCamera;
			PicamAdvanced_GetCameraModel(camera, &newCamera);
			PicamAdvanced_RefreshParametersFromCameraDevice(newCamera);
			Picam_GetParameterCollectionConstraint(newCamera, PicamParameter_TriggerResponse, PicamConstraintCategory_Capable, &capable);

			for (piint j = 0; j < capable->values_count; ++j){
				Picam_GetEnumerationString(PicamEnumeratedType_TriggerResponse, capable->values_array[j], &stringEnum);
				imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", stringEnum);
			}
			Picam_DestroyString(stringEnum);
			Picam_DestroyCollectionConstraints(capable);

		}
		break;


	case PicamValueType_LargeInteger:

		Picam_SetParameterIntegerValue(camera, parameter, _lastIntValue);
		Picam_CommitParameters(camera, &failedParameterArray, &failedParameterCount);
		if (failedParameterCount){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Failed to Commit a Parameter");
		}
		break;

	case PicamValueType_FloatingPoint:
		imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "It's a floater");
		Picam_SetParameterFloatingPointValue(camera, parameter, _lastDoubleValue);
		Picam_CommitParameters(camera, &failedParameterArray, &failedParameterCount);
		if (failedParameterCount){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Failed to Commit a Parameter");
		}
		break;

	case PicamValueType_Rois:
		
		const PicamRois *region;
		PicamHandle modelCamera;
		PicamAdvanced_GetCameraModel(camera, &modelCamera);
		PicamAdvanced_RefreshParametersFromCameraDevice(modelCamera);
		Picam_GetParameterRoisValue(camera, PicamParameter_Rois, &region);

		int subID;
		subID = propertyID - PicamParameter_Rois;
		switch (subID){
		case 1:
			region->roi_array[0].height = _lastIntValue;
			break;
		case 2:
			region->roi_array[0].width = _lastIntValue;
			break;
		case 3:
			region->roi_array[0].x = _lastIntValue;
			break;
		case 4:
			region->roi_array[0].y = _lastIntValue;
			break;
		case 5:
			region->roi_array[0].x_binning = _lastIntValue;
			break;
		case 6:
			region->roi_array[0].y_binning = _lastIntValue;
			break;
		}
		Picam_SetParameterRoisValue(camera, PicamParameter_Rois, region);
		Picam_DestroyRois(region);
		Picam_CommitParameters(camera, &failedParameterArray, &failedParameterCount);
		if (failedParameterCount){
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Failed to Commit a Parameter");
		}
		break;
	case PicamValueType_Pulse:
		break;
	case PicamValueType_Modulations:
		break;
	}

	// Restart the device if it was momentarily stopped to update the feature.
	if (wasAcquiring) {
		// Restart the device. This invokes DemoAdaptor::startCapture() which
		// invoke all property listeners.
		_parent->restart();
	}
	Picam_DestroyParameters(failedParameterArray);
}

