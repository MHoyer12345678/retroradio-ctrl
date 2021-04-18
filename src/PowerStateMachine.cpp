/*
 * MainStateMachine.cpp
 *
 *  Created on: 22.11.2019
 *      Author: joe
 */

#include "PowerStateMachine.h"

#include "RetroradioController.h"

#include <cpp-app-utils/Logger.h>

#include <glib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace CppAppUtils;

#define STARTUP_POLL_TIMEOUT_MS		250

#define HANDOVER_FILE		"/run/system_start_complete"


namespace retroradio_controller {

PowerStateMachine::PowerStateMachine() :
		state(_NOT_INITIALIZED),
		need2ReOpenSoundDevices(true)
{

}

PowerStateMachine::~PowerStateMachine()
{

}

bool PowerStateMachine::Init()
{
	Logger::LogDebug("PowerStateMachine::Init -> Initializing main state machine.");

	return true;
}

void PowerStateMachine::DeInit()
{
	this->state=_NOT_INITIALIZED;
	Logger::LogDebug("PowerStateMachine::DeInit -> Deinitialized main state machine.");
}

void PowerStateMachine::EnterStartingUp()
{
	AudioController *ac=RetroradioController::Instance()->GetAudioController();

	Logger::LogDebug("PowerStateMachine::EnterStartingUp -> Retroradio starting up. Waiting for sources to finish booting.");
	this->state=STARTING_UP;

	// all sources already up and ready for activation?
	if (ac->CheckSourcesStartupState())
		this->OnStartupFinished();
	else
		g_timeout_add(STARTUP_POLL_TIMEOUT_MS, PowerStateMachine::OnStartupPollTimerElapsed, this);
}

gboolean PowerStateMachine::OnStartupPollTimerElapsed(gpointer user_data)
{
	PowerStateMachine *instance=(PowerStateMachine *)user_data;
	AudioController *ac=RetroradioController::Instance()->GetAudioController();

	if (ac->CheckSourcesStartupState())
	{
		instance->OnStartupFinished();
		return FALSE;
	}

	return TRUE;
}

void PowerStateMachine::OnStartupFinished()
{
	Logger::LogDebug("PowerStateMachine::OnStartupFinished -> Startup for sources finished.");
	this->DoEarlyLateHandover();
	if (RetroradioController::Instance()->GetConnObserver()->IsConnected())
	{
		if (RetroradioController::Instance()->GetPersistentState()->IsPowerStateActive())
			this->EnterActivating();
		else
			this->EnterStandby();
	}
	else
		this->EnterWaitingForWifiAndSndCard();
}

void PowerStateMachine::DoEarlyLateHandover()
{
	struct stat r;
	int f,cntr=10;

	Logger::LogDebug("PowerStateMachine::DoEarlyLateHandover -> Doing hand over handshake with early process.");


	//create file /run/system_start_complete to inform rr-early-setup process that system start is complete
	f=open(HANDOVER_FILE, O_CREAT, 0666);
	close(f);

	//the process exists and removes the file directly before
	//when we detect the file removal, we can go on controling LEDs without any influence of the early service
	while(stat(HANDOVER_FILE, &r)==0 && cntr!=0)
	{
		Logger::LogDebug("PowerStateMachine::DoEarlyLateHandover -> Waiting for early process to finish.");
		//give the early process 100ms to exit
		usleep(100000);
		cntr--;
	}

	if (cntr==0)
		Logger::LogError("Early setup process did not remove file /run/system_start_complete within 1 sec.");
}

void PowerStateMachine::EnterWaitingForWifiAndSndCard()
{
	Logger::LogDebug("PowerStateMachine::EnterWaitingForWifiAndSndCard -> Waiting for wifi and/or snd card.");
	this->state=WAITING_FOR_WIFI_AND_SNDCARD;

	RetroradioController::Instance()->GetGPIOController()->SetPowerLedMode(GPIOController::WAITING_FOR_WIFI);

	//check if connection is up again and sound card is ready already -> activate again
	if (this->IsReadyForActivation())
		this->EnterActivating();
}

void PowerStateMachine::EnterConnectionLoss()
{
	AudioController *ac=RetroradioController::Instance()->GetAudioController();
	Logger::LogDebug("PowerStateMachine::EnterConnectionLoss -> Lost connection. Deactivating radio services.");
	ac->DeactivateController(true);
	this->state=CONNECTION_LOSS;
	if (ac->GetState()==AudioController::DEACTIVATED)
		this->EnterWaitingForWifiAndSndCard();
}

void PowerStateMachine::EnterSndCardDisappeared()
{
	AudioController *ac=RetroradioController::Instance()->GetAudioController();
	Logger::LogDebug("PowerStateMachine::EnterSndCardDisappeared -> Sound card disappeared. Deactivating radio services.");
	//sound card gone -> no mute ramp possible
	ac->DeactivateController(false);
	this->state=SNDCARD_DISAPPEARED;
	if (ac->GetState()==AudioController::DEACTIVATED)
		this->EnterWaitingForWifiAndSndCard();
}

void PowerStateMachine::EnterActivating()
{
	AudioController *ac=RetroradioController::Instance()->GetAudioController();
	Logger::LogDebug("PowerStateMachine::EnterActivating -> Activating radio services.");
	this->SetPowerEnabled(true);
	RetroradioController::Instance()->GetPersistentState()->SetPowerStateActive(true);
	ac->ActivateAudioController(this->need2ReOpenSoundDevices);
	this->need2ReOpenSoundDevices=false;
	this->state=ACTIVATING;
	if (ac->GetState()==AudioController::ACTIVATED)
		this->EnterActive();
}

void PowerStateMachine::EnterDeactivating()
{
	AudioController *ac=RetroradioController::Instance()->GetAudioController();
	Logger::LogDebug("PowerStateMachine::EnterDeactivating -> Deactivating radio services.");
	RetroradioController::Instance()->GetPersistentState()->SetPowerStateActive(false);
	ac->DeactivateController(true);
	this->state=DEACTIVATING;
	if (ac->GetState()==AudioController::DEACTIVATED)
		this->EnterStandby();
}

void PowerStateMachine::EnterActive()
{
	Logger::LogDebug("PowerStateMachine::EnterActive -> Radio services activated.");
	this->state=ACTIVE;
}

void PowerStateMachine::EnterStandby()
{
	Logger::LogDebug("PowerStateMachine::EnterStandby -> Entering state standby.");
	this->SetPowerEnabled(false);
	this->state=STANDBY;
}

void PowerStateMachine::OnPowerBtnPressed()
{
	Logger::LogDebug("PowerStateMachine::OnPowerBtnPressed -> Received a \"Power Button released\" event.");
	this->DoProcessPowerBtnEvent();
}

void PowerStateMachine::OnIRPowerCommandReceived()
{
	Logger::LogDebug("PowerStateMachine::OnIRPowerCommandReceived -> Received a Power IR Command Trigger.");
	this->DoProcessPowerBtnEvent();
}

void PowerStateMachine::DoProcessPowerBtnEvent()
{
	switch(state)
	{
	case STANDBY:
		if (this->IsReadyForActivation())
			this->EnterActivating();
		else
			this->EnterWaitingForWifiAndSndCard();
		break;
	case WAITING_FOR_WIFI_AND_SNDCARD:
		this->EnterStandby();
		break;
	case CONNECTION_LOSS:
	case SNDCARD_DISAPPEARED:
		//Standby pressed while deactivating due to connection loss or lost snd card -> make state deactivating to finally go into standby
		this->state=DEACTIVATING;
		break;
	case ACTIVATING:
	case ACTIVE:
		this->EnterDeactivating();
		break;
	case STARTING_UP:
	case DEACTIVATING:
		break;
	}
}

void PowerStateMachine::OnConnectionLost()
{
	if (this->state!=STANDBY)
		this->EnterConnectionLoss();
}

void PowerStateMachine::OnConnectionEstablished()
{
	if (this->state==WAITING_FOR_WIFI_AND_SNDCARD &&
			this->IsReadyForActivation())
		this->EnterActivating();
}

void PowerStateMachine::OnSoundCardDisabled()
{
	//mixer automatically detecting lost sound card via POLLERR. They are deinitializing themselves.
	//Need to flag that we need to initialize them again on next activation
	this->need2ReOpenSoundDevices=true;
	if (this->state!=STANDBY)
		this->EnterSndCardDisappeared();
}

void PowerStateMachine::OnSoundCardReady()
{
	if (this->state==WAITING_FOR_WIFI_AND_SNDCARD &&
			this->IsReadyForActivation())
		this->EnterActivating();
}

void PowerStateMachine::OnAudioControllerStateChanged(
		AudioController::State newState)
{
	AudioController *ac=RetroradioController::Instance()->GetAudioController();
	AudioController::State acState=ac->GetState();

	Logger::LogDebug("PowerStateMachine::OnAudioControllerStateChanged -> Power state machine received state"
			"change event from audio controller. New State: %s",AudioController::StateNames[newState]);

	if (this->state==ACTIVATING && acState==AudioController::ACTIVATED)
		this->EnterActive();
	else if (this->state==DEACTIVATING && acState==AudioController::DEACTIVATED)
		this->EnterStandby();
	else if ((this->state==CONNECTION_LOSS || this->state==SNDCARD_DISAPPEARED)
			&& acState==AudioController::DEACTIVATED)
		this->EnterWaitingForWifiAndSndCard();
}

void PowerStateMachine::KickOff()
{
	Logger::LogDebug("PowerStateMachine::KickOff -> Starting main state machine.");
	this->EnterStartingUp();
}

void PowerStateMachine::SetPowerEnabled(bool enabled)
{
	Logger::LogDebug("PowerStateMachine::SetPowerEnabled -> %s amp power.", enabled ? "Activating" : "DeActivating");
	RetroradioController::Instance()->GetGPIOController()->SetAmpEnabled(enabled);
	if (enabled)
		RetroradioController::Instance()->GetGPIOController()->SetPowerLedMode(GPIOController::POWER_ON);
	else
		RetroradioController::Instance()->GetGPIOController()->SetPowerLedMode(GPIOController::POWER_OFF);
}

bool PowerStateMachine::IsPowered()
{
	return this->state != STANDBY && this->state != DEACTIVATING;
}

bool PowerStateMachine::IsActive()
{
	return this->state==ACTIVE;
}

bool PowerStateMachine::IsReadyForActivation()
{
	return RetroradioController::Instance()->GetConnObserver()->IsConnected() &&
			RetroradioController::Instance()->GetSndCardSetupController()->IsCardAvailable();
}

} /* namespace retroradiocontroller */
