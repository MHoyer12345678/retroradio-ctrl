/*
 * connobserver.c
 *
 *  Created on: 04.08.2017
 *      Author: joe
 */

#include "ConnObserverFile.h"

#include <string.h>

#include "cpp-app-utils/Logger.h"

#define CONNECTED_SIGNAL_DIR "/run"
#define CONNECTED_SIGNAL_PATH "/run/wifi-connected"


using namespace GenericEmbeddedUtils;
using namespace CppAppUtils;

ConnObserverFile::ConnObserverFile(Listener *listener) :
		listener(listener),
		connected(false),
		fileMonitor(NULL)
{
}

ConnObserverFile::~ConnObserverFile()
{
	this->DeInit();
}

bool ConnObserverFile::Init()
{
	GError *err=NULL;
	GFile *file;

	Logger::LogDebug("ConnObserverFile::Init -> Starting inotify of /run to detect wifi-connected file appearing.");

	file=g_file_new_for_path(CONNECTED_SIGNAL_DIR);
	this->fileMonitor = g_file_monitor_directory(file, G_FILE_MONITOR_NONE, NULL,&err);
	if (err!=NULL)
	{
		Logger::LogError("Unable to observe file %s: %s",CONNECTED_SIGNAL_PATH,err->message);
		g_error_free(err);
		return false;
	}
	g_signal_connect(this->fileMonitor,"changed", G_CALLBACK(ConnObserverFile::OnChanges), this);
	g_object_unref(file);

	file=g_file_new_for_path(CONNECTED_SIGNAL_PATH);
	this->connected = g_file_query_exists(file, NULL);
	g_object_unref(file);

	Logger::LogDebug("ConnObserverFile::Init -> Already connected: %s", this->connected ? "true" : "false");

	return true;
}

void ConnObserverFile::DeInit()
{
	if (this->fileMonitor!=NULL)
	{
		g_file_monitor_cancel(this->fileMonitor);
		g_object_unref(this->fileMonitor);
		this->fileMonitor=NULL;
	}
}

bool ConnObserverFile::IsConnected()
{
	return this->connected;
}

void ConnObserverFile::OnChanges(GFileMonitor *monitor, GFile *file,
               GFile *other_file, GFileMonitorEvent event_type, gpointer user_data)
{
	char *path;
	ConnObserverFile *instance =(ConnObserverFile *)user_data;
	path=g_file_get_path(file);
	Logger::LogDebug("ConnObserverFile::OnChanges -> Signaled file event: %s",path);
	if (strcmp(path,CONNECTED_SIGNAL_PATH)==0)
	{
		Logger::LogDebug("ConnObserverFile::OnChanges -> Signaled file creation or remove of %s",
			CONNECTED_SIGNAL_PATH);
		if (event_type==G_FILE_MONITOR_EVENT_CREATED)
			instance->connected=true;
		else if (event_type==G_FILE_MONITOR_EVENT_DELETED)
			instance->connected=false;

		if (instance->listener!=NULL)
		{
			if (instance->connected)
				instance->listener->OnConnectionEstablished();
			else
				instance->listener->OnConnectionLost();
		}
	}

	 g_free(path);
}
