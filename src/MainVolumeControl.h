/*
 * MainVolumeControl.h
 *
 *  Created on: 23.12.2019
 *      Author: joe
 */

#ifndef SRC_MAINVOLUMECONTROL_H_
#define SRC_MAINVOLUMECONTROL_H_

#include "BasicMixerControl.h"
#include "cpp-app-utils/Configuration.h"

using namespace CppAppUtils;

namespace retroradio_controller {

#define MASTER_VOLUME_DEFAULT 	10
#define MASTER_VOL_MAX			37
#define MASTER_VOL_MIN			0

class MainVolumeControl : public BasicMixerControl,
	public Configuration::IConfigurationParserModule
{
private:
	long volumeStep;

	long currentVolReal;

	char *mainMixerName;

	char *sndCardName;

protected:
	virtual void OnMixerEvent(unsigned int mask);

public:
	MainVolumeControl(Configuration *configuration);

	virtual ~MainVolumeControl();

	virtual bool Init();

	void VolumeUp();

	void VolumeDown();

	void Mute();

	void UnMute();

	const char *ConfigGetCardName();

	const char *ConfigGetMixerName();

	virtual bool ParseConfigFileItem(GKeyFile *confFile, const char *group, const char *key);

	virtual bool IsConfigFileGroupKnown(const char *group);
};

} /* namespace retroradio_controller */

#endif /* SRC_MAINVOLUMECONTROL_H_ */
