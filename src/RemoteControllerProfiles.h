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
		__NO_CMD__		= 0x00,
		CMD_POWER		= 0x01,
		CMD_SRC_NEXT	= 0x02,
		CMD_VOL_UP		= 0x03,
		CMD_VOL_DOWN	= 0x04,
		CMD_MUTE		= 0x05,
		CMD_NEXT		= 0x06,
		CMD_PREV		= 0x07,
		CMD_FAV0		= 0x08,
		CMD_FAV1		= 0x09,
		CMD_FAV2		= 0x0A,
		CMD_FAV3		= 0x0B,
		CMD_FAV4		= 0x0C,
		CMD_FAV5		= 0x0D,
		CMD_FAV6		= 0x0E,
		CMD_FAV7		= 0x0F,
		CMD_FAV8		= 0x10,
		CMD_FAV9		= 0x11
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
