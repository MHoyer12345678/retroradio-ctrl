/*
 * SoundCardSetup.h
 *
 *  Created on: 16.02.2021
 *      Author: joe
 */

#ifndef SRC_SOUNDCARDSETUP_H_
#define SRC_SOUNDCARDSETUP_H_

#include <libudev.h>
#include <glib.h>

namespace retroradio_controller {

class SoundCardSetup {
public:
	class ISoundCardSetupListener
	{
	public:
		virtual void OnSoundCardReady()=0;

		virtual void OnSoundCardDisabled()=0;
	};

private:
	typedef struct DeviceReadyT
	{
		const char *devNode;
		bool available;

	} DeviceReadyT;

	DeviceReadyT *devNodeLst;

	ISoundCardSetupListener *eventListener;

	udev* udevObj;

	udev_monitor* udevMonitor;

	guint udevEventId;

	bool SetupUdevListener();

	void TriggerColdPluggedDevices();

	DeviceReadyT *FindDeviceItemByDevNode(const char *devNode);

	void TriggerColdPluggedDevice(const char *devNode);

	static gboolean OnUdevEvent(gint fd, GIOCondition condition, gpointer userData);

	void OnUdevEvent(udev_device* dev);

public:
	SoundCardSetup(ISoundCardSetupListener *aListener);

	virtual ~SoundCardSetup();

	bool Init();

	void DeInit();

	bool IsCardAvailable();
};

} /* namespace retroradio_controller */

#endif /* SRC_SOUNDCARDSETUP_H_ */
