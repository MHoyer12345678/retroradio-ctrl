/*
 * SourceMuteRampCtrl.h
 *
 *  Created on: 24.12.2019
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_SOURCEMUTERAMPCTRL_H_
#define SRC_AUDIOSOURCES_SOURCEMUTERAMPCTRL_H_

#include "BasicMixerControl.h"

#include <glib.h>

namespace retroradio_controller
{

class SourceMuteRampCtrl: public BasicMixerControl
{
public:
	enum RampSpeed
	{
		FAST	= 0,
		NORMAL	= 1,
		SLOW	= 2
	};

	class IMuteRampCtrlListener
	{
	public:
		virtual void OnRampFinished(bool canceled)=0;
	};

	enum State
	{
		_NOT_SET,
		IDLE,
		MUTING,
		UNMUTING
	};

private:
	static const int RAMP_STEP_DELAY_MS[];

	long volumeStep;

	long curVolReal;

	RampSpeed curRampSpeed;

	IMuteRampCtrlListener *listener;

	State state;

	guint timerId;

	void SetupRampTimer(RampSpeed speed);

	static gboolean OnRampTimerElapsed(gpointer user_data);

	void CleanUpTimer();

	void RampFinished(bool canceled);

public:
	SourceMuteRampCtrl(IMuteRampCtrlListener *listener);

	virtual ~SourceMuteRampCtrl();

	virtual bool Init(const char *cardName, const char *mixerName);

	void DeInit();

	void MuteAsync(RampSpeed speed);

	void UnmuteAsync(RampSpeed speed);

	void StopOperation();

};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_SOURCEMUTERAMPCTRL_H_ */
