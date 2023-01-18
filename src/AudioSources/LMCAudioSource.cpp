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

