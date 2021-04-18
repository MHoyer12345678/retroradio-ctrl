/*
 * PowerBTNAndLEDControl.cpp
 *
 *  Created on: 10.03.2020
 *      Author: joe
 */

#include "GPIOController.h"

#include <string.h>

#include "cpp-app-utils/Logger.h"
#include "AudioSources/RetroradioAudioSourceList.h"

using namespace retroradio_controller;
using namespace CppAppUtils;

#define PBTN_GPIO_NR				7
#define PWR_BTN_EVENT_DELAY			100

#define POWER_LED_GPIO_NR			8
#define AMP_POWER_GPIO_NR			12

#define MPC_SRC_LED_GPIO_NR			13
#define DLNA_SRC_LED_GPIO_NR		19
#define LMC_SRC_LED_GPIO_NR			26

#define GPIO_EXPORT_TIMEOUT_MS		100

GPIOOutput::BlinkSequence WaitingForWIFIBinkSEQ(0, (const int []){500,500},2);


GPIOController::GPIOController(IBtnListener *btnListener) :
		btnListener(btnListener),
		btnEventDelayTimerSet(false)
{
	this->powerBtnGPIO=new GPIOInput(PBTN_GPIO_NR, GPIO_EXPORT_TIMEOUT_MS, this);
	this->powerLedGPIO=new GPIOOutput(POWER_LED_GPIO_NR, GPIO_EXPORT_TIMEOUT_MS, false);
	this->ampPowerGPIO=new GPIOOutput(AMP_POWER_GPIO_NR, GPIO_EXPORT_TIMEOUT_MS, false);

	this->CreateSourceGPIOArray();
}

GPIOController::~GPIOController()
{
	this->DeleteSourceGPIOArray();

	delete this->ampPowerGPIO;
	delete this->powerLedGPIO;
	delete this->powerBtnGPIO;
}

void GPIOController::CreateSourceGPIOArray()
{
	this->sourceLedGPIOs = new SourceLedGPIO[4];
	this->sourceLedGPIOs[0].SRC_ID=RetroradioAudioSourceList::MPD_SOURCE;
	this->sourceLedGPIOs[0].srcLedGPIO = new GPIOOutput(MPC_SRC_LED_GPIO_NR, GPIO_EXPORT_TIMEOUT_MS, false);
	this->sourceLedGPIOs[1].SRC_ID=RetroradioAudioSourceList::DLNA_SOURCE;
	this->sourceLedGPIOs[1].srcLedGPIO = new GPIOOutput(DLNA_SRC_LED_GPIO_NR, GPIO_EXPORT_TIMEOUT_MS, false);
	this->sourceLedGPIOs[2].SRC_ID=RetroradioAudioSourceList::LMC_SOURCE;
	this->sourceLedGPIOs[2].srcLedGPIO = new GPIOOutput(LMC_SRC_LED_GPIO_NR, GPIO_EXPORT_TIMEOUT_MS, false);
	this->sourceLedGPIOs[3].SRC_ID=NULL;
	this->sourceLedGPIOs[3].srcLedGPIO=NULL;
}

void GPIOController::DeleteSourceGPIOArray()
{
	for (int a=0; this->sourceLedGPIOs[a].srcLedGPIO!=NULL; a++)
		delete this->sourceLedGPIOs[a].srcLedGPIO;

	delete this->sourceLedGPIOs;
}

bool GPIOController::Init()
{
	Logger::LogDebug("GPIOController::Init - Initializing GPIO controller.");
	if (!this->ampPowerGPIO->Init())
	{
		Logger::LogDebug("Failed to initialize amp gpio (nr: %d)", AMP_POWER_GPIO_NR);
		return false;
	}
	if (!this->powerLedGPIO->Init())
	{
		Logger::LogDebug("Failed to initialize power led gpio (nr: %d)", POWER_LED_GPIO_NR);
		return false;
	}
	if (!this->powerBtnGPIO->Init())
	{
		Logger::LogDebug("Failed to initialize power btn gpio (nr: %d)", PBTN_GPIO_NR);
		return false;
	}

	for (int a=0; this->sourceLedGPIOs[a].srcLedGPIO!=NULL; a++)
	{
		if (!this->sourceLedGPIOs[a].srcLedGPIO->Init())
		{
			Logger::LogDebug("Failed to source led gpio for source: %s", this->sourceLedGPIOs[a].SRC_ID);
			return false;
		}
	}

	return true;
}

void GPIOController::DeInit()
{
}

void GPIOController::SetPowerLedMode(
		PowerLedMode powerLedMode)
{
	if (this->powerLedGPIO == NULL) return;

	if (powerLedMode==POWER_OFF)
		this->powerLedGPIO->SetModeConstantValue(false);
	else if (powerLedMode==POWER_ON)
		this->powerLedGPIO->SetModeConstantValue(true);
	else if (powerLedMode==WAITING_FOR_WIFI)
		this->powerLedGPIO->SetModeBlinking(&WaitingForWIFIBinkSEQ);
}

void GPIOController::SetSourceLedEnabled(
		const char* sourceID, bool enabled)
{
	for (int a=0; this->sourceLedGPIOs[a].srcLedGPIO!=NULL; a++)
	{
		if (strcmp(this->sourceLedGPIOs[a].SRC_ID, sourceID)==0)
		{
			this->sourceLedGPIOs[a].srcLedGPIO->SetModeConstantValue(enabled);
			break;
		}
	}
}

void GPIOController::DisableSourcesLeds()
{
	for (int a=0; this->sourceLedGPIOs[a].srcLedGPIO!=NULL; a++)
		this->sourceLedGPIOs[a].srcLedGPIO->SetModeConstantValue(false);
}

void GPIOController::SetAmpEnabled(bool enabled)
{
	if (this->ampPowerGPIO!=NULL)
		this->ampPowerGPIO->SetModeConstantValue(enabled);
}

void GPIOController::OnValueChanged(GPIOInput* gpio, bool value)
{
	//ignore events within the timer interval after the first event was received
	if (!this->btnEventDelayTimerSet)
	{
		g_timeout_add(PWR_BTN_EVENT_DELAY, GPIOController::OnPwrBtnEventDelayElapsed, this);
		this->btnEventDelayTimerSet=true;
	}
}

gboolean GPIOController::OnPwrBtnEventDelayElapsed(
		gpointer data)
{
	GPIOController *instance = (GPIOController *)data;
	instance->EvaluatePwrBtnStateAfterEventTimeout();
	instance->btnEventDelayTimerSet=false;

	return FALSE;
}

void GPIOController::EvaluatePwrBtnStateAfterEventTimeout()
{
	bool value=this->powerBtnGPIO->GetValue();
	Logger::LogDebug("GPIOController::EvaluatePwrBtnStateAfterEventTimeout - Received power button event. GPIO Value: %d", value);
	if (this->btnListener!=NULL)
	{
		if (value)
			this->btnListener->OnPowerButtonReleased();
		//else
		//	"PowerButtonPressed" event could be send here as well. Not needed currently
	}
}

int GPIOController::GetPollIntervalUs()
{
	return 0;
}
