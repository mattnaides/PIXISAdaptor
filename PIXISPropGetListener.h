/**
* @file:       DemoPropListener.h
*
* Purpose:     Class declaration for DemoPropListener.
*
* $Revision: 1.1.6.3 $
*
* $Authors:    JSH, CP, DT, KL $
*
* $Copyright 2001-2009 The MathWorks, Inc.$
*
* $Date: 2009/12/05 02:05:42 $
*
*/
#ifndef __PIXIS_PROP_GET_LISTENER_HEADER__
#define __PIXIS_PROP_GET_LISTENER_HEADER__

#include "mwadaptorimaq.h"  
#include "PIXISAdaptorClass.h"
//#include "mwdemoimaq.h"

/**
* Class DemoPropListener
*
* @brief:  Listens for changes in device-specific properties.
*
*/
class PIXISPropGetListener : public imaqkit::IPropCustomGetFcn{

public:

	// **************************************
	// CONSTRUCTOR/DESTRUCTOR
	// ************************************** 
	/**
	* Constructor for DemoPropListener class.
	*
	* @param parent: Handle to the instance of the IAdaptor class
	*                that is the parent of this object.
	*/
	 PIXISPropGetListener(PIXISAdaptorClass* parent) : _parent(parent) {}


	virtual ~PIXISPropGetListener() {};

	// *******************************************************************
	// METHODS FOR CONFIGURING AND UPDATING DEMO FEATURE PROPERTY VALUES.
	// *******************************************************************
	/**
	* This is the method the engine calls when a property value
	* changes. notify() casts the new property value to the appropriate
	* type and then calls the DemoPropListener::applyValue() method to
	* configure the property.
	*
	* @param propertyInfo: The property information object.
	* @param newValue: The new value of the property.
	*
	* @return void:
	*/
	//virtual void notify(imaqkit::IPropInfo* propertyInfo, void* newValue);
	virtual void getValue(imaqkit::IPropInfo* propertyInfo, void* value);

	
private:
	/**
	* applyValue: Find the property to update and configure it.
	*
	* @return void:
	*/
	//virtual void applyValue(void);

	/// The instance of the parent class that created this listener. 
	PIXISAdaptorClass* _parent;
	
	

};
#endif
