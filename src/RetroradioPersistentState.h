/*
 * RetroradioPersistentState.h
 *
 *  Created on: 01.02.2020
 *      Author: joe
 */

#ifndef SRC_RETRORADIOPERSISTENTSTATE_H_
#define SRC_RETRORADIOPERSISTENTSTATE_H_

#include "AbstractPersistentState.h"

namespace retroradio_controller {

class RetroradioPersistentState: public AbstractPersistentState {

private:
	typedef struct PersistentState
	{
		char stateStartTag[2];

		unsigned char powerState;

		long masterVolume;

		char currentSrcId[128];

		unsigned int mpdCurrentTrackNr;

		char stateEndTag[2];

	} PersistentState;

	PersistentState state;

	bool CheckDataSetSignature();

	bool CheckPowerValue();

	bool CheckMasterVolumeValue();

	bool CheckCurrentSrcId();

protected:
	virtual bool DoWriteDataSet(int fd);

	virtual bool DoReadDataSet(int fd);

	virtual void DoResetToDefault();

public:
	RetroradioPersistentState(Configuration *configuration);

	virtual ~RetroradioPersistentState();

	void SetPowerStateActive(bool isActive);

	bool IsPowerStateActive();

	void SetMasterVolume(long masterVolume);

	long GetMasterVolume();

	void SetCurrentSrcId(const char *sourceId);

	const char *GetCurrentSrcId();

	void SetMPDCurrentTrackNr(unsigned int currentTrackNr);

	unsigned int GetMPDCurrentTrackNr();
};

} /* namespace retroradio_controller */

#endif /* SRC_RETRORADIOPERSISTENTSTATE_H_ */
