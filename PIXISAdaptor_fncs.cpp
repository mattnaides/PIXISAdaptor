/**
* @file:       PIXISAdaptor_fncs.cpp
*
* Purpose:     Implements adaptor information functions.
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



#include "mwadaptorimaq.h"
#include "pil_platform.h"
#include "picam.h"
#include "picam_advanced.h"
#include "PIXISAdaptorClass.h"
#include <vector>
#include <algorithm>

imaqkit::IAdaptor* adaptor;

/**
* initializeAdaptor: Exported function to initialize the adaptor.
* This function is called directly after the adaptor DLL is loaded into
* memory and gives the adaptor a chance to perform initializations before
* any adaptor object is created.
*/
void initializeAdaptor(){
	Picam_InitializeLibrary();
}

/**
* getAvailHW: Exported function to enumerate all the hardware to which
* the adaptor interfaces. The imaqkit::IEngine calls this function when the
* IMAQHWINFO function is called on the MATLAB command line.
*
* Adaptors can query the device's SDK to find out what devices are available.
* Adaptors can also query the SDK for information about the formats supported
* by each device. If this format information is available in advance, an adaptor
* writer can store it in an IMDF file.
*
* @param imaqkit:IHardwareInfo*: The imaqkit::IEngine passes a handle to a hardware
*                                information container. Adaptors use this container
*                                to communicate all the hardware supported by the adaptor.
*/
void getAvailHW(imaqkit::IHardwareInfo* hardwareContainer){


	imaqkit::IDeviceInfo* deviceInfo = hardwareContainer->createDeviceInfo(1, "PIXIS_Camera");

	deviceInfo->setDeviceFileSupport(false);
	imaqkit::IDeviceFormat* deviceFormat = deviceInfo->createDeviceFormat(1, "PIXIS_Camera");

	deviceInfo->addDeviceFormat(deviceFormat, true);
	hardwareContainer->addDevice(deviceInfo);
	
}

struct SortAlphabetically
{
	pibool operator()(PicamParameter a, PicamParameter b) const
	{
		// - get the string for first parameter
		const pichar* string;
		Picam_GetEnumerationString(
			PicamEnumeratedType_Parameter,
			a,
			&string);
		std::string string_a(string);
		Picam_DestroyString(string);

		// - get the string for the second parameter
		Picam_GetEnumerationString(
			PicamEnumeratedType_Parameter,
			b,
			&string);
		std::string string_b(string);
		Picam_DestroyString(string);

		// - use default C++ comparison
		return string_a < string_b;
	}
};

/**
* Sorts a vector of parameters alphabetically.  The primary purpose of this is for easier to
* read display in MATLAB.
*/

std::vector<PicamParameter> SortParameters(
	const PicamParameter* parameters,
	piint count)
{
	std::vector<PicamParameter> sorted(parameters, parameters + count);
	std::sort(sorted.begin(), sorted.end(), SortAlphabetically());
	return sorted;
}

/**
* removeSpaces takes a string and replaces any spaces with '_'
* MATLAB structs can't have spaces in the field names so we have to replace the spaces.
*/
const pichar* removeSpaces(const pichar* input){
	const int stringLength = strlen(input);
	pichar tempstr[100];	

	int j = 0;
	for (int i = 0; i < stringLength; i++){
		if (input[i] != ' '){
			tempstr[j] = input[i];			
		}
		else{
			tempstr[j] = '_';
		}
			j++;
	}
	tempstr[j] = 0;
	const pichar* output(tempstr);
	return output;
	
}
/**
* addEnumeratedParameter adds an enumerated parameter to the IPropFactory devicePropFact.
* The PIXIS camera is queried for the current value and the list of possible values.
* These values are then added to the devicePropFact.
*/
void addEnumeratedParameter(imaqkit::IPropFactory* devicePropFact, PicamParameter parameter, PicamHandle camera, const pichar* stringNoSpaces, void* hProp){
	piint valueEnum;
	PicamEnumeratedType enumType;
	const pichar* stringEnum;
	const PicamCollectionConstraint* capable;

	Picam_GetParameterIntegerValue(camera, parameter, &valueEnum);     //Gets the integer value of parameter
	Picam_GetParameterEnumeratedType(camera, parameter, &enumType);    //Gets the type of parameter.  See Picam documentation for types
	Picam_GetEnumerationString(enumType, valueEnum, &stringEnum);      //Gets the string associated with the parameter

	// Creates an Enum property with value stringEnum/valueEnum
	hProp = devicePropFact->createEnumProperty(stringNoSpaces, stringEnum, valueEnum);   
	devicePropFact->setIdentifier(hProp, parameter);

	// Gets the allowed values of parameter
	Picam_GetParameterCollectionConstraint(camera, parameter, PicamConstraintCategory_Capable, &capable);

	// Loop through the allowed values in capable and add them to hprop
	for (piint j = 0; j < capable->values_count; ++j){
		if (capable->values_array[j] != valueEnum){
			Picam_GetEnumerationString(enumType, capable->values_array[j], &stringEnum);
			devicePropFact->addEnumValue(hProp, stringEnum, capable->values_array[j]);
		}
	}
	
	Picam_DestroyString(stringEnum);                   // The memory allocated for stringEnum must be opened up
	Picam_DestroyCollectionConstraints(capable);       // The memory allocated for capable must be opened up

	devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);   // Sets the property to be read online while running.
	devicePropFact->addProperty(hProp);
}

