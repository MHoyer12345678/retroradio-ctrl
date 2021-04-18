/*
 * DLNAAudioSource.cpp
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#include "DLNAAudioSource.h"

#include <cpp-app-utils/Logger.h>

using namespace CppAppUtils;

using namespace retroradio_controller;

#define DLNA_CONFIG_GROUP 				"DLNA Source"
#define DLNA_DEFAULT_ALSA_MIXER_NAME	"dlna_vol"
#define DLNA_DEFAULT_SOUND_CARD_NAME	"default"

DLNAAudioSource::DLNAAudioSource(const char *srcName, AbstractAudioSource *predecessor,
		IAudioSourceStateListener *srcListener) :
		AbstractAudioSource(srcName, predecessor, srcListener)
{

}

DLNAAudioSource::~DLNAAudioSource()
{
}

bool DLNAAudioSource::Init()
{
	if (!AbstractAudioSource::Init())
		return false;

	Logger::LogDebug("DLNAAudioSource::Init - Initializing DLNA Audio Source %s.", this->GetName());
	return true;
}

const char* DLNAAudioSource::GetConfigGroupName()
{
	return DLNA_CONFIG_GROUP;
}

const char* DLNAAudioSource::GetDefaultAlsaMixerName()
{
	return DLNA_DEFAULT_ALSA_MIXER_NAME;
}

const char* DLNAAudioSource::GetDefaultSoundCardName()
{
	return DLNA_DEFAULT_SOUND_CARD_NAME;
}
