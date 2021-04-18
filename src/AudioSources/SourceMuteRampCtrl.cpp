/*
 * SourceMuteRampCtrl.cpp
 *
 *  Created on: 24.12.2019
 *      Author: joe
 */

#include "SourceMuteRampCtrl.h"

#include "cpp-app-utils/Logger.h"

using namespace retroradio_controller;
using namespace CppAppUtils;

#define NUMBER_VOL_STEPS	20

const int SourceMuteRampCtrl::RAMP_STEP_DELAY_MS[]={15,30,45};

SourceMuteRampCtrl::SourceMuteRampCtrl(IMuteRampCtrlListener* listener) :
		BasicMixerControl(),
		listener(listener),
		state(_NOT_SET),
		timerId(-1),
		curVolReal(0),
		curRampSpeed(SLOW),
		volumeStep(1)
{
}

SourceMuteRampCtrl::~SourceMuteRampCtrl()
{
}

bool SourceMuteRampCtrl::Init(const char* cardName, const char* mixerName)
{
	if (!BasicMixerControl::Init(cardName, mixerName))
		return false;

	Logger::LogDebug("SourceMuteRampCtrl::Init - Initializing source mute ramp. Card: %s, Mixer: %s", cardName, mixerName);

	this->state=IDLE;
	this->curVolReal=rangeMin;
	this->SetVolumeReal(this->rangeMin);
	this->curRampSpeed=SLOW;

	this->volumeStep=(this->rangeMax-this->rangeMin)/NUMBER_VOL_STEPS;
	if (this->volumeStep==0)
		this->volumeStep=1;

	return true;
}

void SourceMuteRampCtrl::DeInit()
{
}

void SourceMuteRampCtrl::MuteAsync(RampSpeed speed)
{
	if (this->state==_NOT_SET) return;

	if (this->state!=IDLE)
	{
		// in case we are in the middle of an unmute ramp, just change the state to muting. The rest is already set up.
		if (this->state==UNMUTING)
			this->state=MUTING;

		if (this->curRampSpeed==speed) return;
		// speed is different, change timer in this case
	}
	else
	{
		//update from actual volume set currently
		this->curVolReal=this->GetVolumeReal();
		this->state=MUTING;
		Logger::LogDebug("SourceMuteRampCtrl::MuteAsync - Starting a mute ramp for mixer %s. Current volume: %ld",
				this->mixerName, this->curVolReal);
	}

	this->SetupRampTimer(speed);
}

void SourceMuteRampCtrl::UnmuteAsync(RampSpeed speed)
{
	if (this->state==_NOT_SET) return;

	if (this->state!=IDLE)
	{
		// in case we are in the middle of an unmute ramp, just change the state to muting. The rest is already set up.
		if (this->state==MUTING)
			this->state=UNMUTING;

		if (this->curRampSpeed==speed) return;
		// speed is different, change timer in this case
	}
	else
	{
		//update from actual volume set currently
		this->curVolReal=this->GetVolumeReal();
		this->state=UNMUTING;
		Logger::LogDebug("SourceMuteRampCtrl::UnMuteAsync - Starting a unmute ramp for mixer %s. Current volume: %ld",
				this->mixerName, this->curVolReal);
	}

	this->SetupRampTimer(speed);
}

void SourceMuteRampCtrl::SetupRampTimer(RampSpeed speed)
{
	this->curRampSpeed=speed;
	if (this->timerId!=-1)
		this->CleanUpTimer();

	this->timerId=g_timeout_add(RAMP_STEP_DELAY_MS[speed], SourceMuteRampCtrl::OnRampTimerElapsed, this);
}

void SourceMuteRampCtrl::StopOperation()
{
	if (this->timerId==-1)
		return;

	Logger::LogDebug("SourceMuteRampCtrl::StopOperation - Canceling current mute operation for mixer %s. Current volume: %ld",
			this->mixerName, this->curVolReal);
	this->RampFinished(false);
}

void SourceMuteRampCtrl::CleanUpTimer()
{
	Logger::LogDebug("SourceMuteRampCtrl::CleanUpTimer - Ramp done for mixer %s. Cleaning up ramp timer", this->mixerName);
	g_source_remove(this->timerId);
	this->timerId=-1;
}

gboolean SourceMuteRampCtrl::OnRampTimerElapsed(gpointer user_data)
{
	SourceMuteRampCtrl *instance=(SourceMuteRampCtrl *)user_data;
	gboolean result=TRUE;

	switch(instance->state)
	{
	case IDLE:
		instance->CleanUpTimer();
		result=FALSE;
		break;

	case MUTING:
		instance->curVolReal-=instance->volumeStep;
		if (instance->curVolReal<instance->rangeMin)
			instance->curVolReal=instance->rangeMin;
		instance->SetVolumeReal(instance->curVolReal);
		if (instance->curVolReal==0)
		{
			instance->RampFinished(false);
			result=FALSE;
		}
		break;

	case UNMUTING:
		instance->curVolReal+=instance->volumeStep;
		if (instance->curVolReal>instance->rangeMax)
			instance->curVolReal=instance->rangeMax;
		instance->SetVolumeReal(instance->curVolReal);
		if (instance->curVolReal==instance->rangeMax)
		{
			instance->RampFinished(false);
			result=FALSE;
		}
		break;

	case _NOT_SET:
		result=FALSE;
		break;

	}

	return result;
}

void SourceMuteRampCtrl::RampFinished(bool canceled)
{
	this->CleanUpTimer();
	this->state=IDLE;
	if (this->listener!=NULL)
		this->listener->OnRampFinished(canceled);
}
