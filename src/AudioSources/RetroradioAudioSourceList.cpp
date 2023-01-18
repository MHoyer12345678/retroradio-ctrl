/*
 * AudioSourceFactory.cpp
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#include "AudioSources/RetroradioAudioSourceList.h"

#include <stdlib.h>

#include <cpp-app-utils/Logger.h>

#include "RetroradioController.h"

#include "AudioSources/MPDAudioSource.h"
#include "AudioSources/LMCAudioSource.h"
#include "AudioSources/DLNAAudioSource.h"

using namespace CppAppUtils;

using namespace retroradio_controller;

const char *RetroradioAudioSourceList::MPD_SOURCE="retroradio_mpd_source";

const char *RetroradioAudioSourceList::DLNA_SOURCE="retroradio_dlna_source";

const char *RetroradioAudioSourceList::LMC_SOURCE="retroradio_lmc_source";


RetroradioAudioSourceList::RetroradioAudioSourceList(AbstractAudioSource::IAudioSourceListener *srcListener,
		Configuration *configuration) :	srcListener(srcListener)
{
	this->FillList(configuration);
	this->currentAudioSource=this->audioSources;
}

RetroradioAudioSourceList::~RetroradioAudioSourceList()
{
	AbstractAudioSource *nextSrc;

	while (this->audioSources != NULL)
	{
		nextSrc=this->audioSources->GetSuccessor();
		delete this->audioSources;
		this->audioSources=nextSrc;
	}
}

bool RetroradioAudioSourceList::Init()
{
	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		if (!itr->Init()) return false;

	return true;
}

void RetroradioAudioSourceList::DeInit()
{
	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		itr->DeInit();
}

void RetroradioAudioSourceList::FillList(Configuration *configuration)
{
	AbstractAudioSource *nextSrc;
	this->audioSources=CreateMPDAudioSource(MPD_SOURCE,NULL);
	nextSrc=CreateDLNAAudioSource(DLNA_SOURCE, this->audioSources);
	CreateLMCAudioSource(LMC_SOURCE,nextSrc);

	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		configuration->AddConfigurationModule(itr);

}

AbstractAudioSource* RetroradioAudioSourceList::CreateMPDAudioSource(const char *srcName, AbstractAudioSource *predecessor)
{
	return new MPDAudioSource(srcName, predecessor, this->srcListener);
}

AbstractAudioSource* RetroradioAudioSourceList::CreateDLNAAudioSource(const char *srcName, AbstractAudioSource *predecessor)
{
	return new DLNAAudioSource(srcName, predecessor, this->srcListener);
}

AbstractAudioSource* RetroradioAudioSourceList::CreateLMCAudioSource(const char *srcName, AbstractAudioSource *predecessor)
{
	return new LMCAudioSource(srcName, predecessor, this->srcListener);
}

void RetroradioAudioSourceList::ChangeToNextSource()
{
	this->previousAudioSource=this->currentAudioSource;
	this->currentAudioSource=this->currentAudioSource->GetSuccessor();
	if (this->currentAudioSource==NULL)
		this->currentAudioSource=this->audioSources;
}

AbstractAudioSource* RetroradioAudioSourceList::GetCurrentSource()
{
	return this->currentAudioSource;
}

AbstractAudioSource* RetroradioAudioSourceList::GetPreviousSource()
{
	return this->previousAudioSource;
}

void RetroradioAudioSourceList::DeactivateAll()
{
	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		itr->DeActivate();
}

void RetroradioAudioSourceList::ActivateAll(bool need2ReOpenSoundDevices)
{
	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		itr->Activate(need2ReOpenSoundDevices);
}

bool RetroradioAudioSourceList::AreAllActivated()
{
	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		if (!itr->IsActive()) return false;

	return true;
}

bool RetroradioAudioSourceList::AreAllDeactivated()
{
	for (AbstractAudioSource *itr=this->audioSources;itr != NULL; itr=itr->GetSuccessor())
		if (itr->IsActive()) return false;

	return true;
}

bool RetroradioAudioSourceList::IsSourceIdKnown(const char* srcId)
{
	if (strstr(srcId, RetroradioAudioSourceList::MPD_SOURCE)==0)
		return true;
	if (strstr(srcId, RetroradioAudioSourceList::LMC_SOURCE)==0)
		return true;
	if (strstr(srcId, RetroradioAudioSourceList::DLNA_SOURCE)==0)
		return true;

	return false;
}

const char* RetroradioAudioSourceList::GetDefaultSourceId()
{
	return RetroradioAudioSourceList::MPD_SOURCE;
}

AbstractAudioSource *RetroradioAudioSourceList::GetIterator()
{
	return this->audioSources;
}
