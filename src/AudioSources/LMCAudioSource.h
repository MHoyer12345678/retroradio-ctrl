/*
 * LMCAudioSource.h
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_LMCAUDIOSOURCE_H_
#define SRC_AUDIOSOURCES_LMCAUDIOSOURCE_H_

#include "AbstractAudioSource.h"

#include "AudioSources/generated/SqueezeClientInterface.h"

namespace retroradio_controller {

class LMCAudioSource: public AbstractAudioSource {

private:
	enum SqueezeClientStateT
	{
		__NOT_SET			= 0,
		STOPPED				= 1,
		PLAYING				= 2,
		PAUSED				= 3,
		POWERED_OFF			= 4,
		SRV_DISCONNECTED	= 5,
		SRV_CONNECTING		= 6
	};

	SqueezeClientControl *squeezeClientControlIface;

	static void OnSystemdProxyNewFinish(GObject *source_object,
			GAsyncResult *res, gpointer user_data);

	static void OnVolumeChanged(SqueezeClientControl *object, void *userData);

	static void	OnPropertiesChanged(SqueezeClientControl *object,
			GVariant* changedProperties, char** invalidatedProperties, gpointer userData);

	void OnSqueezeClientStateChanged(SqueezeClientStateT newState);

protected:
	virtual const char *GetConfigGroupName();

	virtual const char *GetDefaultAlsaMixerName();

	virtual const char *GetDefaultSoundCardName();

	virtual bool IsStartupFinished();

public:
	LMCAudioSource(const char *srcName, AbstractAudioSource *predecessor,
			IAudioSourceListener *srcListener);

	virtual ~LMCAudioSource();

	virtual bool Init();
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_LMCAUDIOSOURCE_H_ */
