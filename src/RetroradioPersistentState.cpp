/*
 * RetroradioPersistentState.cpp
 *
 *  Created on: 01.02.2020
 *      Author: joe
 */

#include "RetroradioPersistentState.h"

#include "RetroradioController.h"
#include "AudioSources/RetroradioAudioSourceList.h"
#include "MainVolumeControl.h"

#include <stdlib.h>
#include <unistd.h>

using namespace retroradio_controller;

#define STATE_START_TAG_0 	'R'
#define STATE_START_TAG_1 	'E'
#define STATE_END_TAG_0 	'T'
#define STATE_END_TAG_1 	'R'

#define POWER_INACTIVE_CHAR	'I'
#define POWER_ACTIVE_CHAR	'A'

RetroradioPersistentState::RetroradioPersistentState(Configuration *configuration) :
		AbstractPersistentState(configuration)
{
}

RetroradioPersistentState::~RetroradioPersistentState()
{
}

bool RetroradioPersistentState::DoWriteDataSet(int fd)
{
	size_t size=sizeof(PersistentState);

	if (write(fd, &this->state,size)!=size)
		return false;

	return true;
}

bool RetroradioPersistentState::DoReadDataSet(int fd)
{
	size_t size=sizeof(PersistentState);

	if (read(fd, &this->state,size)!=size)
		return false;

	if (!CheckDataSetSignature())	return false;
	if (!CheckPowerValue())	return false;
	if (!CheckMasterVolumeValue())	return false;
	if (!CheckCurrentSrcId())	return false;
	//no sanity check track number ...

	return true;
}

bool RetroradioPersistentState::CheckDataSetSignature()
{
	return this->state.stateStartTag[0]==STATE_START_TAG_0 && this->state.stateStartTag[1]==STATE_START_TAG_1 &&
			this->state.stateEndTag[0]==STATE_END_TAG_0 && this->state.stateEndTag[1]==STATE_END_TAG_1;
}

bool RetroradioPersistentState::CheckPowerValue()
{
	return this->state.powerState==POWER_ACTIVE_CHAR || this->state.powerState==POWER_INACTIVE_CHAR;
}

bool RetroradioPersistentState::CheckMasterVolumeValue()
{
	return this->state.masterVolume>=MASTER_VOL_MIN && this->state.masterVolume<=MASTER_VOL_MAX;
}

bool RetroradioPersistentState::CheckCurrentSrcId()
{
	return RetroradioAudioSourceList::IsSourceIdKnown(this->state.currentSrcId);
}

void RetroradioPersistentState::DoResetToDefault()
{
	this->state.stateStartTag[0]	= STATE_START_TAG_0;
	this->state.stateStartTag[1]	= STATE_START_TAG_1;
	this->state.stateEndTag[0]		= STATE_END_TAG_0;
	this->state.stateEndTag[1]		= STATE_END_TAG_1;

	this->state.masterVolume		= MASTER_VOLUME_DEFAULT;
	this->state.powerState			= POWER_ACTIVE_CHAR;
	strncpy(this->state.currentSrcId, RetroradioAudioSourceList::GetDefaultSourceId(),
			sizeof(this->state.currentSrcId)-1);
	this->state.mpdCurrentTrackNr 	= 0;
}

void RetroradioPersistentState::SetPowerStateActive(bool isActive)
{
	this->state.powerState = isActive ? POWER_ACTIVE_CHAR : POWER_INACTIVE_CHAR;
	this->CommitImmediately();
}

bool RetroradioPersistentState::IsPowerStateActive()
{
	return this->state.powerState==POWER_ACTIVE_CHAR;
}

void RetroradioPersistentState::SetMasterVolume(
		long masterVolume)
{
	this->state.masterVolume=masterVolume;
	this->CommitDelayed();
}

long RetroradioPersistentState::GetMasterVolume()
{
	return this->state.masterVolume;
}

void RetroradioPersistentState::SetCurrentSrcId(const char *sourceId)
{
	strncpy(this->state.currentSrcId, sourceId, sizeof(this->state.currentSrcId)-1);
	this->CommitDelayed();
}

const char *RetroradioPersistentState::GetCurrentSrcId()
{
	return this->state.currentSrcId;
}

void RetroradioPersistentState::SetMPDCurrentTrackNr(
		unsigned int currentTrackNr)
{
	this->state.mpdCurrentTrackNr=currentTrackNr;
	this->CommitDelayed();
}

unsigned int RetroradioPersistentState::GetMPDCurrentTrackNr()
{
	return this->state.mpdCurrentTrackNr;
}
