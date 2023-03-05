/*
 * AudioSourceFactory.h
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_RETRORADIOAUDIOSOURCELIST_H_
#define SRC_AUDIOSOURCES_RETRORADIOAUDIOSOURCELIST_H_

#include "AudioSources/AbstractAudioSource.h"

namespace retroradio_controller
{

class RetroradioAudioSourceList
{

private:
	AbstractAudioSource *CreateMPDAudioSource(const char *srcName, AbstractAudioSource *predecessor);

	AbstractAudioSource *CreateDLNAAudioSource(const char *srcName, AbstractAudioSource *predecessor);

	AbstractAudioSource *CreateLMCAudioSource(const char *srcName, AbstractAudioSource *predecessor);

public:
	static const char *MPD_SOURCE;

	static const char *DLNA_SOURCE;

	static const char *LMC_SOURCE;

	static bool IsSourceIdKnown(const char *srcId);

	static const char *GetDefaultSourceId();

private:
	AbstractAudioSource *audioSources;

	AbstractAudioSource *currentAudioSource;

	AbstractAudioSource *scheduledAudioSource;

	AbstractAudioSource::IAudioSourceListener *srcListener;

	void FillList(Configuration *configuration);

public:
	RetroradioAudioSourceList(AbstractAudioSource::IAudioSourceListener *srcListener,
			Configuration *configuration);

	~RetroradioAudioSourceList();

	bool Init();

	void DeInit();

	void DeactivateAll();

	void ActivateAll(bool need2ReOpenSoundDevices);

	bool AreAllActivated();

	bool AreAllDeactivated();

	void StartChangeToSourceNext();

	bool StartChangeToSourceById(const char *srcId);

	void FinishChangeToSource();

	AbstractAudioSource *GetCurrentSource();

	AbstractAudioSource *GetScheduledSource();

	bool IsSourceChangeOngoing();

	AbstractAudioSource *GetIterator();
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_RETRORADIOAUDIOSOURCELIST_H_ */
