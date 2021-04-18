/*
 * SoundCardSetup.cpp
 *
 *  Created on: 16.02.2021
 *      Author: joe
 */

#include "SoundCardSetup.h"

#include <glib-unix.h>
#include "cpp-app-utils/Logger.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <stdio.h>

#define SND_DEVNODE_1 "/dev/snd/controlC0"
#define SND_DEVNODE_2 "/dev/snd/pcmC0D0c"
#define SND_DEVNODE_3 "/dev/snd/pcmC0D0p"

using namespace retroradio_controller;
using CppAppUtils::Logger;
/*
#error mixer öffnen bereits in init sound devices -> Hier mehr dynamic nötig
#error bedienungsanleitung / scripte für cross make in der pbuilder zelle
*/
SoundCardSetup::SoundCardSetup(ISoundCardSetupListener *aListener) :
		eventListener(aListener),
		udevMonitor(NULL),
		udevObj(NULL),
		udevEventId(0)
{
	this->devNodeLst=new DeviceReadyT[4];

	//all booleans set to false, all devNodes are set to NULL
	memset(this->devNodeLst,0,sizeof(DeviceReadyT[4]));
	//three devNodes set to actual values, last one kept NULL to mark the end of the list
	this->devNodeLst[0].devNode=SND_DEVNODE_1;
	this->devNodeLst[1].devNode=SND_DEVNODE_2;
	this->devNodeLst[2].devNode=SND_DEVNODE_3;
}

SoundCardSetup::~SoundCardSetup()
{
	delete this->devNodeLst;
}

bool SoundCardSetup::Init()
{
	this->DeInit();
	Logger::LogDebug("SoundCardSetup::Init - Initializing sound card setup controller.");
	if (!this->SetupUdevListener())
		return false;

	this->TriggerColdPluggedDevices();

	return true;
}

void SoundCardSetup::DeInit()
{
	if (this->udevMonitor!=NULL)
	{
		udev_monitor_unref(this->udevMonitor);
		this->udevMonitor=NULL;
	}

	if (this->udevObj!=NULL)
	{
		udev_unref(this->udevObj);
		this->udevObj=NULL;
	}
}

bool SoundCardSetup::SetupUdevListener()
{
	int monitorFd;

	Logger::LogDebug("SoundCardSetup::SetupUdevListener - Creating udev monitor.");

	this->udevObj = udev_new();
	if(this->udevObj==NULL)
	{
		Logger::LogError("Unable to create new udev object.");
		return false;
	}

	this->udevMonitor = udev_monitor_new_from_netlink(this->udevObj, "udev");
	udev_monitor_enable_receiving(this->udevMonitor);

	monitorFd=udev_monitor_get_fd(this->udevMonitor);
	this->udevEventId=g_unix_fd_add(monitorFd, G_IO_IN,SoundCardSetup::OnUdevEvent,this);

	return true;
}

void SoundCardSetup::TriggerColdPluggedDevices()
{
	DeviceReadyT *item=this->devNodeLst;
	Logger::LogDebug("SoundCardSetup::ColdPlugDevices - Setting up sound devices if already available.");
	while(item->devNode!=NULL)
	{
		this->TriggerColdPluggedDevice(item->devNode);
		item++;
	}
}

gboolean SoundCardSetup::OnUdevEvent(gint fd, GIOCondition condition, gpointer userData)
{
	SoundCardSetup *instance=(SoundCardSetup *)userData;
	udev_device* dev = udev_monitor_receive_device(instance->udevMonitor);

	if (dev == NULL) return TRUE;

	instance->OnUdevEvent(dev);

	return TRUE;
}

void SoundCardSetup::OnUdevEvent(udev_device *dev)
{
	const char *devNode;
	const char *action;
	DeviceReadyT *deviceReadyItem;
	bool allDevicesSetupOld, allDevicesSetupNew;

	devNode=udev_device_get_devnode(dev);
	action=udev_device_get_action(dev);

	if (devNode==NULL || action==NULL) return;

	deviceReadyItem=FindDeviceItemByDevNode(devNode);
	if (deviceReadyItem==NULL) return;

	allDevicesSetupOld=this->IsCardAvailable();

	Logger::LogDebug("SoundCardSetup::OnUdevEvent - Received uevent for dev node: %s, Action: %s", devNode, action);

	//"change" events are treated same way as "add" events
	deviceReadyItem->available=!(strcmp(action,"remove")==0);
	allDevicesSetupNew=this->IsCardAvailable();

	if (allDevicesSetupOld!=allDevicesSetupNew && this->eventListener!=NULL)
	{
		if (allDevicesSetupNew)
			this->eventListener->OnSoundCardReady();
		else
			this->eventListener->OnSoundCardDisabled();
	}
}

void SoundCardSetup::TriggerColdPluggedDevice(const char *devNode)
{
	struct stat statResult;
	udev_device *udevDevice;
	char deviceType;
	char ueventFilepath[PATH_MAX];
	const char *syspath;
	int fd;

	Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Triggering cold plugged device: %s", devNode);

	// if dev node not yet there -> ok, will appear later
	if (stat(devNode, &statResult)!=0)
	{
		Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Device %s not present yet (stat failed). Waiting for it.", devNode);
		return;
	}

	deviceType = S_ISCHR(statResult.st_mode) ? 'c' : 'b';

	udevDevice=udev_device_new_from_devnum(this->udevObj,deviceType,statResult.st_rdev);
	if (udevDevice==NULL)
	{
		Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Device %s not present yet (no udev device). Waiting for it.", devNode);
		Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Device %s, Type: %c, Major: %d, Minor: %d",
				devNode, deviceType, major(statResult.st_rdev), minor(statResult.st_rdev));
		return;
	}

	syspath=udev_device_get_devpath(udevDevice);
	snprintf(ueventFilepath, PATH_MAX, "/sys%s/uevent", syspath);

	fd=open(ueventFilepath, O_RDWR);
	if (fd != -1)
	{
		if (write(fd, "add", 3)==3)
			Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Wrote \"add\" to uevent file %s successfully.", ueventFilepath);
		else
			Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Error writing add to uevent file: %s.", ueventFilepath);
		close(fd);
	}
	else
	{
		Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Device %s not present yet (open uevent file failed). Waiting for it.", devNode);
		Logger::LogDebug("SoundCardSetup::TriggerColdPluggedDevice - Path: %s",ueventFilepath);
	}

	udev_device_unref(udevDevice);
}

SoundCardSetup::DeviceReadyT* SoundCardSetup::FindDeviceItemByDevNode(
		const char *devNode)
{
	DeviceReadyT *item=this->devNodeLst;
	while(item->devNode!=NULL)
	{
		if (strcmp(item->devNode, devNode)==0) return item;
		item++;
	}

	return NULL;
}

bool SoundCardSetup::IsCardAvailable()
{
	DeviceReadyT *item=this->devNodeLst;

	while(item->devNode!=NULL)
	{
		if (!item->available) return false;
		item++;
	}

	return true;
}

