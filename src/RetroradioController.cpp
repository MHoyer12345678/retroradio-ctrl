/*
 * RetroradioClient.cpp
 *
 *  Created on: 21.10.2017
 *      Author: joe
 */

#include "RetroradioController.h"

#include <cpp-app-utils/Logger.h>
#include <glib-unix.h>
#include <RetroradioControllerConfiguration.h>
#include <sysexits.h>

using namespace retroradio_controller;

RetroradioController *RetroradioController::instance=NULL;

RetroradioController::RetroradioController() :
		returnCode(0)
{
	this->mainloop=g_main_loop_new(NULL,FALSE);
	this->configuration=new RetroradioControllerConfiguration();
	this->persistentState=new RetroradioPersistentState(this->configuration);
	this->stateMachine=new PowerStateMachine();
	this->audioController=new AudioController(this, this->configuration);
	this->remoteController=new RemoteController(this, this->configuration);
	this->gpioController=new GPIOController(this);
	this->connObserver=new ConnObserverFile(this);
	this->soundCardSetupController=new SoundCardSetup(this);
}

RetroradioController::~RetroradioController()
{
	delete this->soundCardSetupController;
	delete this->connObserver;
	delete this->gpioController;
	delete this->remoteController;
	delete this->audioController;
	delete this->stateMachine;
	delete this->persistentState;
	delete this->configuration;
	g_main_loop_unref (this->mainloop);
}

bool RetroradioController::Init(int argc, char *argv[])
{
	if (!this->configuration->ParseArgsEarly(argc,argv,this->returnCode))
		return false;

	Logger::LogInfo("Starting retroradio controller %s",
			RetroradioControllerConfiguration::Version);

	if (!this->configuration->ReadConfigurationFile())
	{
		this->returnCode=EX_CONFIG;
		return false;
	}

    g_unix_signal_add(1, &UnixSignalHandler, this);
    g_unix_signal_add(2, &UnixSignalHandler, this);
    g_unix_signal_add(15, &UnixSignalHandler, this);

    this->persistentState->Init();

    if (!this->gpioController->Init())
    {
    	Logger::LogError("Failed to init gpio controller.");
    	return false;
    }

    if (!this->stateMachine->Init())
    {
    	Logger::LogError("Failed to init main state machine.");
    	return false;
    }

    if (!this->audioController->Init())
    {
    	Logger::LogError("Failed to start audio controller.");
    	return false;
    }

    if (!this->remoteController->Init())
	{
		Logger::LogError("Failed to start remote controller.");
		return false;
	}

	if (!this->connObserver->Init())
	{
		Logger::LogError("Failed to start wlan connection observer.");
		return false;
	}

	if (!this->soundCardSetupController->Init())
	{
		Logger::LogError("Failed to start sound card setup controller.");
		return false;
	}

    Logger::LogDebug("RetroradioController::Init -> Initialized Retroradio Controller");

    this->stateMachine->KickOff();

    Logger::LogDebug("RetroradioController::Init -> Kicked off main state machine");

    return true;
}

void RetroradioController::DeInit()
{
	this->soundCardSetupController->DeInit();
	this->connObserver->DeInit();
	this->audioController->DeInit();
	this->stateMachine->DeInit();
	this->remoteController->DeInit();
	this->gpioController->DeInit();

	delete this;
	RetroradioController::instance=NULL;

	Logger::LogDebug("RetroradioController::DeInit -> Deinitialized Retroradio Controller");
}

RetroradioController *RetroradioController::Instance()
{
	if (RetroradioController::instance==NULL)
		RetroradioController::instance=new RetroradioController();

	return RetroradioController::instance;
}

gboolean RetroradioController::UnixSignalHandler(gpointer user_data)
{
	RetroradioController *instance=(RetroradioController *)user_data;
	g_main_loop_quit(instance->mainloop);
	return TRUE;
}

void RetroradioController::Run()
{
	Logger::LogDebug("RetroradioController::Run -> Going to enter retroradio controller main loop.");
	g_main_loop_run(this->mainloop);
	Logger::LogDebug("RetroradioController::Run -> Retroradio main loop left. Shutting down.");
}

void RetroradioController::OnCommandReceived(
		RemoteControllerProfiles::RemoteCommand cmd)
{
	Logger::LogDebug("RetroradioController::OnCommandReceived -> Received IR command: %d", cmd);

	if (cmd==RemoteControllerProfiles::CMD_POWER)
	{
		this->stateMachine->OnIRPowerCommandReceived();
		return;
	}

	//don't send any command further when we are in standby
	if (!this->stateMachine->IsPowered())
		return;

	if (cmd==RemoteControllerProfiles::CMD_VOL_UP)
		this->audioController->VolumeUp();
	else if (cmd==RemoteControllerProfiles::CMD_VOL_DOWN)
		this->audioController->VolumeDown();
	else if (cmd==RemoteControllerProfiles::CMD_MUTE)
		this->audioController->ToggleMute();

	//don't send next/prev/src command further when we are just resuming from standby
	if (!this->stateMachine->IsActive())
		return;

	if (cmd==RemoteControllerProfiles::CMD_NEXT)
		this->audioController->TriggerSourceNextPressed();
	else if (cmd==RemoteControllerProfiles::CMD_PREV)
		this->audioController->TriggerSourcePrevPressed();
	else if (cmd==RemoteControllerProfiles::CMD_SRC_NEXT)
		this->audioController->ChangeToNextSource();
	else if (cmd>=RemoteControllerProfiles::CMD_FAV0 && cmd<=RemoteControllerProfiles::CMD_FAV9)
	{
		AbstractAudioSource::FavoriteT fav;
		fav=(AbstractAudioSource::FavoriteT)((cmd-RemoteControllerProfiles::CMD_FAV0)+AbstractAudioSource::FAV0);
		this->audioController->TriggerFavPressed(fav);
	}
}

void RetroradioController::OnSoundCardReady()
{
    Logger::LogDebug("RetroradioController::OnSoundCardReady -> Sound card setup controller signaled sound card setup done.");
    this->stateMachine->OnSoundCardReady();
}

void RetroradioController::OnSoundCardDisabled()
{
    Logger::LogDebug("RetroradioController::OnSoundCardReady -> Sound card setup controller signaled disappeared sound card.");
    this->stateMachine->OnSoundCardDisabled();
}

void RetroradioController::ShutdownImmediately(int exitCode)
{
	this->returnCode=exitCode;
	g_main_loop_quit(this->mainloop);
}

int RetroradioController::GetReturnCode()
{
	return this->returnCode;
}

void RetroradioController::OnConnectionEstablished()
{
	Logger::LogDebug("RetroradioController::OnConnectionEstablished - Connection to internet established.");
	this->stateMachine->OnConnectionEstablished();
}

void RetroradioController::OnConnectionLost()
{
	Logger::LogDebug("RetroradioController::OnConnectionLost - Connection to internet lost.");
	this->stateMachine->OnConnectionLost();
}

void RetroradioController::OnStateChanged(AudioController::State newState)
{
	this->stateMachine->OnAudioControllerStateChanged(newState);
}

void RetroradioController::OnPowerButtonReleased()
{
	this->stateMachine->OnPowerBtnPressed();
}
