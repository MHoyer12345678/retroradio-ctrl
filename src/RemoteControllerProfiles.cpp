/*
 * RemoteController.cpp
 *
 *  Created on: 23.11.2019
 *      Author: joe
 */

#include "RemoteControllerProfiles.h"

#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <glib-unix.h>

#include <sys/ioctl.h>

#include <cpp-app-utils/Logger.h>
#include "RetroradioController.h"

#define SMALL_NEC_REMOTE_PROFILE_PROTOCOL_NAME				"nec"

#define NEC1_CODE_STANDBY		0x801E
#define NEC1_CODE_UP			0x801A
#define NEC1_CODE_DOWN			0x8005
#define NEC1_CODE_LEFT			0x8001
#define NEC1_CODE_RIGHT			0x8003
#define NEC1_CODE_ENTER			0x8002
#define NEC1_CODE_EXIT			0x8004
#define NEC1_CODE_SETUP			0x8012
#define NEC1_CODE_ZOOM			0x800A
#define NEC1_CODE_ROTATE		0x801B
#define NEC1_CODE_SLIDE_SHOW	0x801F

#define NEC2_CODE_STANDBY		0xC14485
#define NEC2_CODE_VOLUP			0xC14445
#define NEC2_CODE_VOLDOWN		0xC144C5
#define NEC2_CODE_MUTE			0xC14475
#define NEC2_CODE_MODE			0xC1444D
#define NEC2_CODE_SELECT		0xC14465
#define NEC2_CODE_NEXT			0xC14425
#define NEC2_CODE_PREV			0xC144A5
#define NEC2_CODE_PLAY			0xC1440D

#define TVSTICK_RC5_REMOTE_PROFILE_PROTOCOL_NAME				"rc-5"

#define RC51_CODE_STANDBY		0x0025
#define RC51_CODE_VOLUP			0x0010
#define RC51_CODE_VOLDOWN		0x0011
#define RC51_CODE_CHUP			0x0020
#define RC51_CODE_CHDOWN		0x0021
#define RC51_CODE_MUTE			0x000A
#define RC51_CODE_RIGHT			0x0019
#define RC51_CODE_LEFT			0x0018
#define RC51_CODE_UP			0x0014
#define RC51_CODE_DOWN			0x0015
#define RC51_CODE_ENTER			0x000D
#define RC51_CODE_0				0x0000
#define RC51_CODE_1				0x0001
#define RC51_CODE_2				0x0002
#define RC51_CODE_3				0x0003
#define RC51_CODE_4				0x0004
#define RC51_CODE_5				0x0005
#define RC51_CODE_6				0x0006
#define RC51_CODE_7				0x0007
#define RC51_CODE_8				0x0008
#define RC51_CODE_9				0x0009

using namespace CppAppUtils;

