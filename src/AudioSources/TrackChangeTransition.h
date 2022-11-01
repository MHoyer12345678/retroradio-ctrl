/*
 * TrackChangeTransition.h
 *
 *  Created on: 19.01.2020
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_TRACKCHANGETRANSITION_H_
#define SRC_AUDIOSOURCES_TRACKCHANGETRANSITION_H_

namespace retroradio_controller {

#define _NO_TRACK_SET_		-1

class TrackChangeTransition {
public:
	enum State
	{
		IDLE,
		RAMPING_DOWN,
		RAMPING_UP
	};

private:
	State state;

	//an absolute track number has been selected is stored here
	int trackNoSelected;

	//positive numbers means number of next() calls to MPD, negative number means number of previous() calls to MPD
	int noPendingTrackChanges;

public:
	TrackChangeTransition();

	virtual ~TrackChangeTransition();

	void Reset();

	void StartNew();

	bool OneChangeProcessed();

	void Finalize();

	void Finished();

	void NextPressed();

	void PreviousPressed();

	void TrackSelected(unsigned int trackNo);

	bool NextCallsPending();

	bool PreviousCallsPending();

	bool TrackSelectionPending();

	int GetSelectedTrack(bool reset);

	State GetState();
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_TRACKCHANGETRANSITION_H_ */
