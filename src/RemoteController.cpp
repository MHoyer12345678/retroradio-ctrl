/*
 * RemoteController.cpp
 *
 *  Created on: 23.11.2019
 *      Author: joe
 */

#include "RemoteController.h"

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

#define _SCAN_CODE_NOT_SET	0x0

// remote one
#define CODE_STANDBY		0x801E
#define CODE_UP				0x801A
#define CODE_DOWN			0x8005
#define CODE_LEFT			0x8001
#define CODE_RIGHT			0x8003
#define CODE_ENTER			0x8002
#define CODE_EXIT			0x8004
#define CODE_SETUP			0x8012
#define CODE_ZOOM			0x800A
#define CODE_ROTATE			0x801B
#define CODE_SLIDE_SHOW		0x801F

//remote two
#define CODE2_STANDBY		0xC14485
#define CODE2_VOLUP			0xC14445
#define CODE2_VOLDOWN		0xC144C5
#define CODE2_MUTE			0xC14475
#define CODE2_MODE			0xC1444D
#define CODE2_SELECT		0xC14465
#define CODE2_NEXT			0xC14425
#define CODE2_PREV			0xC144A5
#define CODE2_PLAY			0xC1440D


#define SOFT_REPEAT_DETECTOR_TIMEOUT_MS	250

#define DEFERED_INIT_RETRY_CNT_MAX		10
#define DEFERED_INIT_RETRY_INTERVAL_MS	100

using namespace CppAppUtils;

#define RC_CONFIG_GROUP					"RemoteControl"
#define DEFAULT_RC_INPUTDEV_NAME		"/dev/lirc0"
#define CONFIG_TAG_RC_INPUT_DEV_NAME	"InputDevice"
#define RC_PROTOCOL_PATH_TEMPLATE		"/sys/dev/char/%d:%d/device/protocols"
#define RC_PROTOCOL_NAME				"nec"

