/*
 * AlsaMixerControl.cpp
 *
 *  Created on: 08.12.2019
 *      Author: joe
 */

#include "BasicMixerControl.h"

#include "cpp-app-utils/Logger.h"
#include <glib-unix.h>

using namespace CppAppUtils;

namespace retroradio_controller {

BasicMixerControl::BasicMixerControl() :
		mixerName(NULL),
		cardName(NULL),
		mixerElement(NULL),
		mixerHandle(NULL),
		rangeMin(0),
		rangeMax(100),
		alsaPollEventIds(NULL),
		alsaPollEventIdsCnt(-1)
{
}

BasicMixerControl::~BasicMixerControl()
{
}

bool BasicMixerControl::Init(const char *cardName, const char *mixerName)
{
    snd_mixer_selem_id_t *mixerId;

    if (this->mixerHandle!=NULL)
    	this->DeInit();

    this->mixerName=mixerName;
    this->cardName=cardName;

    if (snd_mixer_open(&this->mixerHandle, 0)!=0)
	{
		Logger::LogError("Unable to get handle for alsa mixer API.");
		return false;
	}

    if (snd_mixer_attach(this->mixerHandle, this->cardName)!=0)
	{
		Logger::LogError("Unable to attach handle to card %s.", this->cardName);
		return false;
	}

    if (snd_mixer_selem_register(this->mixerHandle, NULL, NULL)!=0)
	{
		Logger::LogError("Unable register mixer simple element class.");
		return false;
	}

    if (snd_mixer_load(this->mixerHandle)!=0)
	{
		Logger::LogError("Unable load mixer element.");
		return false;
	}

    snd_mixer_selem_id_alloca(&mixerId);

	snd_mixer_selem_id_set_index(mixerId, 0);
    snd_mixer_selem_id_set_name(mixerId, this->mixerName);
    this->mixerElement=snd_mixer_find_selem(this->mixerHandle, mixerId);

    if (this->mixerElement == NULL)
	{
		Logger::LogError("Unable to find mixer element %s.", this->mixerName);
		return false;
	}

	snd_mixer_elem_set_callback_private(this->mixerElement, this);
	snd_mixer_elem_set_callback(this->mixerElement, OnAlsaMixerEvent);

    if (snd_mixer_selem_get_playback_volume_range(this->mixerElement, &this->rangeMin, &this->rangeMax)!=0)
    {
		Logger::LogInfo("Unable to determine range of mixer: %s. Taking 0 and 100 as range.", this->mixerName);
		this->rangeMin=0;
		this->rangeMin=100;
    }

    if (!this->SetupAlsaPollFDs())
    {
		Logger::LogError("Unable to register alsa event file descriptors for mixer: %s.", this->mixerName);
		return false;
    }

	Logger::LogDebug("BasicMixerControl::Init() - Initialized mixer %s of card %s. Volume range: %ld-%ld",
			this->mixerName, this->cardName, this->rangeMin, this->rangeMax);

	return true;
}

gboolean BasicMixerControl::OnAlsaFDEvent(gint fd, GIOCondition condition,
		gpointer user_data)
{
	BasicMixerControl *instance=(BasicMixerControl *)user_data;
	instance->ProcessAlsaEvent(condition);
	return TRUE;
}

void BasicMixerControl::ProcessAlsaEvent(GIOCondition condition)
{
	if ((condition & G_IO_ERR)!=0)
	{
		Logger::LogError("Poll FD of mixer %s released with condition==G_IO_ERR. Sound card removed. Deinitializing mixer.",
				this->mixerName);
		this->DeInit();
	}
	else if (this->mixerHandle)
		snd_mixer_handle_events(this->mixerHandle);
}

bool BasicMixerControl::SetupAlsaPollFDs()
{
	struct pollfd *fds = NULL;
	int count,rc;
	long mixerVol;

	Logger::LogDebug("BasicMixerControl::SetupAlsaPollFDs() - Setting up alsa event poll fds in main loop.");

	count = snd_mixer_poll_descriptors_count(this->mixerHandle);
	if (count < 0)
	{
		Logger::LogError("snd_mixer_poll_descriptors_count() failed\n");
		return false;
	}

	fds = (struct pollfd *)alloca(count*sizeof(struct pollfd));
	rc = snd_mixer_poll_descriptors (this->mixerHandle, fds, count);
	if (rc < 0)
	{
		Logger::LogError("snd_mixer_poll_descriptors() failed\n");
		return false;
	}

	this->RegisterEventFDs(fds, count);

	return true;
}

void BasicMixerControl::RegisterEventFDs(struct pollfd *fds, int count)
{
	this->DestroyEventFDs();

	this->alsaPollEventIdsCnt=count;
	this->alsaPollEventIds=new guint[count];

	for (int a=0;a<count;a++)
	{
	    GIOCondition con=(GIOCondition)fds[a].events; //take care: assumes that glib2.0 uses same bitmask as poll which is currently actually the case
		Logger::LogDebug("BasicMixerControl::RegisterEventFDs() - Adding poll fd with event mask: 0x%X.", con);
	    this->alsaPollEventIds[a]=g_unix_fd_add(fds[a].fd,con,BasicMixerControl::OnAlsaFDEvent,this);
	}
}

void BasicMixerControl::DestroyEventFDs()
{
	if (this->alsaPollEventIdsCnt!=-1)
	{
		Logger::LogDebug("BasicMixerControl::DestroyEventFDs() - Removing alsa event fds from main loop.");
		for (int a=0;a<alsaPollEventIdsCnt;a++)
			g_source_remove(this->alsaPollEventIds[a]);

		delete this->alsaPollEventIds;
		this->alsaPollEventIdsCnt=-1;
	}
}

int BasicMixerControl::OnAlsaMixerEvent(snd_mixer_elem_t *elem,
		unsigned int mask)
{
	BasicMixerControl *instance=(BasicMixerControl *)snd_mixer_elem_get_callback_private(elem);
	instance->OnMixerEvent(mask);
	return 0;
}

void BasicMixerControl::SetVolumeReal(long volReal)
{
	if (this->mixerElement==NULL) return;

	if (volReal<this->rangeMin) volReal=rangeMin;
	if (volReal>this->rangeMax) volReal=rangeMax;

	Logger::LogDebug("BasicMixerControl::SetVolumeReal - now setting volume to %ld", volReal);

    if (snd_mixer_selem_set_playback_volume_all(this->mixerElement,volReal)!=0)
		Logger::LogError("Unable to set volume of mixer %s to mixerVolume %ld",
				this->mixerName, volReal);
}

void BasicMixerControl::SetVolumeNormalized(int volNorm)
{
	this->SetVolumeReal(this->PercentToMixerVol(volNorm));
}

long BasicMixerControl::GetVolumeReal()
{
	long mixerVolReal;

	if (this->mixerElement==NULL) return -1;

	if (snd_mixer_selem_get_playback_volume(this->mixerElement,
			SND_MIXER_SCHN_FRONT_LEFT,&mixerVolReal)!=0)
	{
		Logger::LogError("Unable to read volume from mixer %s", this->mixerName);
		return this->rangeMin;
	}

	return mixerVolReal;
}

int BasicMixerControl::GetVolumeNormalized()
{
	long mixerVolReal;

	mixerVolReal=this->GetVolumeReal();
	if (mixerVolReal==-1)
		return -1;

	return this->MixerVolToPercent(mixerVolReal);
}

void BasicMixerControl::DeInit()
{
	this->DestroyEventFDs();

	if (this->mixerElement!=NULL)
		snd_mixer_elem_set_callback(this->mixerElement, NULL);

	if (this->mixerHandle!=NULL)
	{
	    snd_mixer_close(this->mixerHandle);
	    this->mixerName=NULL;
	    this->cardName=NULL;
	    this->mixerHandle=NULL;
	    this->mixerElement=NULL;
	    this->rangeMin=0;
	    this->rangeMax=100;
	}
}

void BasicMixerControl::OnMixerEvent(unsigned int mask)
{

}

int BasicMixerControl::MixerVolToPercent(long mixerVol)
{
	return ((mixerVol-this->rangeMin)*100)/(this->rangeMax-this->rangeMin);
}

long BasicMixerControl::PercentToMixerVol(int vol)
{
	return this->rangeMin+(vol*(this->rangeMax-this->rangeMin)/100);
}

} /* namespace retroradio_controller */
