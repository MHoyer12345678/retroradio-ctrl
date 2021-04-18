/*
 * Configuration.h
 *
 *  Created on: 28.11.2016
 *      Author: joe
 */

#ifndef SRC_RETRORADIO_CONTROLLER_CONFIGURATION_H_
#define SRC_RETRORADIO_CONTROLLER_CONFIGURATION_H_

#include <string>

#include <glib.h>

#include "cpp-app-utils/Logger.h"
#include "cpp-app-utils/Configuration.h"

using namespace CppAppUtils;

namespace retroradio_controller {

class RetroradioControllerConfiguration : public Configuration
{
public:
	static const char *Version;

protected:
	virtual const char *GetVersion();

	virtual const char *GetDescriptionString();

	virtual const char *GetCommand();

public:
	RetroradioControllerConfiguration();

	virtual ~RetroradioControllerConfiguration();
};

} /* namespace retroradio_controller */

#endif /* SRC_RETRORADIO_CONTROLLER_CONFIGURATION_H_ */