namespace retroradio_controller {

RemoteController::RemoteController(IRemoteControllerListener* theListener,
		Configuration *configuration) :
		listener (theListener),
		pollFd(-1),
		lircEventSrc(0),
		inputDeviceName(NULL),
		retryCntr(0),
		defereTimerId(0),
		softwarRepeatDetectorDelayEnabled(false),
		softwareRepeatDetectorTimerId(0),
		lastScanCode(_SCAN_CODE_NOT_SET)
{
	configuration->AddConfigurationModule(this);
}

RemoteController::~RemoteController()
{
	if (this->inputDeviceName!=NULL)
		free(this->inputDeviceName);
}

bool RemoteController::Init()
{
	int result;
	Logger::LogDebug("RemoteController::Init -> Initializing Remote Controller.");
	this->retryCntr=0;
	this->softwarRepeatDetectorDelayEnabled=false;
	this->softwareRepeatDetectorTimerId=0;

	result=InitializeLIRC();

	if (result==EAGAIN)
	{
		this->DefereInitalization();
		result=0;
	}

	return result==0;
}

void RemoteController::DefereInitalization()
{
	this->defereTimerId=g_timeout_add(DEFERED_INIT_RETRY_INTERVAL_MS, RemoteController::RetryInit, this);
}

gboolean RemoteController::RetryInit(gpointer data)
{
	int initResult;

	RemoteController *instance=(RemoteController *)data;

	initResult=instance->InitializeLIRC();
	instance->retryCntr++;

	if (initResult != EAGAIN || instance->retryCntr >= DEFERED_INIT_RETRY_CNT_MAX)
	{
		if (initResult==0)
			Logger::LogDebug("RemoteController::RetryInit - Initialization succeeded after %d tries.", instance->retryCntr);
		if (initResult==EAGAIN)
			Logger::LogError("Did not succeed initializing the remote controller after %d tries. Giving up.", instance->retryCntr);
		else
			Logger::LogError("Failed to initialize the remote controller. Result: %d", initResult);

		instance->defereTimerId=0;
		return FALSE;
	}

	return TRUE;
}

void RemoteController::DeInit()
{
	if (this->softwareRepeatDetectorTimerId!=0)
	{
		g_source_remove(this->softwareRepeatDetectorTimerId);
				this->softwareRepeatDetectorTimerId=0;
		this->softwarRepeatDetectorDelayEnabled=false;
		this->lastScanCode=_SCAN_CODE_NOT_SET;
	}

	if (this->defereTimerId!=0)
	{
		g_source_remove(this->defereTimerId);
		this->defereTimerId=0;
	}

	if (this->lircEventSrc!=0)
	{
		g_source_remove(this->lircEventSrc);
		this->lircEventSrc=0;
	}

	if (this->pollFd!= -1)
	{
		close(this->pollFd);
		this->pollFd=-1;
	}
	Logger::LogDebug("RemoteController::DeInit -> Deinitialized Remote controller");
}

int RemoteController::InitializeLIRC()
{
	int result;
	const char *lircDevice = this->GetLircDeviceName();

	Logger::LogDebug("RemoteController::InitializeLIRC -> Open LIRC device: %s.", lircDevice);

	if (this->pollFd!=-1)
		close(this->pollFd);

	result=this->SetIRDeviceProtocol(lircDevice);

	if (result==0)
		result=this->EnableIRDeviceReceiveMode(lircDevice);


	return result;
}

int RemoteController::SetIRDeviceProtocol(const char *lircDevice)
{
	int fd, len;
	int result=0;
	int majorId,minorId;
	struct stat statResult;
	char fn[2048];

	Logger::LogDebug("RemoteController::SetIRDeviceProtocol -> Setting IR device protocol to NEC.");
	if (stat(lircDevice, &statResult)!=0)
	{
		if (errno == ENOENT)
			return EAGAIN;
		else
		{
			Logger::LogError("Error looking up IR device %s: %s", lircDevice, strerror(errno));
			return errno;
		}
	}

	majorId=major(statResult.st_rdev);
	minorId=minor(statResult.st_rdev);

	if (minorId < 0 || majorId < 0)
	{
		Logger::LogError("Given path %s is not a device node.", lircDevice);
		return EINVAL;
	}

	snprintf(fn,2040,RC_PROTOCOL_PATH_TEMPLATE, majorId,minorId);
	Logger::LogDebug("RemoteController::SetIRDeviceProtocol -> Opening IR protocols file: %s", fn);

	fd=open(fn, O_WRONLY|O_NONBLOCK);
	if (fd==-1)
	{
		Logger::LogError("Error opening protocol file of IR device %s: %s", fn, strerror(errno));
		return errno;
	}

	len=strlen(RC_PROTOCOL_NAME);
	if (write(fd, RC_PROTOCOL_NAME, len)!=len)
	{
		Logger::LogError("Error setting IR protocol to %s.", RC_PROTOCOL_NAME);
		result=EINVAL;
	}

	close(fd);

	return result;
}

int RemoteController::EnableIRDeviceReceiveMode(const char *lircDevice)
{
	unsigned mode = LIRC_MODE_SCANCODE;

	Logger::LogDebug("RemoteController::EnableIRDeviceReceiveMode -> Setting IR device into receiver mode.");
	this->pollFd=open(lircDevice, O_RDONLY | O_NONBLOCK);
	if (this->pollFd==-1)
	{
		Logger::LogInfo("Failed to open LIRC device: %s. Retrying again.", lircDevice);
		return EAGAIN;
	}

	if (ioctl(this->pollFd, LIRC_SET_REC_MODE, &mode))
	{
		Logger::LogError("Failed to set lirc kernel module into SCAN mode.");
		close(this->pollFd);
		this->pollFd=-1;
		return EFAULT;
	}

    this->lircEventSrc=g_unix_fd_add(this->pollFd,G_IO_IN,RemoteController::OnLircEvent, this);

	return 0;
}

const char* RemoteController::GetLircDeviceName()
{
	if (this->inputDeviceName!=NULL)
		return this->inputDeviceName;
	else
		return DEFAULT_RC_INPUTDEV_NAME;
}

gboolean RemoteController::OnLircEvent(gint fd, GIOCondition condition,
		gpointer user_data)
{
	RemoteController *instance=(RemoteController *)user_data;
	instance->ReadLircEvent();
	return TRUE;
}

void RemoteController::ReadLircEvent()
{
	lirc_scancode_t sc[64];
	int bytesRd;

	bytesRd = read(this->pollFd, sc, sizeof(sc));

	if (bytesRd != -1)
		ParseScanCodes(sc, (unsigned int)(bytesRd / sizeof(lirc_scancode_t)));
	else if (errno != EAGAIN)
		Logger::LogError("Failed to read scan code from lirc module.");
}

void RemoteController::ParseScanCodes(lirc_scancode_t* scanCodes,
		unsigned int numberOfCodes)
{
	unsigned i;

	for (i=0; i< numberOfCodes; i++)
	{
		bool repeated=(scanCodes[i].flags & LIRC_SCANCODE_FLAG_REPEAT)!=0;
		bool toggled=(scanCodes[i].flags & LIRC_SCANCODE_FLAG_TOGGLE)!=0;

		Logger::LogDebug("RemoteController::ParseScanCodes -> Received IR code: 0x%llX, %s , %s",
				scanCodes[i].scancode, repeated ? "repeated" : "", toggled ? "toggled" : "");

		// software repeat filter.
		if (this->CheckSoftwareRepeatDetector(scanCodes[i].scancode, repeated))
			repeated=true;

		if (this->FilterOutScanCode(scanCodes[i].scancode, repeated, toggled))
			break;

		this->ProcessScanCode(scanCodes[i].scancode, repeated, toggled);
	}
}

bool RemoteController::CheckSoftwareRepeatDetector(unsigned long scancode,
		bool repeated)
{
	//events already marked as repeated can be ignored
	//remark:
	//	a potentially enabled timer is not disabled here for performance reasons.
	//	It is disabled:
	//     - either when it elapsed by returning FALSE in the timeout function
	//     - or when it is armed again (in case of a new scan code received)
	if (repeated)
	{
		this->lastScanCode=scancode;
		this->softwarRepeatDetectorDelayEnabled=false;
		return false;
	}

	//got the same scancode within delay interval but not marked as repeated -> mark it as repeated
	if (this->lastScanCode == scancode && this->softwarRepeatDetectorDelayEnabled)
		return true;

	// now we got a new scan code or the same after significant time -> arm timeout
	this->lastScanCode=scancode;
	this->softwarRepeatDetectorDelayEnabled=true;

	//remove old timeout if active (can happen when different scancodes arrive fast one after the other)
	if (this->softwareRepeatDetectorTimerId!=0)
		g_source_remove(this->softwareRepeatDetectorTimerId);

	this->softwareRepeatDetectorTimerId=g_timeout_add(SOFT_REPEAT_DETECTOR_TIMEOUT_MS,
			RemoteController::SoftwareRepeatDetectorTimeoutFunc, this);

	return false;
}

gboolean RemoteController::SoftwareRepeatDetectorTimeoutFunc(gpointer data)
{
	RemoteController *instance=(RemoteController *)data;
	instance->softwarRepeatDetectorDelayEnabled=false;

	// timer itsself is disabled by returning FALSE here.
	// Just need to clean up the timer id here.
	instance->softwareRepeatDetectorTimerId=0;

	return FALSE;
}

bool RemoteController::FilterOutScanCode(unsigned long scancode, bool repeated,
		bool toggled)
{
	// all repeated events but the ones for up and down (vol up and vol down) are suppressed
	return repeated && scancode != CODE_UP && scancode != CODE2_VOLUP
			&& scancode != CODE_DOWN && scancode != CODE2_VOLDOWN;
}

void RemoteController::ProcessScanCode(unsigned long scancode, bool repeated,
		bool toggled)
{
	bool cmdFound=true;
	RemoteCommand cmd;

	switch(scancode)
	{
	case CODE_STANDBY:
	case CODE2_STANDBY:
		cmd=CMD_POWER;
		break;
	case CODE_ROTATE:
	case CODE2_MODE:
		cmd=CMD_SRC_NEXT;
		break;
	case CODE_UP:
	case CODE2_VOLUP:
		cmd=CMD_VOL_UP;
		break;
	case CODE_DOWN:
	case CODE2_VOLDOWN:
		cmd=CMD_VOL_DOWN;
		break;
	case CODE_ENTER:
	case CODE2_MUTE:
		cmd=CMD_MUTE;
		break;
	case CODE_RIGHT:
	case CODE2_NEXT:
		cmd=CMD_NEXT;
		break;
	case CODE_LEFT:
	case CODE2_PREV:
		cmd=CMD_PREV;
		break;
	default:
		cmdFound=false;
	}

	if (cmdFound && this->listener != NULL)
		this->listener->OnCommandReceived(cmd);
}

bool RemoteController::ParseConfigFileItem(GKeyFile* confFile, const char* group, const char* key)
{
	if (strcasecmp(group, RC_CONFIG_GROUP)!=0) return true;
	if (strcasecmp(key, CONFIG_TAG_RC_INPUT_DEV_NAME)==0)
	{
		char *dev;
		if (Configuration::GetStringValueFromKey(confFile,key,group, &dev))
		{
			if (this->inputDeviceName!=NULL)
				free(this->inputDeviceName);
			this->inputDeviceName=dev;
		}
		else
			return false;
	}

	return true;
}

bool RemoteController::IsConfigFileGroupKnown(const char* group)
{
	return strcasecmp(group, RC_CONFIG_GROUP);
}

} /* namespace retroradiocontroller */
