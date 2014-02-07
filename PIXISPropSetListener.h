#ifndef __PIXIS_PROP_SET_LISTENER_HEADER__
#define __PIXIS_PROP_SET_LISTENER_HEADER__

#include "mwadaptorimaq.h"
#include "PIXISAdaptorClass.h"

class PIXISPropSetListener : public imaqkit::IPropPostSetListener{
public:
	PIXISPropSetListener(PIXISAdaptorClass* parent) :_parent(parent){}

	virtual ~PIXISPropSetListener(){};

	virtual void notify(imaqkit::IPropInfo* propertyInfo, void* newValue);

private:
	PIXISAdaptorClass* _parent;

	/**
	* applyValue: Find the property to update and configure it.
	*
	* @return void:
	*/
	virtual void applyValue(void);

	/// Property Information object.
	imaqkit::IPropInfo* _propInfo;

	/// The new value for integer properties.
	piint _lastIntValue;

	/// The new value for double properties.
	piflt _lastDoubleValue;

	/// The new value for string properties.
	pichar* _lastStrValue;
};
#endif