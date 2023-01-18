/*
 * DLNAAudioSource.h
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_DLNAAUDIOSOURCE_H_
#define SRC_AUDIOSOURCES_DLNAAUDIOSOURCE_H_

#include "AbstractAudioSource.h"
#include "cpp-app-utils/Configuration.h"

using namespace CppAppUtils;

namespace retroradio_controller {

class DLNAAudioSource: public AbstractAudioSource
{
protected:
	virtual const char *GetConfigGroupName();

	virtual const char *GetDefaultAlsaMixerName();

	virtual const char *GetDefaultSoundCardName();

public:
	DLNAAudioSource(const char *srcName, AbstractAudioSource *predecessor,
			IAudioSourceListener *srcListener);

	virtual ~DLNAAudioSource();

	virtual bool Init();
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_DLNAAUDIOSOURCE_H_ */
