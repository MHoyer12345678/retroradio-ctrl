/*
 * TrackChangeTransition.cpp
 *
 *  Created on: 19.01.2020
 *      Author: joe
 */

#include "TrackChangeTransition.h"

using namespace retroradio_controller;

TrackChangeTransition::TrackChangeTransition()
{
	this->Reset();
}

TrackChangeTransition::~TrackChangeTransition()
{
}

void TrackChangeTransition::Reset()
{
	this->state=IDLE;
	this->noPendingTrackChanges=0;
	this->trackNoSelected=_NO_TRACK_SET_;
}

void TrackChangeTransition::StartNew()
{
	this->noPendingTrackChanges=0;
	this->trackNoSelected=_NO_TRACK_SET_;
	this->state=RAMPING_DOWN;
}

bool TrackChangeTransition::OneChangeProcessed()
{
	if (this->noPendingTrackChanges<0)
		this->noPendingTrackChanges++;
	else if (this->noPendingTrackChanges>0)
		this->noPendingTrackChanges--;
	else
		return false;

	return true;
}

void TrackChangeTransition::Finalize()
{
	this->state=RAMPING_UP;
	this->noPendingTrackChanges=0;
	this->trackNoSelected=_NO_TRACK_SET_;
}

void TrackChangeTransition::Finished()
{
	this->state=IDLE;
}

void TrackChangeTransition::NextPressed()
{
	//no reset of absolut track selected. Next/prev pressed after absolute track was selected.
	//Sequence is kept when processing the events (#1: change to absolute track, #2: process next/prev events)
	if (this->state==RAMPING_DOWN)
		this->noPendingTrackChanges++;
}

void TrackChangeTransition::PreviousPressed()
{
	//no reset of absolut track selected. Next/prev pressed after absolute track was selected.
	//Sequence is kept when processing the events (#1: change to absolute track, #2: process next/prev events)
	if (this->state==RAMPING_DOWN)
		this->noPendingTrackChanges--;
}

void TrackChangeTransition::TrackSelected(unsigned int trackNo)
{
	if (this->state==RAMPING_DOWN)
	{
		this->trackNoSelected=trackNo;
		//reset any pending next/prev calls when new absolute track has been selected
		this->noPendingTrackChanges=0;
	}
}


bool TrackChangeTransition::NextCallsPending()
{
	return this->noPendingTrackChanges>0;
}

bool TrackChangeTransition::PreviousCallsPending()
{
	return this->noPendingTrackChanges<0;
}

bool TrackChangeTransition::TrackSelectionPending()
{
	return this->trackNoSelected!=_NO_TRACK_SET_;
}

int TrackChangeTransition::GetSelectedTrack(bool reset)
{
	int trackNo=this->trackNoSelected;

	if (reset)
		this->trackNoSelected=_NO_TRACK_SET_;

	return trackNo;
}

TrackChangeTransition::State TrackChangeTransition::GetState()
{
	return this->state;
}
