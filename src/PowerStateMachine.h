/*
 * MainStateMachine.h
 *
 *  Created on: 22.11.2019
 *      Author: joe
 */

#ifndef SRC_POWERSTATEMACHINE_H_
#define SRC_POWERSTATEMACHINE_H_

#include "AudioController.h"

namespace retroradio_controller {

class PowerStateMachine {

private:
	enum State
	{
		_NOT_INITIALIZED,
		STARTING_UP,
		WAITING_FOR_WIFI_AND_SNDCARD,
		CONNECTION_LOSS,
		SNDCARD_DISAPPEARED,
		ACTIVATING,
		ACTIVE,
		DEACTIVATING,
		STANDBY
	};

private:
	State state;

	bool need2ReOpenSoundDevices;

	void EnterStartingUp();

	void OnStartupFinished();

	void EnterWaitingForWifiAndSndCard();

	void EnterConnectionLoss();

	void EnterSndCardDisappeared();

	void EnterActivating();

	void EnterActive();

	void EnterDeactivating();

	void EnterStandby();

	void SetPowerEnabled(bool enabled);

	static gboolean OnStartupPollTimerElapsed(gpointer user_data);

	void DoEarlyLateHandover();

	void DoProcessPowerBtnEvent();

	bool IsReadyForActivation();

public:
	PowerStateMachine();

	virtual ~PowerStateMachine();

	bool Init();

	void DeInit();

	void OnIRPowerCommandReceived();

	void OnPowerBtnPressed();

	void OnConnectionLost();

	void OnConnectionEstablished();

	void OnSoundCardReady();

	void OnSoundCardDisabled();

	void OnAudioControllerStateChanged(AudioController::State newState);

	void KickOff();

	bool IsPowered();

	bool IsActive();
};

} /* namespace retroradiocontroller */

#endif /* SRC_POWERSTATEMACHINE_H_ */
