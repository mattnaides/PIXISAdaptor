/**
* @file:       PIXISPropGetListener.cpp
*
* Purpose:     Implements custom get functions for getting parameters from PIXIS camera.
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

#include "assert.h"
#include "PIXISPropGetListener.h"
#include "picam_advanced.h"
#include <vector>
#include <algorithm>

//getValue returns the parameter value by casting void* void to the parameter value
void PIXISPropGetListener::getValue(imaqkit::IPropInfo* propertyInfo, void* value){
	const char* propname = propertyInfo->getPropertyName();
	int propertyID = propertyInfo->getPropertyIdentifier();

	PicamParameter parameter = static_cast<PicamParameter>(propertyID);
	PicamValueType type;
	PicamHandle camera;
	PicamHandle inputCamera;
	inputCamera = _parent->getCameraHandle();

	PicamAdvanced_GetCameraModel(inputCamera, &camera);
	PicamAdvanced_RefreshParametersFromCameraDevice(camera);  //This call is necessary to refresh parameters
															  //without it you won't get current values

	//Checks to see if the parameter is a ROI parameter
	if (PicamParameter_Rois == propertyID ||
		PicamParameter_Rois == propertyID -1 ||
		PicamParameter_Rois == propertyID -2 ||
		PicamParameter_Rois == propertyID -3 ||
		PicamParameter_Rois == propertyID -4 ||
		PicamParameter_Rois == propertyID -5 ||
		PicamParameter_Rois == propertyID -6){
		type = PicamValueType_Rois;
	}
	else{
		Picam_GetParameterValueType(camera, parameter, &type);
		const char* paramName;
		Picam_GetEnumerationString(PicamEnumeratedType_Parameter, parameter, &paramName);
	}

	piflt valueFloat = 0.0;
	piint valueInt;
	piint valueBool;
	piint valueEnum;
	
	//Calls the appropriate get function for the parameter type
	switch (type){
	case PicamValueType_Integer:
		
		Picam_GetParameterIntegerValue(camera, parameter, &valueInt);
		*reinterpret_cast<int*>(value) = valueInt;
		break;

	case PicamValueType_Boolean:

		Picam_GetParameterIntegerValue(camera, parameter, &valueBool);
		*reinterpret_cast<int*>(value) = valueBool;
		break;

	case PicamValueType_Enumeration:
		
		Picam_GetParameterIntegerValue(camera, parameter, &valueEnum);
		*reinterpret_cast<int*>(value) = valueEnum;
		
		break;
	case PicamValueType_LargeInteger:

		pi64s valueLargeInt;
		Picam_GetParameterLargeIntegerValue(camera, parameter, &valueLargeInt);
		*reinterpret_cast<int*>(value) = valueLargeInt;
		break;

	case PicamValueType_FloatingPoint:

		Picam_GetParameterFloatingPointValue(camera, parameter, &valueFloat);
		*reinterpret_cast<double*>(value) = valueFloat;
		break;

	case PicamValueType_Rois:
		const PicamRois *region;
		int subID;
		subID = propertyID - PicamParameter_Rois;
		
		
		Picam_GetParameterRoisValue(camera, PicamParameter_Rois, &region);

		switch (subID){
		case 1:
			*reinterpret_cast<int*>(value) = region->roi_array[0].height;
			break;
		case 2:
			*reinterpret_cast<int*>(value) = region->roi_array[0].width;
			break;
		case 3:
			*reinterpret_cast<int*>(value) = region->roi_array[0].x;
			break;
		case 4:
			*reinterpret_cast<int*>(value) = region->roi_array[0].y;
			break;
		case 5:
			*reinterpret_cast<int*>(value) = region->roi_array[0].x_binning;
			break;
		case 6:
			*reinterpret_cast<int*>(value) = region->roi_array[0].y_binning;
			break;
		}

		Picam_DestroyRois(region);
		break;
	case PicamValueType_Pulse:
		break;
	case PicamValueType_Modulations:
		break;
	default:
		*reinterpret_cast<int*>(value) = 22;
		break;
	}
}