/**
* getDeviceAttributes() -- Exported function used to dynamically add device-specific
* properties. The imaqkit::IEngine calls this function when a user creates a
* video input object by calling the VIDEOINPUT function at the MATLAB command line.
*
* Adaptors can query the device's SDK to determine these properties or, if the
* information is available in advance, store it in an IMDF file and read the
* information from the file.
*
* Adaptors create property objects and use methods of the imaqkit::IPropFactory object
* to store the properties in the device-specific property container.
*
* @param deviceInfo: The imaqkit::IEngine passes a handle to an imaqkit::IDeviceInfo object
*                    identifying the target device. The handle will be one of
*                    the IDeviceInfo objects created in getAvailHW().
*
* @param sourceType: The imaqkit::IEngine passes a text string that specifies the target
*                    source type. This can be either one of the fixed format names
*                    specified for the device in getAvailHW() or a filename.
*                    If it's a file name, it is the name of a device configuration
*                    file (also known as a camera file).
*                    The imaqkit::IEngine performs no processing on device configuration files.
*                    Your adaptor should just pass this file to your device.
*
* @param devicePropFact: The imaqkit::IEngine passes a handle to an imaqkit::IPropFactory object.
*                        This object supports methods you use to create and add device-specific properties.
*
* @param sourceContainer: The imaqkit::IEngine passes a handle to an imaqkit::IVideoSourceInfo
*                         object. You use this object to identify the device-specific
*                         video sources.
*
*                         NOTE: To be able to create a videoinput object in MATLAB,
*                               your adaptor must identify at least one video source.
*
* @param hwTriggerInfo: The imaqkit::IEngine passes a handle to an imaqkit::ITriggerInfo object.
*                       You use this object to create and add valid hardware
*                       trigger configurations. Manual and immediate trigger
*                         configurations are handled by the IMAQ engine automatically.
*
*/
void getDeviceAttributes(const imaqkit::IDeviceInfo* deviceInfo,
	const char* formatName,
	imaqkit::IPropFactory* devicePropFact,
	imaqkit::IVideoSourceInfo* sourceContainer,
	imaqkit::ITriggerInfo* hwTriggerInfo){

	// Create a video source
	void* hProp;  // Declare a handle to a property object.

	const PicamParameter* parameters;
	piint count;
	PicamHandle camera;

	/** Opens the first available Princeton Instruments camera.  Usually the handle to the camera is
	* Stored in the PIXISAdaptorClass but since the image acquisiton toolbox hasn't created an instance
	* of that class yet we have to open and then close the camera here to get the parameters from the camera
	*/
	Picam_OpenFirstCamera(&camera); 
	Picam_GetParameters(camera, &parameters, &count);
	std::vector<PicamParameter> sorted = SortParameters(parameters, count);
	Picam_DestroyParameters(parameters);
	PicamValueType type;

	piint j = 0;
	
	// Loops over each of the parameters
	for (piint i = 0; i < count; ++i)
	{
		const pichar* string;
		Picam_GetEnumerationString(PicamEnumeratedType_Parameter, sorted[i], &string);  //Gets the name of the parameter

		Picam_GetParameterValueType(camera, sorted[i], &type);							//Gets the value of the parameter
		const pichar* stringNoSpaces = removeSpaces(string);							//Replaces spaces in parameter names with '_'
		PicamConstraintType constraint_type;
		Picam_DestroyString(string);
		switch (type){
		case PicamValueType_Integer:
			//Adds Int pararemeter to hProp
			piint valueInt;
			Picam_GetParameterIntegerValue(camera, sorted[i], &valueInt);
			hProp = devicePropFact->createIntProperty(stringNoSpaces, valueInt);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i]);
			devicePropFact->addProperty(hProp);
			break;

		case PicamValueType_Boolean:
			//Adds bool parameter to hProp, but we just treat it like an integer
			piint valueBool;
			Picam_GetParameterIntegerValue(camera, sorted[i], &valueBool);
			hProp = devicePropFact->createIntProperty(stringNoSpaces, valueBool);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i]);
			devicePropFact->addProperty(hProp);
			break;

		case PicamValueType_Enumeration:
			//Adds enum parameter to hProp via addEnumeratedParameter method
			Picam_GetParameterConstraintType(camera,sorted[i],&constraint_type);
			switch (constraint_type){
				case PicamConstraintType_Collection:
					addEnumeratedParameter(devicePropFact, sorted[i], camera, stringNoSpaces, hProp);
			}
			
			break;

		case PicamValueType_LargeInteger:
			//Adds a large integer paraemter to hProp, but the toolbox treats it like an int
			pi64s valueLargeInt;
			Picam_GetParameterLargeIntegerValue(camera, sorted[i], &valueLargeInt);
			hProp = devicePropFact->createIntProperty(stringNoSpaces, valueLargeInt);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i]);
			devicePropFact->addProperty(hProp);
			break;

		case PicamValueType_FloatingPoint:
			//Adds float parameter to hProp
			piflt valueFloat;
			Picam_GetParameterFloatingPointValue(camera, sorted[i], &valueFloat);
			hProp = devicePropFact->createDoubleProperty(stringNoSpaces, valueFloat);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i]);
			devicePropFact->addProperty(hProp);
			break;

		case PicamValueType_Rois:
			/**Adds ROI parameter to hProp.  The code currently only supports one ROI.
			* I opt to store ROI information as six int parameters instead of an IntArray parameter
			*/
			const PicamRois *region;
			Picam_GetParameterRoisValue(camera, sorted[i], &region);   //Gets the ROI from the camera
			
			int height;     //Height of the ROI
			int width;      //Width of the ROI
			int x_offset;   //X offset of the ROI
			int y_offset;   //Y offset of the ROI
			int x_binning;  
			int y_binning;
			
			height = region->roi_array[0].height;
			width = region->roi_array[0].width;

			x_offset = region->roi_array[0].x;
			y_offset = region->roi_array[0].y;

			x_binning = region->roi_array[0].x_binning;
			y_binning = region->roi_array[0].y_binning;
			
			Picam_DestroyRois(region);

			hProp = devicePropFact->createIntProperty("ROIHeight", height);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i]+1);
			devicePropFact->addProperty(hProp);

			hProp = devicePropFact->createIntProperty("ROIWidth", width);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i] + 2);
			devicePropFact->addProperty(hProp);

			hProp = devicePropFact->createIntProperty("ROIXOffset", x_offset);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i] +3);
			devicePropFact->addProperty(hProp);

			hProp = devicePropFact->createIntProperty("ROIYOffset", y_offset);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i] + 4);
			devicePropFact->addProperty(hProp);

			hProp = devicePropFact->createIntProperty("ROIXBinning", x_binning);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i] + 5);
			devicePropFact->addProperty(hProp);

			hProp = devicePropFact->createIntProperty("ROIYBinning", y_binning);
			devicePropFact->setPropReadOnly(hProp, imaqkit::propreadonly::WHILE_RUNNING);
			devicePropFact->setIdentifier(hProp, sorted[i] + 6);
			devicePropFact->addProperty(hProp);
			break;
		case PicamValueType_Pulse:
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Pulse");
			break;
		case PicamValueType_Modulations:
			imaqkit::adaptorWarn("PIXISCameraAdaptor:hoot", "Modulatoins");
			break;
		}

	}
	

	Picam_CloseCamera(camera);   //Closes the camera and frees up any memory associated with it

	sourceContainer->addAdaptorSource("PIXIS_Camera_Source", 1);
}

