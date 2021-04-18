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
}

void TrackChangeTransition::StartNew()
{
	this->noPendingTrackChanges=0;
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
}

void TrackChangeTransition::Finished()
{
	this->state=IDLE;
}

void TrackChangeTransition::NextPressed()
{
	if (this->state==RAMPING_DOWN)
		this->noPendingTrackChanges++;
}

void TrackChangeTransition::PreviousPressed()
{
	if (this->state==RAMPING_DOWN)
		this->noPendingTrackChanges--;
}

bool TrackChangeTransition::NextCallsPending()
{
	return this->noPendingTrackChanges>0;
}

bool TrackChangeTransition::PreviousCallsPending()
{
	return this->noPendingTrackChanges<0;
}

TrackChangeTransition::State TrackChangeTransition::GetState()
{
	return this->state;
}
