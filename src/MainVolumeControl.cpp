/*
 * MainVolumeControl.cpp
 *
 *  Created on: 23.12.2019
 *      Author: joe
 */

#include "MainVolumeControl.h"

#include "cpp-app-utils/Logger.h"
#include "RetroradioController.h"

using namespace CppAppUtils;

#define MAIN_VOL_CONFIG_GROUP		"MainVolumeControl"

#define	CONFIG_TAG_MIXER_NAME		"MainMixerName"
#define DEFAULT_MAIN_MIXER_NAME		"Speaker"
#define	CONFIG_TAG_SNDCARD_NAME		"SndCardName"
#define DEFAULT_SND_CARD_NAME		"default"

#define NUMBER_VOL_STEPS			100

namespace retroradio_controller {

MainVolumeControl::MainVolumeControl(Configuration *configuration) :
		BasicMixerControl(),
		currentVolReal(0),
		sndCardName(NULL),
		mainMixerName(NULL),
		volumeStep(0)

{
	configuration->AddConfigurationModule(this);
}

MainVolumeControl::~MainVolumeControl()
{
	if (this->mainMixerName!=NULL)
		free(this->mainMixerName);
	if (this->sndCardName!=NULL)
		free(this->sndCardName);
}

bool MainVolumeControl::Init()
{
	if (!BasicMixerControl::Init(this->ConfigGetCardName(), this->ConfigGetMixerName()))
		return false;

	this->currentVolReal=this->PercentToMixerVol(RetroradioController::Instance()->GetPersistentState()->GetMasterVolume());
	this->SetVolumeReal(this->currentVolReal);

	this->volumeStep=(this->rangeMax-this->rangeMin)/NUMBER_VOL_STEPS;
	if (this->volumeStep==0)
		this->volumeStep=1;

	return true;
}


void MainVolumeControl::VolumeUp()
{
	this->currentVolReal+=this->volumeStep;
	if (this->currentVolReal > this->rangeMax)
		this->currentVolReal = this->rangeMax;

	Logger::LogDebug("MainVolumeControl::VolumeUp -> Audio Controller requested to increase volume to %ld.", this->currentVolReal);

	RetroradioController::Instance()->GetPersistentState()->SetMasterVolume(this->currentVolReal);
	BasicMixerControl::SetVolumeReal(this->currentVolReal);
}

void MainVolumeControl::VolumeDown()
{
	this->currentVolReal-=this->volumeStep;
	if (this->currentVolReal < this->rangeMin)
		this->currentVolReal = this->rangeMin;

	Logger::LogDebug("MainVolumeControl::VolumeDown -> Audio Controller requested to decrease volume to %ld.", this->currentVolReal);
	RetroradioController::Instance()->GetPersistentState()->SetMasterVolume(this->currentVolReal);
	BasicMixerControl::SetVolumeReal(this->currentVolReal);
}

void MainVolumeControl::OnMixerEvent(unsigned int mask)
{
	Logger::LogDebug("MainVolumeControl::OnMixerEvent -> Received mixer event. Mask: %d", mask);

	long curVolAlsaReal;
	curVolAlsaReal=this->GetVolumeReal();

	if (this->currentVolReal!=curVolAlsaReal)
	{
		Logger::LogDebug("MainVolumeControl::OnMixerEvent -> Mixer volume changed to %ld. Writing it again.", curVolAlsaReal);
		this->currentVolReal=curVolAlsaReal;
		//do not write 0 to file -> Not store mute state, 0 received in case of remove sound card
		if (this->currentVolReal != this->rangeMin)
			RetroradioController::Instance()->GetPersistentState()->SetMasterVolume(this->currentVolReal);
	}
}

void MainVolumeControl::Mute()
{
	Logger::LogDebug("MainVolumeControl::Mute -> Main volume control requested to mute.");
	BasicMixerControl::SetVolumeReal(this->rangeMin);
}

void MainVolumeControl::UnMute()
{
	Logger::LogDebug("MainVolumeControl::UnMute -> Main volume control requested to unmute.");
	BasicMixerControl::SetVolumeReal(this->currentVolReal);
}

const char* MainVolumeControl::ConfigGetMixerName()
{
	if (this->mainMixerName!=NULL)
		return this->mainMixerName;
	else
		return DEFAULT_MAIN_MIXER_NAME;
}

const char* MainVolumeControl::ConfigGetCardName()
{
	if (this->sndCardName!=NULL)
		return this->sndCardName;
	else
		return DEFAULT_SND_CARD_NAME;
}

bool MainVolumeControl::ParseConfigFileItem(GKeyFile* confFile, const char* group, const char* key)
{
	if (strcasecmp(group, MAIN_VOL_CONFIG_GROUP)!=0) return true;
	if (strcasecmp(key, CONFIG_TAG_SNDCARD_NAME)==0)
	{
		char *sndCard;
		if (Configuration::GetStringValueFromKey(confFile,key,group, &sndCard))
		{
			if (this->sndCardName!=NULL)
				free(this->sndCardName);
			this->sndCardName=sndCard;
		}
		else
			return false;
	}
	else if (strcasecmp(key, CONFIG_TAG_MIXER_NAME)==0)
	{
		char *mixer;
		if (Configuration::GetStringValueFromKey(confFile,key,group, &mixer))
		{
			if (this->mainMixerName!=NULL)
				free(this->mainMixerName);
			this->mainMixerName=mixer;
		}
		else
			return false;
	}

	return true;
}

bool MainVolumeControl::IsConfigFileGroupKnown(const char* group)
{
	return strcasecmp(group, MAIN_VOL_CONFIG_GROUP);
}

} /* namespace retroradio_controller */
