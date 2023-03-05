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
			"DEACTIVATING_SOURCES",
			"CHG_SRC_CUR_DOWN",
			"CHG_SRC_NEW_UP"
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

void AudioController::ChangeToNextSource()
{
	Logger::LogDebug("AudioController::ChangeToNextSource -> Audio Controller requested to change to next source.");

	switch(this->state)
	{
	case ACTIVATING_SOURCES:
		//if in activation sequence, src can just be changed, no ramping
		this->audioSources->StartChangeToSourceNext();
		this->audioSources->FinishChangeToSource();
		this->UpdateSrcLeds();
		break;
	case ACTIVATED:
	case STARTING_PLAYING:
	case CHG_SRC_NEW_UP:
		// current source is playing or about to play or ramping up -> enter down sequence
		this->EnterChangeSrcCurDown();
		break;
	case CHG_SRC_CUR_DOWN:
		// already in down sequence of changing a src -> call next to switch on to
		// next src being selected after down sequence is done
		this->audioSources->StartChangeToSourceNext();
		this->UpdateSrcLeds();
		break;

	//ignoring by intention.
	case STARTING_UP: //No src changes while radio is booting up
	case STOPING_PLAYING: //No src changes in poweroff sequence
	case DEACTIVATING_SOURCES: //No src changes in poweroff sequence
	case DEACTIVATED: //No src changes when powered off
	default: //No src changes when controller is not initialized
		break;
	}
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
		this->EnterDeactivated();

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
	if (this->state!=DEACTIVATED) return;
	Logger::LogDebug("AudioController::ActivateAudioController -> Activating audio controller.");
	this->EnterActivatingSources(need2ReOpenSoundDevices);
}

void AudioController::DeactivateController(bool doMuteRamp)
{
	Logger::LogDebug("AudioController::DeactivateAudioController -> DeActivating audio controller.");

	switch(this->state)
		{
		case _NOT_SET:				//controller not initialized -> ignore request
		case DEACTIVATED: 			//already deactivated? -> do nothing
		case DEACTIVATING_SOURCES:  //already in deactivation sequence? -> do nothing
		case STOPING_PLAYING:		//already in deactivation sequence? -> do nothing
			break;

		case ACTIVATED:				//playing -> stop playing
		case STARTING_PLAYING:		//about to play after power on -> stop playing
		case CHG_SRC_NEW_UP:		//about to play after src change -> stop playing
			this->EnterStopPlaying(doMuteRamp);
			break;

		case ACTIVATING_SOURCES:	//activating sources, not yet playing -> deactivate sources
			this->EnterDeactivatingSources();
			break;

		case CHG_SRC_CUR_DOWN:		//src change down sequence -> back to activated, kick off stop playing sequence
			this->SetState(ACTIVATED); // back to active state ramp does not to be stopped, being kicked off again on stop playing
			this->EnterStopPlaying(doMuteRamp);
			break;
		}
}

void AudioController::TriggerSourceNextPressed()
{
	Logger::LogDebug("AudioController::TriggerSourceNextPressed -> Audio controller request to trigger current source that next has pressed.");

	//command passed to scheduled src
	//- in case of a src state transition: command passed already to new one
	//- else: command is passed to current one (scheduled == current in case of no transition ongoing)
	this->audioSources->GetScheduledSource()->Next();
}

void AudioController::TriggerSourcePrevPressed()
{
	Logger::LogDebug("AudioController::TriggerSourcePrevPressed -> Audio controller request to trigger current source that prev has pressed.");

	//command passed to scheduled src
	//- in case of a src state transition: command passed already to new one
	//- else: command is passed to current one (scheduled == current in case of no transition ongoing)
	this->audioSources->GetScheduledSource()->Previous();
}

void AudioController::TriggerFavPressed(AbstractAudioSource::FavoriteT favorite)
{
	Logger::LogDebug("AudioController::TriggerSourcePrevPressed -> Audio controller request to trigger"
			" current source that a favorite key has been pressed. Fav: %d", favorite);

	//command passed to scheduled src
	//- in case of a src state transition: command passed already to new one
	//- else: command is passed to current one (scheduled == current in case of no transition ongoing)
	this->audioSources->GetScheduledSource()->Favorite(favorite);
}

void AudioController::OnStateChanged(AbstractAudioSource *src, AbstractAudioSource::State newState)
{
	Logger::LogDebug("AudioController::OnStateChanged -> Audio controller received state change event from source %s. New State: %s",
			src->GetName(), AbstractAudioSource::StateNames[newState]);

	AbstractAudioSource* currentSrc=this->audioSources->GetCurrentSource();

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

	case CHG_SRC_NEW_UP:
	case STARTING_PLAYING:
		if (currentSrc->IsPlaying())
			this->EnterActivated();
		break;

	case STOPING_PLAYING:
		if (!currentSrc->IsPlaying() &&
			!currentSrc->IsTransitioningFromOrToPlay())
			this->EnterDeactivatingSources();
		break;

	case DEACTIVATING_SOURCES:
		if (!this->audioSources->AreAllDeactivated())
			this->EnterDeactivated();
		break;

	case CHG_SRC_CUR_DOWN:
		if (!currentSrc->IsPlaying() &&
			!currentSrc->IsTransitioningFromOrToPlay())
			this->EnterChangeSrcNewUp();
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
	this->CheckStateMachine(this->state==DEACTIVATING_SOURCES || this->state==STARTING_UP,
			"DEACTIVED");
	this->SetState(DEACTIVATED);
	this->UpdateSrcLeds();
}

void AudioController::EnterActivatingSources(bool need2ReOpenSoundDevices)
{
	if (need2ReOpenSoundDevices)
	{
		this->mainVolumeCtrl->DeInit();
		this->mainVolumeCtrl->Init();
	}

	this->audioSources->ActivateAll(need2ReOpenSoundDevices);
	this->SetState(ACTIVATING_SOURCES);
	this->UpdateSrcLeds();

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

void AudioController::EnterChangeSrcCurDown()
{
	this->audioSources->StartChangeToSourceNext(); //schedule src change to next src
	this->UpdateSrcLeds();
	this->audioSources->GetCurrentSource()->GoOffline(true);

	this->SetState(State::CHG_SRC_CUR_DOWN);

	if (!this->audioSources->GetCurrentSource()->IsTransitioningFromOrToPlay())
		this->EnterChangeSrcNewUp();
}

void AudioController::EnterChangeSrcNewUp()
{
	this->audioSources->FinishChangeToSource(); //switch to new src
	this->audioSources->GetCurrentSource()->GoOnline();
	this->SetState(State::CHG_SRC_NEW_UP);

	if (this->audioSources->GetCurrentSource()->IsPlaying())
		this->EnterActivated();
}

void AudioController::UpdateSrcLeds()
{
	if (this->state == STARTING_UP) return; //startup state is handled somewhere else

	RetroradioController::Instance()->GetGPIOController()->DisableSourcesLeds();
	if (this->state!=State::DEACTIVATED)
		RetroradioController::Instance()->GetGPIOController()->SetSourceLedEnabled(
				this->audioSources->GetScheduledSource()->GetName(), true);
}

} /* namespace retroradiocontroller */

