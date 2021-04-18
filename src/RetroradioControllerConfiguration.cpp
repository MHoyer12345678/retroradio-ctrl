/*
 * Configuration.cpp
 *
 *  Created on: 28.11.2016
 *      Author: joe
 */

#include "RetroradioControllerConfiguration.h"

#include "RetroradioController.h"

#include <iostream>
#include <sysexits.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define DEFAULT_CONF_FILE				"/etc/retroradiocontroller/retroradio.conf"


namespace retroradio_controller {

const char *RetroradioControllerConfiguration::Version="v2.0";

RetroradioControllerConfiguration::RetroradioControllerConfiguration() :
		Configuration(DEFAULT_CONF_FILE,Logger::INFO)
{

}

RetroradioControllerConfiguration::~RetroradioControllerConfiguration()
{

}

const char *RetroradioControllerConfiguration::GetVersion()
{
	return Version;
}

const char *RetroradioControllerConfiguration::GetDescriptionString()
{
	return "Starts retroradio controller application.";
}

const char *RetroradioControllerConfiguration::GetCommand()
{
	return "retroradio-controller";
}

} /* namespace RetroradioController */
