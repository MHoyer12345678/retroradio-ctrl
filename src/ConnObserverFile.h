/*
 * connobserver.h
 *
 *  Created on: 04.08.2017
 *      Author: joe
 */

#ifndef SRC_CONNOBSERVERFILE_H_
#define SRC_CONNOBSERVERFILE_H_

#include <glib.h>
#include <gio/gio.h>

namespace GenericEmbeddedUtils
{

class ConnObserverFile
{
public:
	class Listener
	{
	public:
		virtual void OnConnectionEstablished()=0;
		virtual void OnConnectionLost()=0;
	};

private:
	static void OnChanges(GFileMonitor *monitor, GFile *file,
               GFile *other_file, GFileMonitorEvent event_type, gpointer user_data);

	Listener *listener;

	GFileMonitor *fileMonitor;
	
	bool connected;

public:
	ConnObserverFile(Listener *listener);

	virtual ~ConnObserverFile();

	bool Init();

	void DeInit();

	bool IsConnected();

};

};

#endif /* SRC_CONNOBSERVERFILE_H_ */