/**
* createInstance() -- Exported function to return a new instance of an adaptor object.
* The imaqkit::IEngine calls this function when a user attempts to create
* a video input object by calling the VIDEOINPUT function at the MATLAB command line.
*
* @param engine: The imaqkit::IEngine passes a handle to an imaqkit::IEngine object to which the
*                adaptor will interface.
*
* @param deviceInfo: The imaqkit::IEngine passes a handle to an imaqkit::IDeviceInfo object
*                    identifying the target device. The handle will be one of
*                    the IDeviceInfo objects created in getAvailHW().
*
* @param sourceType: The given source type ('format') given.  Can be one of the
*                    format names specified for the specific device or the name
*                    of a device configuration file.
*/

imaqkit::IAdaptor* createInstance(imaqkit::IEngine* engine,
	const imaqkit::IDeviceInfo* deviceInfo,
	const char* formatName){

	imaqkit::IAdaptor* adaptor = new PIXISAdaptorClass(engine, deviceInfo, formatName);
	return  adaptor;
}

/**
* uninitializeAdaptor: Exported function to uninitialize the adaptor.
* This function is called just before the adaptor DLL is unloaded when the
* state of the toolbox is reset using IMAQRESET or when MATLAB exits.
* This function gives the adaptor a chance to perform any clean-up tasks
* such as deleting any dynamically allocated memory not covered in the
* adaptor instance's destructor. This function will be called after the
* destructor for all existing adaptor objects have been invoked.
*/
void uninitializeAdaptor(){
	Picam_UninitializeLibrary();
}