namespace retroradio_controller {

RemoteControllerProfiles::RemoteControllerProfiles() :
		protocolName(NULL)
{
}

RemoteControllerProfiles::~RemoteControllerProfiles()
{
}

bool RemoteControllerProfiles::Init(const char *profileName)
{
	Logger::LogDebug("RemoteControllerProfiles::Init -> Initializing remote controller profile %s.", profileName);

	if (strcmp(profileName, SMALL_NEC_REMOTE_PROFILE_ID)==0)
		this->CreateSmallNECRemoteProfile();
	else if (strcmp(profileName, TVSTICK_RC5_REMOTE_PROFILE_ID)==0)
		this->CreateTvstickRC5RemoteProfile();
	else
	{
		Logger::LogError("Unknown remote controller profile: %s", profileName);
		Logger::LogError("Known Profiles: %s, %s", SMALL_NEC_REMOTE_PROFILE_ID, TVSTICK_RC5_REMOTE_PROFILE_ID);
		return false;
	}

	Logger::LogDebug("RemoteControllerProfiles::Init -> Remote Protocol: %s", this->GetProtocolName());

	return true;
}


void RemoteControllerProfiles::DeInit()
{
	Logger::LogDebug("RemoteControllerProfiles::DeInit -> Deinitialized Remote controller profile.");
}

void RemoteControllerProfiles::CreateSmallNECRemoteProfile()
{
	this->protocolName=SMALL_NEC_REMOTE_PROFILE_PROTOCOL_NAME;


	this->scancode2commandMap[NEC1_CODE_STANDBY]=CMD_POWER;
	this->scancode2commandMap[NEC2_CODE_STANDBY]=CMD_POWER;
	this->scancode2commandMap[NEC1_CODE_UP]=CMD_VOL_UP;
	this->scancode2commandMap[NEC2_CODE_VOLUP]=CMD_VOL_UP;
	this->scancode2commandMap[NEC1_CODE_DOWN]=CMD_VOL_DOWN;
	this->scancode2commandMap[NEC2_CODE_VOLDOWN]=CMD_VOL_DOWN;
	this->scancode2commandMap[NEC1_CODE_LEFT]=CMD_PREV;
	this->scancode2commandMap[NEC2_CODE_PREV]=CMD_PREV;
	this->scancode2commandMap[NEC1_CODE_RIGHT]=CMD_NEXT;
	this->scancode2commandMap[NEC2_CODE_NEXT]=CMD_NEXT;
	this->scancode2commandMap[NEC1_CODE_ENTER]=CMD_MUTE;
	this->scancode2commandMap[NEC2_CODE_MUTE]=CMD_MUTE;
	this->scancode2commandMap[NEC1_CODE_ROTATE]=CMD_SRC_NEXT;
	this->scancode2commandMap[NEC2_CODE_MODE]=CMD_SRC_NEXT;
}

void RemoteControllerProfiles::CreateTvstickRC5RemoteProfile()
{
	this->protocolName=TVSTICK_RC5_REMOTE_PROFILE_PROTOCOL_NAME;

	this->scancode2commandMap[RC51_CODE_STANDBY]=CMD_POWER;
	this->scancode2commandMap[RC51_CODE_VOLUP]=CMD_VOL_UP;
	this->scancode2commandMap[RC51_CODE_VOLDOWN]=CMD_VOL_DOWN;
	this->scancode2commandMap[RC51_CODE_CHDOWN]=CMD_PREV;
	this->scancode2commandMap[RC51_CODE_CHUP]=CMD_NEXT;
	this->scancode2commandMap[RC51_CODE_MUTE]=CMD_MUTE;
	this->scancode2commandMap[RC51_CODE_RIGHT]=CMD_SRC_NEXT;
	this->scancode2commandMap[RC51_CODE_0]=CMD_FAV0;
	this->scancode2commandMap[RC51_CODE_1]=CMD_FAV1;
	this->scancode2commandMap[RC51_CODE_2]=CMD_FAV2;
	this->scancode2commandMap[RC51_CODE_3]=CMD_FAV3;
	this->scancode2commandMap[RC51_CODE_4]=CMD_FAV4;
	this->scancode2commandMap[RC51_CODE_5]=CMD_FAV5;
	this->scancode2commandMap[RC51_CODE_6]=CMD_FAV6;
	this->scancode2commandMap[RC51_CODE_7]=CMD_FAV7;
	this->scancode2commandMap[RC51_CODE_8]=CMD_FAV8;
	this->scancode2commandMap[RC51_CODE_9]=CMD_FAV9;
}

const char* RemoteControllerProfiles::GetProtocolName()
{
	return this->protocolName;
}

RemoteControllerProfiles::RemoteCommand RemoteControllerProfiles::GetCommandFromScanCode(
		unsigned long scancode)
{
	std::map<unsigned long, RemoteCommand>::iterator entry = this->scancode2commandMap.find(scancode);
	if (entry == this->scancode2commandMap.end())
		return __NO_CMD__;
	else
		return entry->second;
}

} /* namespace retroradiocontroller */
