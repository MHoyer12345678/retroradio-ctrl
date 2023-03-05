/*
 * LMCAudioSource.cpp
 *
 *  Created on: 24.11.2019
 *      Author: joe
 */

#include "LMCAudioSource.h"

#include <cpp-app-utils/Logger.h>

#define LMC_CONFIG_GROUP 				"LMC Source"
#define LMC_DEFAULT_ALSA_MIXER_NAME		"lmc_vol"
#define LMC_DEFAULT_SOUND_CARD_NAME		"default"

#define SQUEEZECLIENT_BUSNAME		"Squeeze.Client"
#define SQUEEZECLIENT_OBJPATH		"/SqueezeClient"

#define POWER_TOGGLE	0
#define POWER_OFF		1
#define POWER_ON		2

#define PLAY_TOGGLE		0
#define PLAY_PAUSE		1
#define PLAY_PLAY		2

using namespace CppAppUtils;

using namespace retroradio_controller;

LMCAudioSource::LMCAudioSource(const char *srcName, AbstractAudioSource *predecessor,
		IAudioSourceListener *srcListener) :
		AbstractAudioSource(srcName, predecessor, srcListener),
		squeezeClientControlIface(NULL)
{

}

LMCAudioSource::~LMCAudioSource()
{
	if (this->squeezeClientControlIface!=NULL)
		g_object_unref(this->squeezeClientControlIface);
}

bool LMCAudioSource::Init()
{
	if (!AbstractAudioSource::Init())
		return false;
	Logger::LogDebug("LMCAudioSource::Init - Initializing LMC Audio Source %s.", this->GetName());

	squeeze_client_control_proxy_new_for_bus(G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE, SQUEEZECLIENT_BUSNAME,
			SQUEEZECLIENT_OBJPATH, NULL, LMCAudioSource::OnSystemdProxyNewFinish, this);

	return true;
}

const char* LMCAudioSource::GetConfigGroupName()
{
	return LMC_CONFIG_GROUP;
}

const char* LMCAudioSource::GetDefaultAlsaMixerName()
{
	return LMC_DEFAULT_ALSA_MIXER_NAME;
}

const char* LMCAudioSource::GetDefaultSoundCardName()
{
	return LMC_DEFAULT_SOUND_CARD_NAME;
}

bool LMCAudioSource::IsStartupFinished()
{
	SqueezeClientStateT clientState;

	Logger::LogDebug("LMCAudioSource::IsStartupFinished - Controller asks us if we are ready.");
	if (this->squeezeClientControlIface==NULL)
	{
		Logger::LogDebug("LMCAudioSource::IsStartupFinished - DBUS proxy not yet up. Returning false.");
		return false;
	}

	clientState=(SqueezeClientStateT)squeeze_client_control_get_squeeze_client_state(this->squeezeClientControlIface);
	Logger::LogDebug("LMCAudioSource::IsStartupFinished - Client state is: %d", clientState);

	return clientState!=__NOT_SET;
}

void LMCAudioSource::OnSystemdProxyNewFinish(GObject *source_object,
		GAsyncResult *res, gpointer user_data)
{
	LMCAudioSource* instance=(LMCAudioSource *)user_data;
	GError *error=NULL;

	instance->squeezeClientControlIface=squeeze_client_control_proxy_new_finish(res,&error);

	if (error != NULL)
	{
		Logger::LogError("Unable to connect to squeezeclient: %s",error->message);
		g_error_free(error);
		return;
	}

	(void) g_signal_connect(instance->squeezeClientControlIface, "volume-changed",
			G_CALLBACK(LMCAudioSource::OnVolumeChanged), instance);
	(void) g_signal_connect(instance->squeezeClientControlIface, "g-properties-changed",
			G_CALLBACK(LMCAudioSource::OnPropertiesChanged), instance);
	Logger::LogDebug("LMCAudioSource::OnSystemdProxyNewFinish -> Connected with squeezeclient.");
}

void LMCAudioSource::OnVolumeChanged(SqueezeClientControl *object, void *userData)
{
	Logger::LogDebug("LMCAudioSource::OnVolumeChanged -> Squeezeclient changed volume.");
}

