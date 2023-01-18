/*
 * AbstractAudioSource.cpp
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#include "AbstractAudioSource.h"

#include "cpp-app-utils/Logger.h"
#include "RetroradioControllerConfiguration.h"

using namespace CppAppUtils;
using namespace retroradio_controller;

#define ALSA_MIXER_NAME_CONFIG_KEY		"AlsaMixerName"
#define SNDCARD_NAME_CONFIG_KEY			"SoundCardName"

const char *AbstractAudioSource::StateNames[] =
	{
			"_NOT_SET",
			"DEACTIVATED",
			"ACTIVATING",
			"ACTIVE_IDLE",
			"DEACTIVATING",
			"START_PLAYING",
			"START_PLAYING_RAMP",
			"PLAYING",
			"STOP_PLAYING_RAMP",
			"STOP_PLAYING"
	};



AbstractAudioSource::AbstractAudioSource(const char *srcName,
		AbstractAudioSource* predecessor,
		IAudioSourceListener *listener) :
		listener(listener),
		name(srcName),
		successor(NULL),
		srcState(_NOT_SET),
		muted(false)
{
	this->predecessor=predecessor;
	if (predecessor!=NULL)
		this->predecessor->successor=this;
	this->muteRampCtrl=new SourceMuteRampCtrl(this);
	alsaMixerName=NULL;
	soundCardName=NULL;
}

AbstractAudioSource::~AbstractAudioSource()
{
	delete this->muteRampCtrl;
	if (this->alsaMixerName!=NULL)
		free(this->alsaMixerName);

	if (this->soundCardName)
		free(this->soundCardName);
}

bool AbstractAudioSource::Init()
{
	const char *cardName;
	const char *mixerName;

	this->srcState=DEACTIVATED;
	cardName=this->soundCardName!=NULL ? this->soundCardName : this->GetDefaultSoundCardName();
	mixerName=this->alsaMixerName!=NULL ? this->alsaMixerName : this->GetDefaultAlsaMixerName();

	return true;
}

void AbstractAudioSource::DeInit()
{
	Logger::LogDebug("AbstractAudioSource::DeInit - Deinitializing Audio Source %s.", this->name);
	this->srcState=_NOT_SET;
}

void AbstractAudioSource::Activate(bool need2ReOpenSoundDevices)
{
	if (this->srcState!=DEACTIVATED)
		return;

	Logger::LogDebug("AbstractAudioSource::Activate - Activating source %s.", this->name);
	this->EnterActivating(need2ReOpenSoundDevices);
}

void AbstractAudioSource::DeActivate()
{
	Logger::LogDebug("AbstractAudioSource::DeActivate - Requesting source %s to deactivate. Current state: %s",
			this->name, StateNames[this->srcState]);
	if (this->srcState!=ACTIVE_IDLE && this->srcState!=ACTIVATING)
		return;

	Logger::LogDebug("AbstractAudioSource::DeActivate - Deactivating source %s.", this->name);
	this->EnterDeActivating();
}

void AbstractAudioSource::GoOnline()
{
	if (this->srcState!=ACTIVE_IDLE)
		return;

	Logger::LogDebug("AbstractAudioSource::GoOnline - Source %s is now going online.", this->name);
	this->EnterStartPlaying();
}

void AbstractAudioSource::GoOffline(bool doMuteRamp)
{
	if (this->srcState!=PLAYING)
		return;
	Logger::LogDebug("AbstractAudioSource::GoOffline - Source %s is now going offline.", this->name);
	if (doMuteRamp)
		this->EnterStopPlayingRamp();
	else
		this->EnterStopPlaying();
}

void AbstractAudioSource::Next()
{
	Logger::LogDebug("AbstractAudioSource::Next - Source %s received next command.", this->name);
}

void AbstractAudioSource::Previous()
{
	Logger::LogDebug("AbstractAudioSource::Previous - Source %s received previous command.", this->name);
}

void AbstractAudioSource::Favorite(FavoriteT favorite)
{
	Logger::LogDebug("AbstractAudioSource::Previous - Source %s received favorite command. Fav: %d", this->name, favorite);
}

AbstractAudioSource *AbstractAudioSource::GetSuccessor()
{
	return this->successor;
}

AbstractAudioSource *AbstractAudioSource::GetPreDecessor()
{
	return this->predecessor;
}

const char *AbstractAudioSource::GetName()
{
	return this->name;
}

void AbstractAudioSource::EnterActivating(bool need2ReOpenSoundDevices)
{
	Logger::LogDebug("AbstractAudioSource::EnterActivating - Activating source %s.", this->name);
	this->SetState(ACTIVATING);

	if (need2ReOpenSoundDevices)
	{
		Logger::LogDebug("AbstractAudioSource::EnterActivating - Need to initialize mute ramp control.");
		this->muteRampCtrl->DeInit();
		this->muteRampCtrl->Init(this->soundCardName, this->alsaMixerName);
	}

	this->DoActivateSource(need2ReOpenSoundDevices);
}

void AbstractAudioSource::EnterActivated()
{
	Logger::LogDebug("AbstractAudioSource::EnterActivated - Source %s activated.", this->name);
	this->SetState(ACTIVE_IDLE);
}

void AbstractAudioSource::DoActivateSource(bool need2ReOpenSoundDevices)
{
	this->SourceActivationFinished();
}

void AbstractAudioSource::SourceActivationFinished()
{
	this->EnterActivated();
}

void AbstractAudioSource::EnterDeActivating()
{
	Logger::LogDebug("AbstractAudioSource::EnterDeActivating - DeActivating source %s.", this->name);
	this->SetState(DEACTIVATING);
	this->DoDeActivateSource();
}

void AbstractAudioSource::EnterDeActivated()
{
	Logger::LogDebug("AbstractAudioSource::EnterDeActivated - Source %s deactivated.", this->name);
	this->SetState(DEACTIVATED);
}

void AbstractAudioSource::DoDeActivateSource()
{
	this->SourceDeActivationFinished();
}

void AbstractAudioSource::SourceDeActivationFinished()
{
	this->EnterDeActivated();
}

void AbstractAudioSource::EnterStartPlaying()
{
	Logger::LogDebug("AbstractAudioSource::EnterStartPlaying - Source %s prepares for playing.", this->name);
	this->SetState(START_PLAYING);
	this->DoStartPlaying();
}

void AbstractAudioSource::DoStartPlaying()
{
	this->SourceStartPlayingFinished();
}

void AbstractAudioSource::SourceStartPlayingFinished()
{
	if (this->muted)
	{
		Logger::LogDebug("AbstractAudioSource::SourceStartPlayingFinished - Audio controller muted. Not ramping up volume of source %s.", this->name);
		this->EnterPlaying();
	}
	else
		this->EnterStartPlayingRamp();
}

void AbstractAudioSource::EnterStartPlayingRamp()
{
	Logger::LogDebug("AbstractAudioSource::EnterStartPlayingRamp - Source %s ramps up volume.", this->name);
	this->SetState(START_PLAYING_RAMP);
	this->muteRampCtrl->UnmuteAsync(SourceMuteRampCtrl::NORMAL);
}

void AbstractAudioSource::EnterPlaying()
{
	Logger::LogDebug("AbstractAudioSource::EnterPlaying - Source %s now playing.", this->name);
	this->SetState(PLAYING);
}

void AbstractAudioSource::EnterStopPlayingRamp()
{
	Logger::LogDebug("AbstractAudioSource::EnterStopPlayingRamp - Source %s ramps down volume.", this->name);
	this->SetState(STOP_PLAYING_RAMP);
	this->muteRampCtrl->MuteAsync(SourceMuteRampCtrl::NORMAL);
}

void AbstractAudioSource::EnterStopPlaying()
{
	Logger::LogDebug("AbstractAudioSource::EnterStopPlaying - Source %s about to stop playing.", this->name);
	this->SetState(STOP_PLAYING);
	this->DoStopPlaying();
}

void AbstractAudioSource::DoStopPlaying()
{
	this->SourceStopPlayingFinished();
}

void AbstractAudioSource::SourceStopPlayingFinished()
{
	this->EnterActivated();
}

void AbstractAudioSource::SetState(State newState)
{
	this->srcState=newState;
	Logger::LogDebug("AbstractAudioSource::SetState - Source %s now in state: %s", this->name, StateNames[newState]);
	if (this->listener != NULL)
	{
		StateChangeEvent *event=new StateChangeEvent;
		event->src=this;
		event->newState=newState;
		g_idle_add(AbstractAudioSource::NotifyStateChange, event);
	}
}

gboolean AbstractAudioSource::NotifyStateChange(gpointer user_data)
{
	StateChangeEvent *event=(StateChangeEvent *)user_data;
	event->src->listener->OnStateChanged(event->src, event->newState);
	delete event;

	return FALSE;
}

bool AbstractAudioSource::IsPlaying()
{
	return this->srcState==PLAYING;
}

bool AbstractAudioSource::IsTransitioningFromOrToPlay()
{
	return this->srcState==START_PLAYING || this->srcState==START_PLAYING_RAMP ||
		   this->srcState==STOP_PLAYING || this->srcState==STOP_PLAYING_RAMP;
}


bool AbstractAudioSource::IsActive()
{
	return this->srcState!=_NOT_SET && this->srcState!=ACTIVATING &&
		this->srcState!=DEACTIVATING && this->srcState!=DEACTIVATED;
}

void AbstractAudioSource::StopTransition()
{
	Logger::LogDebug("AbstractAudioSource::StopTransition - Source %s requested to stop any transition ongoing.", this->name);
	this->muteRampCtrl->StopOperation();
}

AbstractAudioSource::State AbstractAudioSource::GetState()
{
	return this->srcState;
}

void AbstractAudioSource::OnRampFinished(bool canceled)
{
	Logger::LogDebug("AbstractAudioSource::OnRampFinished - Source %s received a ramp %s signal from ramp control.",
			this->name, canceled ? "canceled" : "finished");

	Logger::LogDebug("AbstractAudioSource::OnRampFinished - State %s", StateNames[this->srcState]);

	if (this->srcState==START_PLAYING_RAMP)
		this->EnterPlaying();
	else if (this->srcState==STOP_PLAYING_RAMP)
		this->EnterStopPlaying();
}

void AbstractAudioSource::SetMuted(bool muted)
{
	Logger::LogDebug("AbstractAudioSource::SetMuted - Source %s received %s request.",
			this->name, muted ? "mute" : "unmute");

	this->muted=muted;

	// in state PLAYING: mute and unmute must be executed
	// in state START_PLAYING_RAMP: at least mute must be executed to ramp down volume again, unmute is executed as
	//  							well not having any effect on the already ramping up volume
	if (muted)
	{
		if (this->IsMuteDownRampAllowed())
			this->muteRampCtrl->MuteAsync(SourceMuteRampCtrl::NORMAL);
	}
	else
	{
		if (this->IsMuteUpRampAllowed())
			this->muteRampCtrl->UnmuteAsync(SourceMuteRampCtrl::NORMAL);
	}
}

bool AbstractAudioSource::IsMuteDownRampAllowed()
{
	return this->srcState==PLAYING || this->srcState==START_PLAYING_RAMP;
}

bool AbstractAudioSource::IsMuteUpRampAllowed()
{
	return this->srcState==PLAYING || this->srcState==START_PLAYING_RAMP;
}

void AbstractAudioSource::StartMuteRamp(SourceMuteRampCtrl::RampSpeed speed)
{
	this->muteRampCtrl->MuteAsync(speed);
}

void AbstractAudioSource::StartUnMuteRamp(SourceMuteRampCtrl::RampSpeed speed)
{
	this->muteRampCtrl->UnmuteAsync(speed);
}

bool AbstractAudioSource::IsMuted()
{
	return this->muted;
}

bool AbstractAudioSource::IsStartupFinished()
{
	return true;
}

bool AbstractAudioSource::IsConfigFileGroupKnown(
		const char* group)
{
	return strcmp(group, this->GetConfigGroupName())==0;
}

bool AbstractAudioSource::ParseConfigFileItem(
		GKeyFile* confFile, const char* group, const char* key)
{
	const char *groupName;
	bool result=true;

	groupName=this->GetConfigGroupName();
	if (strcasecmp(group, groupName)!=0) return true;

	if (strcasecmp(key, SNDCARD_NAME_CONFIG_KEY)==0)
	{
		char *cardName;
		if (Configuration::GetStringValueFromKey(confFile,key,groupName, &cardName))
		{
			if (this->soundCardName)
				free(this->soundCardName);
			this->soundCardName=cardName;
		}
		else
			result=false;
	}
	else if (strcasecmp(key, ALSA_MIXER_NAME_CONFIG_KEY)==0)
	{
		char *mixerName;
		if (Configuration::GetStringValueFromKey(confFile,key,groupName, &mixerName))
		{
			if (this->alsaMixerName!=NULL)
				free(this->alsaMixerName);
			this->alsaMixerName=mixerName;
		}
	}

	return result;
}
