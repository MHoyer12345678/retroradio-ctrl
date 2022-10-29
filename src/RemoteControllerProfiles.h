/*
 * RemoteControllerProfiles.h
 *
 *  Created on: 24.10.2022
 *      Author: joe
 */

#ifndef SRC_REMOTECONTROLLERPROFILES_H_
#define SRC_REMOTECONTROLLERPROFILES_H_

#include <glib.h>
#include <linux/lirc.h>
#include <map>

using namespace CppAppUtils;

#define _SCAN_CODE_NOT_SET	0xFFFF

#define SMALL_NEC_REMOTE_PROFILE_ID							"SmallNECRemoteProfile"
#define TVSTICK_RC5_REMOTE_PROFILE_ID						"TVStickRC5RemoteProfile"

namespace retroradio_controller {

class RemoteControllerProfiles {

public:
	enum RemoteCommand
	{
		__NO_CMD__,
		CMD_POWER,
		CMD_SRC_NEXT,
		CMD_VOL_UP,
		CMD_VOL_DOWN,
		CMD_MUTE,
		CMD_NEXT,
		CMD_PREV
	};

private:
	std::map<unsigned long, RemoteCommand> scancode2commandMap;

	const char *protocolName;

	void CreateSmallNECRemoteProfile();

	void CreateTvstickRC5RemoteProfile();

public:
	RemoteControllerProfiles();

	virtual ~RemoteControllerProfiles();

	bool Init(const char *profileName);

	const char *GetProtocolName();

	RemoteCommand GetCommandFromScanCode(unsigned long scancode);

	void DeInit();
};

} /* namespace retroradiocontroller */

#endif /* SRC_REMOTECONTROLLERPROFILES_H_ */