void LMCAudioSource::OnPropertiesChanged(SqueezeClientControl *object, GVariant *changedProperties,
		char **invalidatedProperties, gpointer userData)
{
	LMCAudioSource* instance=(LMCAudioSource *)userData;
	GVariant* propertyVal;

	Logger::LogDebug("LMCAudioSource::OnPropertiesChanged -> Squeezeclient changed a property.");
	propertyVal=g_variant_lookup_value(changedProperties, "SqueezeClientState", NULL);
	if (propertyVal==NULL) return;

	instance->OnSqueezeClientStateChanged((SqueezeClientStateT)g_variant_get_byte(propertyVal));
}

void LMCAudioSource::OnSqueezeClientStateChanged(SqueezeClientStateT newState)
{
	Logger::LogDebug("LMCAudioSource::OnSqueezeClientStateChanged -> Squeezeclient changed state to %d.", newState);
}

void LMCAudioSource::DoStartPlaying()
{
	Logger::LogDebug("LMCAudioSource::DoStartPlaying -> LMC audio source requested to play.");

	if (this->squeezeClientControlIface!=NULL)
	{
		GError* err=NULL;
		//request squeeze server to power us on (if not yet on already)
		//request squeeze server to start playing (if not yet on already)
		if (!squeeze_client_control_call_request_power_state_change_sync (
			this->squeezeClientControlIface, POWER_ON,NULL,&err))
			Logger::LogError("Error sending power on request to squeeze client: %s", err->message);
		else if (!squeeze_client_control_call_request_play_sync (
				this->squeezeClientControlIface,NULL,&err))
			Logger::LogError("Error sending power on request to squeeze client: %s", err->message);

		#warning later away when IR cmd queue is implemented in squeezecient
		//-------------------->
		usleep(250000);
		Logger::LogError("Sending play again");
		squeeze_client_control_call_request_play_sync (
						this->squeezeClientControlIface,NULL,NULL);
		//-------------------->

		if (err==NULL)
			Logger::LogDebug("LMCAudioSource::DoStartPlaying -> Successfully called squeeze dbus commands play and power on.");
		else
			g_error_free(err);
	}
	else
		Logger::LogError("Start playing requested without DBUS proxy up and running.");

	this->SourceStartPlayingFinished();
}

void LMCAudioSource::DoStopPlaying()
{
	Logger::LogDebug("LMCAudioSource::DoStoptPlaying -> LMC audio source requested to stop playing.");

	if (this->squeezeClientControlIface!=NULL)
	{
		GError* err=NULL;
		//request squeeze server to pause playing (if not yet on pause already)
		if (!squeeze_client_control_call_request_pause_resume_sync (
				this->squeezeClientControlIface,PLAY_PAUSE, NULL,&err))
		{
			Logger::LogError("Error sending pause request to squeeze client: %s", err->message);
			g_error_free(err);
		}
	}
	else
		Logger::LogError("Stop playing requested without DBUS proxy up and running.");

	this->SourceStopPlayingFinished();
}

void LMCAudioSource::Next()
{
	Logger::LogDebug("LMCAudioSource::Next -> LMC audio source requested to switch to next track.");
	if (this->squeezeClientControlIface!=NULL)
	{
		GError* err=NULL;
		//request squeeze server to switch to next track
		if (!squeeze_client_control_call_request_next_sync (
				this->squeezeClientControlIface,NULL,&err))
		{
			Logger::LogError("Error sending next request to squeeze client: %s", err->message);
			g_error_free(err);
		}
	}
	else
		Logger::LogError("Next track requested without DBUS proxy up and running.");
}

void LMCAudioSource::Previous()
{
	Logger::LogDebug("LMCAudioSource::Previous -> LMC audio source requested to switch to previous track.");
	if (this->squeezeClientControlIface!=NULL)
	{
		GError* err=NULL;
		//request squeeze server to switch to previous track
		if (!squeeze_client_control_call_request_prev_sync (
				this->squeezeClientControlIface,NULL,&err))
		{
			Logger::LogError("Error sending previous request to squeeze client: %s", err->message);
			g_error_free(err);
		}
	}
	else
		Logger::LogError("Previous track requested without DBUS proxy up and running.");
}
