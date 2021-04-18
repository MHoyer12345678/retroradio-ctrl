/*
 * MPDAudioSource.h
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_MPDAUDIOSOURCE_H_
#define SRC_AUDIOSOURCES_MPDAUDIOSOURCE_H_

#include <mpd/connection.h>

#include <glib.h>

#include "TrackChangeTransition.h"

#include "AbstractAudioSource.h"

namespace retroradio_controller
{

class MPDAudioSource: public AbstractAudioSource
{
private:
	char *mpdHost;

	unsigned int mpdPort;

	char *mpdStationPlayList;

	TrackChangeTransition trackChangeTransition;

	struct mpd_connection *mpdCon;

	unsigned int trackNr;

	guint pollSourceId;

	guint mpdWatchdogTimerId;

	bool ConnectToMPD();

	void DisconnectFromMPD();

	const char *ConfigGetMPDHost();

	unsigned int ConfigGetMPDPort();

	void ArmMPDAliveWatchdog();

	void DisarmMPDAliveWatchdog();

	void CheckMPDAliveAndReadTrackPos();

	void StartPollingMPD();

	void StopPollingMPD();

	static gboolean RetryConnect(gpointer data);

	static gboolean MPDConnectionWatchdog(gpointer data);

	void LoadPlayList();

	virtual void OnRampFinished(bool canceled);

	void KickOffChangeTrackTransition();

	void ProcessPendingTrackChangeCommands();

	void FinalizeChangeTrackTransition();

protected:
	virtual const char *GetConfigGroupName();

	virtual const char *GetDefaultAlsaMixerName();

	virtual const char *GetDefaultSoundCardName();

	const char *ConfigGetRadioStationPlaylistName();

	unsigned int PersGetTrackNumber();

	virtual bool IsMuteDownRampAllowed();

	virtual bool IsMuteUpRampAllowed();

	virtual void SourceActivationFinished();

public:
	MPDAudioSource(const char *srcName, AbstractAudioSource *predecessor,
			IAudioSourceStateListener *srcListener);

	virtual ~MPDAudioSource();

	virtual bool Init();

	virtual void DeInit();

	virtual bool IsStartupFinished();

	virtual void DoActivateSource(bool need2ReOpenSoundDevices);

	virtual void DoDeActivateSource();

	virtual void DoStartPlaying();

	virtual void DoStopPlaying();

	virtual void Next();

	virtual void Previous();

	virtual bool ParseConfigFileItem(GKeyFile *confFile, const char *group, const char *key);
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_MPDAUDIOSOURCE_H_ */
