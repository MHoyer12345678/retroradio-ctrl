/*
 * TrackChangeTransition.h
 *
 *  Created on: 19.01.2020
 *      Author: joe
 */

#ifndef SRC_AUDIOSOURCES_TRACKCHANGETRANSITION_H_
#define SRC_AUDIOSOURCES_TRACKCHANGETRANSITION_H_

namespace retroradio_controller {

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

	bool NextCallsPending();

	bool PreviousCallsPending();

	State GetState();
};

} /* namespace retroradio_controller */

#endif /* SRC_AUDIOSOURCES_TRACKCHANGETRANSITION_H_ */
