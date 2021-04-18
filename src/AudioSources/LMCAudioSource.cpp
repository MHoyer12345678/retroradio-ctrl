/*
 * LMCAudioSource.cpp
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#include "LMCAudioSource.h"

#include <cpp-app-utils/Logger.h>

#define LMC_CONFIG_GROUP 				"LMC Source"
#define LMC_DEFAULT_ALSA_MIXER_NAME		"lmc_vol"
#define LMC_DEFAULT_SOUND_CARD_NAME		"default"

using namespace CppAppUtils;

using namespace retroradio_controller;

LMCAudioSource::LMCAudioSource(const char *srcName, AbstractAudioSource *predecessor,
		IAudioSourceStateListener *srcListener) :
		AbstractAudioSource(srcName, predecessor, srcListener)
{

}

LMCAudioSource::~LMCAudioSource()
{
}

bool LMCAudioSource::Init()
{
	if (!AbstractAudioSource::Init())
		return false;
	Logger::LogDebug("LMCAudioSource::Init - Initializing LMC Audio Source %s.", this->GetName());
	return true;
}

const char* LMCAudioSource::GetConfigGroupName()
{
	return LMC_CONFIG_GROUP;
}

const char* LMCAudioSource::GetDefaultAlsaMixerName()
{
	return LMC_DEFAULT_ALSA_MIXER_NAME;
}

const char* LMCAudioSource::GetDefaultSoundCardName()
{
	return LMC_DEFAULT_SOUND_CARD_NAME;
}
