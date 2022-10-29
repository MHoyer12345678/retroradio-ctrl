/*
 * RemoteController.h
 *
 *  Created on: 23.11.2019
 *      Author: joe
 */

#ifndef SRC_REMOTECONTROLLER_H_
#define SRC_REMOTECONTROLLER_H_

#include <glib.h>
#include <linux/lirc.h>

#include <cpp-app-utils/Configuration.h>

#include "RemoteControllerProfiles.h"

extern "C"
{
	typedef struct lirc_scancode lirc_scancode_t;
}

using namespace CppAppUtils;

namespace retroradio_controller {

class RemoteController : public Configuration::IConfigurationParserModule {

public:
	class IRemoteControllerListener {
	public:
		virtual void OnCommandReceived(RemoteControllerProfiles::RemoteCommand cmd)=0;
	};

private:
	RemoteControllerProfiles *remoteControllerProfiles;

	IRemoteControllerListener *listener;

	char *inputDeviceName;

	char *remoteProfileName;

	guint softwareRepeatDetectorTimerId;

	bool softwarRepeatDetectorDelayEnabled;

	unsigned long lastScanCode;

	guint lircEventSrc;

	int pollFd;

	int retryCntr;

	guint defereTimerId;

	int InitializeLIRC();

	int SetIRDeviceProtocol(const char *lircDevice);

	int EnableIRDeviceReceiveMode(const char *lircDevice);

	void DefereInitalization();

	static gboolean RetryInit(gpointer data);

	const char *GetLircDeviceName();

	const char *GetRemoteProfileName();

	static gboolean OnLircEvent(gint fd, GIOCondition condition, gpointer user_data);

	void ReadLircEvent();

	void ParseScanCodes(lirc_scancode_t *scanCodes, unsigned int numberOfCodes);

	bool CheckSoftwareRepeatDetector(unsigned long scancode, bool repeated);

	static gboolean SoftwareRepeatDetectorTimeoutFunc(gpointer data);

	bool FilterRepeatedCmds(RemoteControllerProfiles::RemoteCommand cmd, bool repeated, bool toggled);

	void ProcessScanCode(unsigned long scancode, bool repeated, bool toggled);

public:
	RemoteController(IRemoteControllerListener *theListener, Configuration *configuration);

	virtual ~RemoteController();

	bool Init();

	void DeInit();

	virtual bool ParseConfigFileItem(GKeyFile *confFile, const char *group, const char *key);

	virtual bool IsConfigFileGroupKnown(const char *group);
};

} /* namespace retroradiocontroller */

#endif /* SRC_REMOTECONTROLLER_H_ */
