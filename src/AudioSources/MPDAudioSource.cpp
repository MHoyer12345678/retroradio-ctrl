/*
 * MPDAudioSource.cpp
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#include "MPDAudioSource.h"

#include <cpp-app-utils/Logger.h>
#include "RetroradioController.h"

#include <glib-unix.h>
#include <mpd/status.h>
#include <mpd/playlist.h>
#include <mpd/player.h>
#include <mpd/queue.h>

using namespace CppAppUtils;

using namespace retroradio_controller;

#define MPD_CONNECT_TIMEOUT_MS 			1000
#define MPD_CONNECT_RETRY_INTERVAL_MS	1000
#define MPD_ALIVE_WATCHDOG_TIMEOUT_MS	5000

#define MPC_CONFIG_GROUP 				"MPD Source"
#define MPC_DEFAULT_ALSA_MIXER_NAME		"mpc_vol"
#define MPC_DEFAULT_SOUND_CARD_NAME		"default"

#define MPD_DEFAULT_HOST						"127.0.0.1"
#define MPD_CONFIG_TAG_HOST						"MpdHost"
#define MPD_DEFAULT_PORT						6600
#define MPD_CONFIG_TAG_PORT						"MpdPort"
#define MPD_DEFAULT_RADIO_STATION_PLAYLIST		"radio"
#define MPD_CONFIG_TAG_PLAYLIST					"RadioStationPlaylist"

//TODO: adapt to be a bit more robust when connection is lost

MPDAudioSource::MPDAudioSource(const char *srcName, AbstractAudioSource *predecessor,
		IAudioSourceListener *srcListener) :
		AbstractAudioSource(srcName, predecessor, srcListener),
		mpdCon(NULL),
		pollSourceId(0),
		mpdWatchdogTimerId(0),
		trackNr(0),
		queueLength(0),
		mpdHost(NULL),
		mpdPort(MPD_DEFAULT_PORT),
		mpdStationPlayList(NULL)
{

}

MPDAudioSource::~MPDAudioSource()
{
	if (this->mpdHost!=NULL)
		free(this->mpdHost);
	if (this->mpdStationPlayList!=NULL)
		free(this->mpdStationPlayList);
}

bool MPDAudioSource::Init()
{
	if (!AbstractAudioSource::Init())
		return false;

	this->trackChangeTransition.Reset();
	this->trackNr=RetroradioController::Instance()->GetPersistentState()->GetMPDCurrentTrackNr();

	Logger::LogDebug("MPDAudioSource::Init - Initializing MPD Audio Source %s.", this->GetName());

	return true;
}

void MPDAudioSource::DeInit()
{
	this->DisconnectFromMPD();
	Logger::LogDebug("MPDAudioSource::DeInit - Uninitiated MPD Audio Source %s.", this->GetName());
}

bool MPDAudioSource::IsStartupFinished()
{
	bool result=true;

	struct mpd_connection *con;
	Logger::LogDebug("MPDAudioSource::IsStartupFinished - Checking if mpd server is available.");

	con=mpd_connection_new(this->ConfigGetMPDHost(), this->ConfigGetMPDPort(), MPD_CONNECT_TIMEOUT_MS);
	if (mpd_connection_get_error(con)!=MPD_ERROR_SUCCESS)
	{
		Logger::LogDebug("MPDAudioSource::ConnectToMPD - MPD daemon not yet available");
		result=false;
	}
	else
	{
		struct mpd_status *statusResult;
		statusResult=mpd_run_status(con);
		if (statusResult==NULL)
		{
			Logger::LogDebug("MPDAudioSource::ConnectToMPD - MPD daemon does not answer properly a status request.");
			result=false;
		}
		else
		{
			mpd_status_free(statusResult);
		}
	}

	mpd_connection_free(con);
	return result;
}


void MPDAudioSource::DoActivateSource(bool need2ReOpenSoundDevices)
{
	Logger::LogDebug("MPDAudioSource::DoActivateSource - About to activate source.");
	if (this->ConnectToMPD())
		this->SourceActivationFinished();
	else
		this->StartPollingMPD();
}

bool MPDAudioSource::ConnectToMPD()
{
	struct mpd_connection *con;

	Logger::LogDebug("MPDAudioSource::ConnectToMPD - Connecting to mpd daemon.");
	con=mpd_connection_new(this->ConfigGetMPDHost(), this->ConfigGetMPDPort(), MPD_CONNECT_TIMEOUT_MS);
	if (mpd_connection_get_error(con)!=MPD_ERROR_SUCCESS)
	{
		Logger::LogDebug("MPDAudioSource::ConnectToMPD - Unable to connect to mpd daemon. Retrying ...");
		Logger::LogDebug("MPDAudioSource::ConnectToMPD - Error: %s",  mpd_connection_get_error_message(con));
		mpd_connection_free(con);
		return false;
	}

	mpd_connection_set_keepalive(con,true);
	this->mpdCon=con;
	Logger::LogDebug("MPDAudioSource::ConnectToMPD - Connected to mpd daemon.");

	this->ArmMPDAliveWatchdog();

	return true;
}

void MPDAudioSource::StartPollingMPD()
{
	//pollSourceId != 0 -> already polling
	if (this->pollSourceId!=0)
		return;

	this->pollSourceId=g_timeout_add(MPD_CONNECT_RETRY_INTERVAL_MS, MPDAudioSource::RetryConnect, this);
}

void MPDAudioSource::StopPollingMPD()
{
	if (this->pollSourceId==0)
		return;
	Logger::LogDebug("MPDAudioSource::StopPollingMPD - Stop polling for connecting to the MPD daemon.");

	g_source_remove(this->pollSourceId);
	this->pollSourceId=0;
}

void MPDAudioSource::DoDeActivateSource()
{
	Logger::LogDebug("MPDAudioSource::DoDeActivateSource - About to deactivate source.");
	if (this->mpdCon==NULL)
		this->StopPollingMPD();
	else
		this->DisconnectFromMPD();
	this->SourceDeActivationFinished();
}

gboolean MPDAudioSource::RetryConnect(gpointer data)
{
	MPDAudioSource *instance=(MPDAudioSource *)data;

	//state change in the meanwhile -> stop timer
	if (instance->GetState()==DEACTIVATED || instance->GetState()==DEACTIVATING)
	{
		instance->StopPollingMPD();
		return FALSE;
	}

	if (instance->ConnectToMPD())
	{
		if (instance->GetState()==ACTIVATING)
			instance->SourceActivationFinished();
		instance->StopPollingMPD();
		return FALSE;
	}

	return TRUE;
}

void MPDAudioSource::DisconnectFromMPD()
{
	if (this->mpdCon==NULL)
		return;
	Logger::LogDebug("MPDAudioSource::DisconnectFromMPD - Disconnecting from MPD daemon.");

	this->DisarmMPDAliveWatchdog();
	mpd_connection_free(this->mpdCon);
	this->mpdCon=NULL;
}

void MPDAudioSource::ArmMPDAliveWatchdog()
{
	Logger::LogDebug("MPDAudioSource::ArmMPDAliveWatchdog - Starting mpd connection watchdog.");
	//mpdWatchdogTimerId != 0 -> already polling
	if (this->mpdWatchdogTimerId!=0)
		return;

	this->mpdWatchdogTimerId=g_timeout_add(MPD_ALIVE_WATCHDOG_TIMEOUT_MS, MPDAudioSource::MPDConnectionWatchdog, this);
}

void MPDAudioSource::DisarmMPDAliveWatchdog()
{
	if (this->mpdWatchdogTimerId==0) return;
	Logger::LogDebug("MPDAudioSource::DisarmMPDAliveWatchdog - Stopping mpd connection watchdog.");

	g_source_remove(this->mpdWatchdogTimerId);
	this->mpdWatchdogTimerId=0;
}

void MPDAudioSource::CheckMPDAliveAndReadTrackPos()
{
	struct mpd_status *statusResult;

	Logger::LogDebug("MPDAudioSource::CheckMPDAlive - Checking connection to mpd daemon.");

	if (this->mpdCon==NULL)
	{
		Logger::LogDebug("MPDAudioSource::CheckMPDAlive - Seems we have a state machine error, mpd connection object already null. Disarming watchdog.");
		this->DisarmMPDAliveWatchdog();
		return;
	}

	statusResult=mpd_run_status(this->mpdCon);
	if (statusResult!=NULL)
	{
		int lTrackNr=mpd_status_get_song_pos(statusResult);
		Logger::LogDebug("MPDAudioSource::CheckMPDAlive - MPD answers. Everything ok.");
		if (lTrackNr!=-1)
		{
			if (this->trackNr!=lTrackNr)
			{
				this->trackNr=lTrackNr;
				RetroradioController::Instance()->GetPersistentState()->SetMPDCurrentTrackNr(this->trackNr);
			}
			Logger::LogDebug("MPDAudioSource::CheckMPDAlive - Currently playing track: %d", this->trackNr);
		}

		this->queueLength=mpd_status_get_queue_length(statusResult);

		mpd_status_free(statusResult);
		return;
	}

	//no answer -> disconnect & try reconnect again
	Logger::LogDebug("MPDAudioSource::CheckMPDAlive - MPD does not answers. Closing connection and try to reopen again.");
	this->DisarmMPDAliveWatchdog();
	mpd_connection_free(this->mpdCon);
	this->mpdCon=NULL;

	if (!this->ConnectToMPD())
		this->StartPollingMPD();
}

gboolean MPDAudioSource::MPDConnectionWatchdog(gpointer data)
{
	MPDAudioSource *instance=(MPDAudioSource *)data;

	instance->CheckMPDAliveAndReadTrackPos();

	return TRUE;
}

void MPDAudioSource::LoadPlayList()
{
	const char *playlist=ConfigGetRadioStationPlaylistName();
	Logger::LogDebug("MPDAudioSource::LoadPlayListAndTrack - Loading radio station playlist: %s", playlist);

	if (!mpd_run_clear(this->mpdCon))
		Logger::LogError("Unable to clear queue: %s",  mpd_connection_get_error_message (this->mpdCon));

	if (!mpd_run_load(this->mpdCon,playlist))
	{
		Logger::LogError("Unable to load playlist: %s",  mpd_connection_get_error_message (this->mpdCon));
		return;
	}

	if (!mpd_run_repeat(this->mpdCon,true))
	{
		Logger::LogError("Unable to set player into repeat mode: %s",  mpd_connection_get_error_message (this->mpdCon));
		return;
	}
}

void MPDAudioSource::DoStartPlaying()
{
	this->trackNr=RetroradioController::Instance()->GetPersistentState()->GetMPDCurrentTrackNr();
	if (this->mpdCon==NULL)
	{
		Logger::LogError("MPD sources assumed to be in state START_PLAYING but mpd connection object found to be NULL.");
		return;
	}

	Logger::LogDebug("MPDAudioSource::DoStartPlaying - Start playing track %d", trackNr);

	mpd_run_play_pos(this->mpdCon, this->trackNr);
	this->SourceStartPlayingFinished();
}

void MPDAudioSource::DoStopPlaying()
{
	Logger::LogDebug("MPDAudioSource::DoStopPlaying - About to stop source %s.", this->GetName());
	if (this->mpdCon==NULL)
	{
		Logger::LogError("MPD sources assumed to be in state STOP_PLAYING but mpd connection object found to be NULL.");
		return;
	}

	Logger::LogDebug("MPDAudioSource::DoStopPlaying - Checking for pending track change commands.");
	if (this->trackChangeTransition.GetState()==TrackChangeTransition::RAMPING_DOWN)
	{
		this->ProcessPendingTrackChangeCommands();
		this->trackChangeTransition.Finished();
	}

	Logger::LogDebug("MPDAudioSource::DoStopPlaying - Stop playing track %d", this->PersGetTrackNumber());
	mpd_run_stop(this->mpdCon);
	this->SourceStopPlayingFinished();
}

void MPDAudioSource::Next()
{
	Logger::LogDebug("MPDAudioSource::Next - MPD source received next command.");
	if (this->GetState()==PLAYING)
	{
		switch(this->trackChangeTransition.GetState())
		{
		case TrackChangeTransition::IDLE:
		case TrackChangeTransition::RAMPING_UP:
			this->KickOffChangeTrackTransition();
			//No break by intention: Need to set first next call as well after kicking off the ramp
		case TrackChangeTransition::RAMPING_DOWN:
			this->trackChangeTransition.NextPressed();
			break;
		}
	}
}

void MPDAudioSource::SourceActivationFinished()
{
	this->LoadPlayList();
	AbstractAudioSource::SourceActivationFinished();
}

void MPDAudioSource::Previous()
{
	Logger::LogDebug("MPDAudioSource::Previous - MPD source received Previous command.");
	if (this->GetState()==PLAYING)
	{
		switch(this->trackChangeTransition.GetState())
		{
		case TrackChangeTransition::IDLE:
		case TrackChangeTransition::RAMPING_UP:
			this->KickOffChangeTrackTransition();
			//No break by intention: Need to set first next call as well after kicking off the ramp
		case TrackChangeTransition::RAMPING_DOWN:
			this->trackChangeTransition.PreviousPressed();
			break;
		}
	}
}

void MPDAudioSource::Favorite(FavoriteT favorite)
{
	Logger::LogDebug("MPDAudioSource::Previous - MPD source received favorite command. Fav: %d", favorite);
	if (this->GetState()==PLAYING)
	{

		this->CheckMPDAliveAndReadTrackPos();
		if (favorite<0 || favorite>this->queueLength-1)
		{
			Logger::LogInfo("Ignoring favorite %d since it is out of mpd queue range (0-%d).", favorite, this->queueLength-1);
			return;
		}

		if (favorite==this->trackNr)
		{
			Logger::LogInfo("Ignoring favorite %d since it is currently played.", favorite);
			return;
		}

		switch(this->trackChangeTransition.GetState())
		{
		case TrackChangeTransition::IDLE:
		case TrackChangeTransition::RAMPING_UP:
			this->KickOffChangeTrackTransition();
			//No break by intention: Need to set first next call as well after kicking off the ramp
		case TrackChangeTransition::RAMPING_DOWN:
			this->trackChangeTransition.TrackSelected((unsigned int)favorite);
			break;
		}
	}
}

void MPDAudioSource::ProcessPendingTrackChangeCommands()
{
	this->ProcessPendingSelectTrackCommand();
	this->ProcessPendingNextPrevCommands();
}

void MPDAudioSource::ProcessPendingSelectTrackCommand()
{
	int trackNo=this->trackChangeTransition.GetSelectedTrack(true);
	if (this->mpdCon==NULL || trackNo==_NO_TRACK_SET_)
		return;

	mpd_run_play_pos(this->mpdCon, trackNo);
 	Logger::LogDebug("MPDAudioSource::ProcessPendingSelectTrackCommand - MPD source changed to track %d.",trackNo);
	this->CheckMPDAliveAndReadTrackPos();
	RetroradioController::Instance()->GetPersistentState()->SetMPDCurrentTrackNr(this->trackNr);
}

void MPDAudioSource::ProcessPendingNextPrevCommands()
{
	bool changeForward=this->trackChangeTransition.NextCallsPending();
	while (this->trackChangeTransition.OneChangeProcessed())
	{
		if (this->mpdCon==NULL) continue;
		if (changeForward)
			mpd_run_next(this->mpdCon);
		else
			mpd_run_previous(this->mpdCon);
		Logger::LogDebug("MPDAudioSource::ProcessPendingNextPrevCommands - MPD source changed to %s track.",
				changeForward ? "next" : "previous");
	}
	this->CheckMPDAliveAndReadTrackPos();
	RetroradioController::Instance()->GetPersistentState()->SetMPDCurrentTrackNr(this->trackNr);
}

void MPDAudioSource::FinalizeChangeTrackTransition()
{
	this->trackChangeTransition.Finalize();
	if (!this->IsMuted())
		this->StartUnMuteRamp(SourceMuteRampCtrl::NORMAL);
	else
		this->trackChangeTransition.Finished();
}

void MPDAudioSource::KickOffChangeTrackTransition()
{
	this->trackChangeTransition.StartNew();
	this->StartMuteRamp(SourceMuteRampCtrl::FAST);
}

void MPDAudioSource::OnRampFinished(bool canceled)
{
	AbstractAudioSource::OnRampFinished(canceled);
	if (this->GetState()!=PLAYING) return;

	if (this->trackChangeTransition.GetState()==TrackChangeTransition::RAMPING_DOWN)
	{
		this->ProcessPendingTrackChangeCommands();
		this->FinalizeChangeTrackTransition();
	}
	else if (this->trackChangeTransition.GetState()==TrackChangeTransition::RAMPING_UP)
		this->trackChangeTransition.Finished();
}

bool MPDAudioSource::IsMuteDownRampAllowed()
{
	return AbstractAudioSource::IsMuteDownRampAllowed();
}

bool MPDAudioSource::IsMuteUpRampAllowed()
{
	if (this->trackChangeTransition.GetState()==TrackChangeTransition::RAMPING_DOWN)
		return false;
	return AbstractAudioSource::IsMuteUpRampAllowed();
}

const char* MPDAudioSource::ConfigGetMPDHost()
{
	return this->mpdHost!=NULL ? this->mpdHost : MPD_DEFAULT_HOST;
}

unsigned int MPDAudioSource::ConfigGetMPDPort()
{
	return this->mpdPort;
}

const char* MPDAudioSource::ConfigGetRadioStationPlaylistName()
{
	return this->mpdStationPlayList!=NULL ? this->mpdStationPlayList : MPD_DEFAULT_RADIO_STATION_PLAYLIST;
}

unsigned int MPDAudioSource::PersGetTrackNumber()
{
	return this->trackNr;
}

const char* MPDAudioSource::GetConfigGroupName()
{
	return MPC_CONFIG_GROUP;
}

const char* MPDAudioSource::GetDefaultAlsaMixerName()
{
	return MPC_DEFAULT_ALSA_MIXER_NAME;
}

const char* MPDAudioSource::GetDefaultSoundCardName()
{
	return MPC_DEFAULT_SOUND_CARD_NAME;
}

bool MPDAudioSource::ParseConfigFileItem(GKeyFile* confFile, const char* group, const char* key)
{
	const char *groupName;
	bool result=true;

	if (!AbstractAudioSource::ParseConfigFileItem(confFile,group, key))
		return false;

	groupName=this->GetConfigGroupName();
	if (strcasecmp(key, MPD_CONFIG_TAG_HOST)==0)
	{
		char *host;
		if (Configuration::GetStringValueFromKey(confFile,key,groupName, &host))
		{
			if (this->mpdHost)
				free(this->mpdHost);
			this->mpdHost=host;
		}
		else
			result=false;
	}
	else if (strcasecmp(key, MPD_CONFIG_TAG_PORT)==0)
	{
		int port;
		if (Configuration::GetInt64ValueFromKey(confFile,key,groupName, &port))
			this->mpdPort=port;
		else
			result=false;
	}
	else if (strcasecmp(key, MPD_CONFIG_TAG_PLAYLIST)==0)
	{
		char *playlist;
		if (Configuration::GetStringValueFromKey(confFile,key,groupName, &playlist))
		{
			if (this->mpdStationPlayList)
				free(this->mpdStationPlayList);
			this->mpdStationPlayList=playlist;
		}
		else
			result=false;
	}

	return result;
}
