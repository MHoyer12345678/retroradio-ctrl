/*
 * LMCAudioSource.h
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_LMCAUDIOSOURCE_H_
#define SRC_AUDIOSOURCES_LMCAUDIOSOURCE_H_

#include "AbstractAudioSource.h"

namespace retroradio_controller {

class LMCAudioSource: public AbstractAudioSource {

protected:
	virtual const char *GetConfigGroupName();

	virtual const char *GetDefaultAlsaMixerName();

	virtual const char *GetDefaultSoundCardName();

public:
	LMCAudioSource(const char *srcName, AbstractAudioSource *predecessor,
			IAudioSourceStateListener *srcListener);

	virtual ~LMCAudioSource();

	virtual bool Init();
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_LMCAUDIOSOURCE_H_ */
