/*
 * AbstractAudioSource.h
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#ifndef SRC_ABSTRACTAUDIOSOURCE_H_
#define SRC_ABSTRACTAUDIOSOURCE_H_

#include <glib.h>

#include "BasicMixerControl.h"
#include "AudioSources/SourceMuteRampCtrl.h"
#include "cpp-app-utils/Configuration.h"

using namespace CppAppUtils;

namespace retroradio_controller {

class AbstractAudioSource : public SourceMuteRampCtrl::IMuteRampCtrlListener,
	public Configuration::IConfigurationParserModule
{

public:
	enum FavoriteT
	{
		_FAV_NOT_SET_			= 0xFF,
		FAV0					= 0x00,
		FAV1					= 0x01,
		FAV2					= 0x02,
		FAV3					= 0x03,
		FAV4					= 0x04,
		FAV5					= 0x05,
		FAV6					= 0x06,
		FAV7					= 0x07,
		FAV8					= 0x08,
		FAV9					= 0x09,
	};

	enum State
	{
		_NOT_SET			= 0,
		DEACTIVATED			= 1,
		ACTIVATING			= 2,
		ACTIVE_IDLE			= 3,
		DEACTIVATING		= 4,
		START_PLAYING		= 5,
		START_PLAYING_RAMP 	= 6,
		PLAYING				= 7,
		STOP_PLAYING_RAMP	= 8,
		STOP_PLAYING		= 9
	};

	static const char *StateNames[];

	class IAudioSourceStateListener
	{
	public:
		virtual void OnStateChanged(AbstractAudioSource *src, State newState)=0;
	};


private:
	struct StateChangeEvent
	{
		AbstractAudioSource *src;
		State newState;
	};

	const char *name;

	State srcState;

	bool muted;

	SourceMuteRampCtrl *muteRampCtrl;

	IAudioSourceStateListener *listener;

	AbstractAudioSource *predecessor;

	AbstractAudioSource *successor;

	char *alsaMixerName;

	char *soundCardName;

	static gboolean NotifyStateChange(gpointer user_data);

	void EnterActivating(bool need2ReOpenSoundDevices);

	void EnterActivated();

	void EnterDeActivating();

	void EnterDeActivated();

	void EnterStartPlaying();

	void EnterStartPlayingRamp();

	void EnterPlaying();

	void EnterStopPlayingRamp();

	void EnterStopPlaying();

	void SetState(State newState);

protected:
	virtual const char *GetConfigGroupName()=0;

	virtual const char *GetDefaultAlsaMixerName()=0;

	virtual const char *GetDefaultSoundCardName()=0;

	virtual void DoActivateSource(bool need2ReOpenSoundDevices);

	virtual void SourceActivationFinished();

	virtual void DoDeActivateSource();

	virtual void SourceDeActivationFinished();

	virtual void DoStartPlaying();

	void SourceStartPlayingFinished();

	virtual void DoStopPlaying();

	void SourceStopPlayingFinished();

	State GetState();

	void StartMuteRamp(SourceMuteRampCtrl::RampSpeed speed);

	void StartUnMuteRamp(SourceMuteRampCtrl::RampSpeed speed);

protected:
	virtual void StopMuteRamp();

	virtual bool IsMuteDownRampAllowed();

	virtual bool IsMuteUpRampAllowed();

public:
	AbstractAudioSource(const char *srcName, AbstractAudioSource *predecessor,
			IAudioSourceStateListener *listener);

	virtual ~AbstractAudioSource();

	virtual bool Init();

	virtual void DeInit();

	AbstractAudioSource *GetSuccessor();

	AbstractAudioSource *GetPreDecessor();

	virtual bool IsStartupFinished();

	bool IsPlaying();

	bool IsTransitioningFromOrToPlay();

	bool IsActive();

	virtual void Activate(bool need2ReOpenSoundDevices);

	virtual void DeActivate();

	void SetMuted(bool muted);

	virtual void GoOnline();

	virtual void GoOffline(bool doMuteRamp);

	virtual void Next();

	virtual void Previous();

	virtual void Favorite(FavoriteT favorite);

	const char *GetName();

	virtual void OnRampFinished(bool canceled);

	bool IsMuted();

	virtual bool ParseConfigFileItem(GKeyFile *confFile, const char *group, const char *key);

	virtual bool IsConfigFileGroupKnown(const char *group);

};

} /* namespace retroradio_controller */

#endif /* SRC_ABSTRACTAUDIOSOURCE_H_ */
