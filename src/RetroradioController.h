/*
 * RetroradioController.h
 *
 *  Created on: 21.10.2017
 *      Author: joe
 */

#ifndef SRC_RETRORADIOCONTROLLER_H_
#define SRC_RETRORADIOCONTROLLER_H_

#include "RemoteController.h"

#include <glib.h>

#include "RetroradioControllerConfiguration.h"
#include "AudioController.h"
#include "RemoteController.h"
#include "GPIOController.h"
#include "PowerStateMachine.h"
#include "ConnObserverFile.h"
#include "SoundCardSetup.h"
#include "RetroradioPersistentState.h"

using namespace GenericEmbeddedUtils;


namespace retroradio_controller
{

class RetroradioController : public RemoteController::IRemoteControllerListener,
	ConnObserverFile::Listener, AudioController::IStateListener, GPIOController::IBtnListener,
	SoundCardSetup::ISoundCardSetupListener
{

private:
	static RetroradioController *instance;

	int returnCode;

	GMainLoop *mainloop;

	RetroradioControllerConfiguration *configuration;

	AudioController *audioController;

	RemoteController *remoteController;

	GPIOController *gpioController;

	PowerStateMachine *stateMachine;

	ConnObserverFile *connObserver;

	SoundCardSetup *soundCardSetupController;

	RetroradioPersistentState *persistentState;

	static gboolean UnixSignalHandler(gpointer user_data);

	RetroradioController();

	virtual ~RetroradioController();

public:

	//RemoteController::IRemoteControllerListener
	virtual void OnCommandReceived(RemoteController::RemoteCommand cmd);

	//ConnObserverFile::Listener
	virtual void OnConnectionEstablished();

	virtual void OnConnectionLost();

	//AudioController::IStateListener
	virtual void OnStateChanged(AudioController::State newState);

	//GPIOController::IBtnListener

	virtual void OnPowerButtonReleased();

	//SoundCardSetup::ISoundCardSetupListener

	virtual void OnSoundCardReady();

	virtual void OnSoundCardDisabled();

	static RetroradioController *Instance();

	bool Init(int argc, char *argv[]);

	void DeInit();

	void Run();

	int GetReturnCode();

	void ShutdownImmediately(int exitCode);

	RetroradioControllerConfiguration* GetConfiguration()
	{
		return this->configuration;
	}

	AudioController* GetAudioController()
	{
		return this->audioController;
	}

	ConnObserverFile* GetConnObserver()
	{
		return this->connObserver;
	}

	RemoteController* GetRemoteController()
	{
		return this->remoteController;
	}

	PowerStateMachine* GetPowerStateMachine()
	{
		return this->stateMachine;
	}

	RetroradioPersistentState* GetPersistentState()
	{
		return this->persistentState;
	}

	GPIOController* GetGPIOController()
	{
		return this->gpioController;
	}

	SoundCardSetup* GetSndCardSetupController()
	{
		return this->soundCardSetupController;
	}
};

} //namespace retroradio_controller

#endif /* SRC_RETRORADIOCONTROLLER_H_ */
