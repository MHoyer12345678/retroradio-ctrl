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

#define SOFT_REPEAT_DETECTOR_TIMEOUT_MS	250

#define DEFERED_INIT_RETRY_CNT_MAX		10
#define DEFERED_INIT_RETRY_INTERVAL_MS	100

using namespace CppAppUtils;

#define RC_CONFIG_GROUP					"RemoteControl"
#define DEFAULT_RC_INPUTDEV_NAME		"/dev/lirc0"
#define CONFIG_TAG_RC_INPUT_DEV_NAME	"InputDevice"
#define DEFAULT_RC_PROFILE_NAME			SMALL_NEC_REMOTE_PROFILE_ID
#define CONFIG_TAG_RC_PROFILE_NAME		"RemoteProfile"
#define RC_PROTOCOL_PATH_TEMPLATE		"/sys/dev/char/%d:%d/device/protocols"

namespace retroradio_controller {

RemoteController::RemoteController(IRemoteControllerListener* theListener,
		Configuration *configuration) :
		listener (theListener),
		pollFd(-1),
		lircEventSrc(0),
		inputDeviceName(NULL),
		remoteProfileName(NULL),
		retryCntr(0),
		defereTimerId(0),
		softwarRepeatDetectorDelayEnabled(false),
		softwareRepeatDetectorTimerId(0),
		lastScanCode(_SCAN_CODE_NOT_SET)
{
	this->remoteControllerProfiles=new RemoteControllerProfiles();
	configuration->AddConfigurationModule(this);
}

RemoteController::~RemoteController()
{
	if (this->remoteProfileName!=NULL)
		free(this->remoteProfileName);
	if (this->inputDeviceName!=NULL)
		free(this->inputDeviceName);

	delete this->remoteControllerProfiles;
}

bool RemoteController::Init()
{
	int result;
	Logger::LogDebug("RemoteController::Init -> Initializing Remote Controller.");
	this->retryCntr=0;
	this->softwarRepeatDetectorDelayEnabled=false;
	this->softwareRepeatDetectorTimerId=0;

	if (!this->remoteControllerProfiles->Init(this->GetRemoteProfileName()))
		return false;

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

	this->remoteControllerProfiles->DeInit();
	Logger::LogDebug("RemoteController::DeInit -> Deinitialized Remote controller");
}

int RemoteController::InitializeLIRC()
{
	int result;
	const char *lircDevice = this->GetLircDeviceName();

	Logger::LogDebug("RemoteController::InitializeLIRC -> Open LIRC device: %s.", lircDevice);

	Logger::LogDebug("RemoteController::InitializeLIRC -> Ping: %d", this->pollFd);

	if (this->pollFd!=-1)
		close(this->pollFd);

	Logger::LogDebug("RemoteController::InitializeLIRC -> Ping ...");

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

	const char *protoName=this->remoteControllerProfiles->GetProtocolName();

	Logger::LogDebug("RemoteController::SetIRDeviceProtocol -> Setting IR device protocol to %s.", protoName);
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

	len=strlen(protoName);
	if (write(fd, protoName, len)!=len)
	{
		Logger::LogError("Error setting IR protocol to %s.", protoName);
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

bool RemoteController::FilterRepeatedCmds(RemoteControllerProfiles::RemoteCommand cmd, bool repeated,
		bool toggled)
{
	// all repeated events but the ones for vol up and down are suppressed
	return repeated && cmd != RemoteControllerProfiles::CMD_VOL_UP && cmd != RemoteControllerProfiles::CMD_VOL_DOWN;
}

void RemoteController::ProcessScanCode(unsigned long scancode, bool repeated,
		bool toggled)
{
	RemoteControllerProfiles::RemoteCommand cmd;
	cmd=this->remoteControllerProfiles->GetCommandFromScanCode(scancode);

	if (cmd == RemoteControllerProfiles::__NO_CMD__)
		return;

	if (this->FilterRepeatedCmds(cmd, repeated, toggled))
		return;

	if (this->listener != NULL)
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
	else if (strcasecmp(key, CONFIG_TAG_RC_PROFILE_NAME)==0)
	{
		char *name;
		if (Configuration::GetStringValueFromKey(confFile,key,group, &name))
		{
			if (this->remoteProfileName!=NULL)
				free(this->remoteProfileName);
			this->remoteProfileName=name;
		}
		else
			return false;
	}

	return true;
}

const char* RemoteController::GetRemoteProfileName()
{
	if (this->remoteProfileName!=NULL)
		return this->remoteProfileName;
	else
		return DEFAULT_RC_PROFILE_NAME;
}

bool RemoteController::IsConfigFileGroupKnown(const char* group)
{
	return strcasecmp(group, RC_CONFIG_GROUP);
}

} /* namespace retroradiocontroller */
