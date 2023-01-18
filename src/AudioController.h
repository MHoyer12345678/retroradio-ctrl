/*
 * AudioController.h
 *
 *  Created on: 22.11.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOCONTROLLER_H_
#define SRC_AUDIOCONTROLLER_H_

#include "AudioSources/RetroradioAudioSourceList.h"

#include "MainVolumeControl.h"


namespace retroradio_controller
{

class AudioController : public AbstractAudioSource::IAudioSourceListener
{

public:
	enum State
	{
		_NOT_SET				= 0,
		STARTING_UP				= 1,
		DEACTIVATED				= 2,
		ACTIVATING_SOURCES		= 3,
		STARTING_PLAYING		= 4,
		ACTIVATED				= 5,
		STOPING_PLAYING			= 6,
		DEACTIVATING_SOURCES	= 7,
		CHG_SRC_CUR_DOWN		= 8,
		CHG_SRC_NEW_UP			= 9
	};

	static const char *StateNames[];

	class IStateListener
	{
	public:
		virtual void OnStateChanged(State newState)=0;
	};

private:
	struct StateChangeEvent
	{
		AudioController *controller;
		State newState;
	};

	bool muted;

	IStateListener *listener;

	RetroradioAudioSourceList *audioSources;

	MainVolumeControl *mainVolumeCtrl;

	State state;

	void MuteMasterVolume();

	void UnMuteMasterVolume();

	void SetState(State newState);

	void CheckStateMachine(bool passed, const char *stateToEnter);

	static gboolean NotifyStateChange(gpointer user_data);

	const char *ConfigGetMixerName();

	const char *ConfigGetCardName();
public:
	AudioController(IStateListener *stateListener, Configuration *configuration);

	virtual ~AudioController();

	bool Init();

	void DeInit();

	bool CheckSourcesStartupState();

	void ActivateAudioController(bool need2ReOpenSoundDevices);

	void DeactivateController(bool doMuteRamp);

	void EnterStopPlaying(bool doMuteRamp);

	void EnterDeactivatingSources();

	void EnterDeactivated();

	void EnterActivatingSources(bool need2ReOpenSoundDevices);

	void EnterStartPlaying();

	void EnterActivated();

	void EnterChangeSrcCurDown();

	void EnterChangeSrcNewUp();

	void EnterActivated();

	void ChangeToNextSource();

	void Mute();

	void UnMute();

	void ToggleMute();

	void VolumeUp();

	void VolumeDown();

	void TriggerSourceNextPressed();

	void TriggerSourcePrevPressed();

	void TriggerFavPressed(AbstractAudioSource::FavoriteT favorite);

	virtual void OnStateChanged(AbstractAudioSource *src, AbstractAudioSource::State newState);

	State GetState();
};

} /* namespace retroradiocontroller */

#endif /* SRC_AUDIOCONTROLLER_H_ */
