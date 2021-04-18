/*
 * AlsaMixerControl.h
 *
 *  Created on: 08.12.2019
 *      Author: joe
 */

#ifndef SRC_BASICMIXERCONTROL_H_
#define SRC_BASICMIXERCONTROL_H_

#include <alsa/asoundlib.h>
#include <glib.h>

namespace retroradio_controller {

class BasicMixerControl {

private:
	gint alsaPollEventIdsCnt;

	guint *alsaPollEventIds;

	static gboolean OnAlsaFDEvent(gint fd,
            GIOCondition condition, gpointer user_data);

	static int OnAlsaMixerEvent(snd_mixer_elem_t *elem, unsigned int mask);

	void ProcessAlsaEvent(GIOCondition condition);

	bool SetupAlsaPollFDs();

	void RegisterEventFDs(struct pollfd *fds, int count);

	void DestroyEventFDs();

protected:
	const char *cardName;

	const char *mixerName;

	long rangeMin;

	long rangeMax;

    snd_mixer_elem_t* mixerElement;

    snd_mixer_t *mixerHandle;

    int MixerVolToPercent(long mixerVol);

    long PercentToMixerVol(int vol);

	virtual void SetVolumeReal(long volReal);

	virtual void SetVolumeNormalized(int volNorm);

	virtual long GetVolumeReal();

	virtual int GetVolumeNormalized();

	virtual void OnMixerEvent(unsigned int mask);

public:
	BasicMixerControl();

	virtual ~BasicMixerControl();

	virtual bool Init(const char *cardName, const char *mixerName);

	void DeInit();

};

} /* namespace retroradio_controller */

#endif /* SRC_BASICMIXERCONTROL_H_ */
