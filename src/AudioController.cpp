/*
 * AudioController.cpp
 *
 *  Created on: 22.11.2019
 *      Author: joe
 */

#include "AudioController.h"

#include "RetroradioController.h"

#include <cpp-app-utils/Logger.h>

using namespace CppAppUtils;

namespace retroradio_controller {

const char *AudioController::StateNames[] =
	{
			"_NOT_SET",
			"STARTING_UP",
			"DEACTIVATED",
			"ACTIVATING_SOURCES",
			"STARTING_PLAYING",
			"ACTIVATED",
			"STOPING_PLAYING",
			"DEACTIVATING_SOURCES"
	};



AudioController::AudioController(IStateListener *stateListener, Configuration *configuration) :
		muted(false),
		state(_NOT_SET),
		listener(stateListener)
{
	this->audioSources=new RetroradioAudioSourceList(this, configuration);
	this->mainVolumeCtrl=new MainVolumeControl(configuration);
}

AudioController::~AudioController()
{
	delete this->mainVolumeCtrl;
	delete this->audioSources;
}

bool AudioController::Init()
{
	Logger::LogDebug("AudioController::Init -> Initializing Audio Controller.");

	this->state=STARTING_UP;
	return this->audioSources->Init();
}

void AudioController::DeInit()
{
	this->state=_NOT_SET;
	this->mainVolumeCtrl->DeInit();
	this->audioSources->DeInit();
	Logger::LogDebug("AudioController::DeInit -> Deinitialized Audio controller");
}

void AudioController::DoChangeToSource()
{
	//TODO: volume ramp down
	//TODO: change source to this->currentSource
	//TODO: Persist state
	//TODO: volume ramp up
}

void AudioController::ChangeToNextSource()
{
	Logger::LogDebug("AudioController::ChangeToNextSource -> Audio Controller requested to change to next source.");
	this->DoChangeToSource();
}

bool AudioController::CheckSourcesStartupState()
{
	bool result=true;
	if (this->state != STARTING_UP) return true;

	for (AbstractAudioSource *itr=this->audioSources->GetIterator(); itr!=NULL; itr=itr->GetSuccessor())
	{
		if (!itr->IsStartupFinished())
			result=false;
		else
			RetroradioController::Instance()->GetGPIOController()->SetSourceLedEnabled(itr->GetName(), true);
	}

	if (result)
		RetroradioController::Instance()->GetGPIOController()->DisableSourcesLeds();

	return result;
}

void AudioController::Mute()
{
	Logger::LogDebug("AudioController::Mute -> Audio Controller requested to mute.");
	if (this->muted) return;

	this->audioSources->GetCurrentSource()->SetMuted(true);
	this->muted=true;
}

void AudioController::UnMute()
{
	Logger::LogDebug("AudioController::UnMute -> Audio Controller requested to unmute.");
	if (!this->muted) return;

	this->audioSources->GetCurrentSource()->SetMuted(false);
	this->muted=false;
}

void AudioController::ToggleMute()
{
	Logger::LogDebug("AudioController::ToggleMute -> Audio Controller requested to toggle mute state.");

	if (this->muted)
		this->UnMute();
	else
		this->Mute();
}

void AudioController::VolumeUp()
{
	Logger::LogDebug("AudioController::VolumeUp -> Audio Controller requested to increase volume.");
	if (this->muted)
		this->UnMute();
	this->mainVolumeCtrl->VolumeUp();
}

void AudioController::VolumeDown()
{
	Logger::LogDebug("AudioController::VolumeDown -> Audio Controller requested to decrease volume.");
	if (this->muted)
		this->UnMute();
	this->mainVolumeCtrl->VolumeDown();
}

void AudioController::ActivateAudioController(bool need2ReOpenSoundDevices)
{
	if (this->state!=DEACTIVATED && this->state!=STARTING_UP) return;
	Logger::LogDebug("AudioController::ActivateAudioController -> Activating audio controller.");
	this->EnterActivatingSources(need2ReOpenSoundDevices);
}

void AudioController::DeactivateController(bool doMuteRamp)
{
	Logger::LogDebug("AudioController::DeactivateAudioController -> DeActivating audio controller.");
	//already on deactivation sequence? -> do nothing
	if (this->state==DEACTIVATED || this->state==DEACTIVATING_SOURCES || this->state==STOPING_PLAYING) return;

	if (this->state==ACTIVATED || this->state==STARTING_PLAYING)
		this->EnterStopPlaying(doMuteRamp);
	else if (this->state==ACTIVATING_SOURCES)
		this->EnterDeactivatingSources();
}

void AudioController::TriggerSourceNextPressed()
{
	Logger::LogDebug("AudioController::TriggerSourceNextPressed -> Audio controller request to trigger current source that next has pressed.");
	this->audioSources->GetCurrentSource()->Next();
}

void AudioController::TriggerSourcePrevPressed()
{
	Logger::LogDebug("AudioController::TriggerSourcePrevPressed -> Audio controller request to trigger current source that prev has pressed.");
	this->audioSources->GetCurrentSource()->Previous();
}

void AudioController::TriggerFavPressed(AbstractAudioSource::FavoriteT favorite)
{
	Logger::LogDebug("AudioController::TriggerSourcePrevPressed -> Audio controller request to trigger"
			" current source that a favorite key has been pressed. Fav: %d", favorite);

	this->audioSources->GetCurrentSource()->Favorite(favorite);
}

void AudioController::OnStateChanged(AbstractAudioSource *src, AbstractAudioSource::State newState)
{
	Logger::LogDebug("AudioController::OnStateChanged -> Audio controller received state change event from source %s. New State: %s",
			src->GetName(), AbstractAudioSource::StateNames[newState]);

	switch(this->state)
	{
	case _NOT_SET:
		Logger::LogError("State Machine Error: Audio controller in state _NOT_SET but source %s changed to state %s.", src->GetName(),
				AbstractAudioSource::StateNames[newState]);
		break;

	case DEACTIVATED:
	case ACTIVATED:
		break;

	case ACTIVATING_SOURCES:
		if (this->audioSources->AreAllActivated())
			this->EnterStartPlaying();
		break;

	case STARTING_PLAYING:
		if (this->audioSources->GetCurrentSource()->IsPlaying())
			this->EnterActivated();
		break;

	case STOPING_PLAYING:
		if (!this->audioSources->GetCurrentSource()->IsPlaying() &&
			!this->audioSources->GetCurrentSource()->IsTransitioningFromOrToPlay())
			this->EnterDeactivatingSources();
		break;

	case DEACTIVATING_SOURCES:
		if (!this->audioSources->AreAllDeactivated())
			this->EnterDeactivated();
		break;
	}
}

void AudioController::EnterStopPlaying(bool doMuteRamp)
{
	this->CheckStateMachine(this->state==ACTIVATED || this->state==STARTING_PLAYING, "STOP_PLAYING");
	this->SetState(STOPING_PLAYING);
	this->audioSources->GetCurrentSource()->GoOffline(doMuteRamp);
	if (!this->audioSources->GetCurrentSource()->IsPlaying() &&
		!this->audioSources->GetCurrentSource()->IsTransitioningFromOrToPlay())
		this->EnterDeactivatingSources();
}

void AudioController::EnterDeactivatingSources()
{
	this->CheckStateMachine(this->state==STOPING_PLAYING || this->state==ACTIVATING_SOURCES , "DEACTIVATING_SOURCES");
	this->mainVolumeCtrl->Mute();
	this->audioSources->DeactivateAll();
	this->SetState(DEACTIVATING_SOURCES);
	if (this->audioSources->AreAllDeactivated())
		this->EnterDeactivated();
}

void AudioController::EnterDeactivated()
{
	RetroradioController::Instance()->GetGPIOController()->DisableSourcesLeds();
	this->CheckStateMachine(this->state==DEACTIVATING_SOURCES, "DEACTIVED");
	this->SetState(DEACTIVATED);
}

void AudioController::EnterActivatingSources(bool need2ReOpenSoundDevices)
{
	RetroradioController::Instance()->GetGPIOController()->SetSourceLedEnabled(this->audioSources->GetCurrentSource()->GetName(), true);

	if (need2ReOpenSoundDevices)
	{
		this->mainVolumeCtrl->DeInit();
		this->mainVolumeCtrl->Init();
	}

	this->audioSources->ActivateAll(need2ReOpenSoundDevices);
	this->SetState(ACTIVATING_SOURCES);

	if (this->audioSources->AreAllActivated())
		this->EnterStartPlaying();
}

void AudioController::EnterStartPlaying()
{
	this->mainVolumeCtrl->UnMute();
	this->audioSources->GetCurrentSource()->GoOnline();
	this->SetState(STARTING_PLAYING);
	if (this->audioSources->GetCurrentSource()->IsPlaying())
		this->EnterActivated();
}

void AudioController::EnterActivated()
{
	this->SetState(ACTIVATED);
}

void AudioController::SetState(State newState)
{
	this->state=newState;
	Logger::LogDebug("AudioController::SetState -> Audio controller entered new state: %s", StateNames[newState]);

	if (this->listener != NULL)
	{
		StateChangeEvent *event=new StateChangeEvent;
		event->controller=this;
		event->newState=newState;
		g_idle_add(AudioController::NotifyStateChange, event);
	}
}

gboolean AudioController::NotifyStateChange(gpointer user_data)
{
	StateChangeEvent *event=(StateChangeEvent *)user_data;
	event->controller->listener->OnStateChanged(event->newState);
	delete event;

	return FALSE;
}

void AudioController::CheckStateMachine(bool passed, const char *stateToEnter)
{
	if (!passed)
		Logger::LogError("AudioController state machine error. Entering state %s from state: %s", stateToEnter,
				StateNames[this->state]);
}

AudioController::State AudioController::GetState()
{
	return this->state;
}

} /* namespace retroradiocontroller */

