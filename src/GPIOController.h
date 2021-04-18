/*
 * PowerBTNAndLEDControl.h
 *
 *  Created on: 10.03.2020
 *      Author: joe
 */

#ifndef SRC_POWERBTNANDLEDCONTROL_H_
#define SRC_POWERBTNANDLEDCONTROL_H_

#include <generic-embedded-utils/GPIOInput.h>
#include <generic-embedded-utils/GPIOOutput.h>

#include <glib.h>

using namespace GenericEmbeddedUtils;

namespace retroradio_controller {

class GPIOController : public GPIOInput::IGpioValueListener
{
public:
	typedef enum
	{
		POWER_OFF,
		POWER_ON,
		WAITING_FOR_WIFI
	} PowerLedMode;

	class IBtnListener
	{
		public:
			virtual void OnPowerButtonReleased()=0;
	};

private:
	typedef struct
	{
		const char *SRC_ID;
		GPIOOutput *srcLedGPIO;
	} SourceLedGPIO;

	bool btnEventDelayTimerSet;

	GPIOInput *powerBtnGPIO;

	GPIOOutput *ampPowerGPIO;

	GPIOOutput *powerLedGPIO;

	SourceLedGPIO *sourceLedGPIOs;

	IBtnListener *btnListener;

	void CreateSourceGPIOArray();

	void DeleteSourceGPIOArray();

	static gboolean OnPwrBtnEventDelayElapsed(gpointer data);

	void EvaluatePwrBtnStateAfterEventTimeout();

public:
	GPIOController(IBtnListener *btnListener);

	virtual ~GPIOController();

	bool Init();

	void DeInit();

	void SetPowerLedMode(PowerLedMode powerLedMode);

	void SetSourceLedEnabled(const char *sourceID, bool enabled);

	void DisableSourcesLeds();

	void SetAmpEnabled(bool enabled);

	virtual void OnValueChanged(GPIOInput *gpio, bool value);

	virtual int GetPollIntervalUs();
};

} /* namespace retroradio_controller */

#endif /* SRC_POWERBTNANDLEDCONTROL_H_ */
