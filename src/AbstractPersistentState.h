/*
 * AbstractPersistence.h
 *
 *  Created on: 30.01.2020
 *      Author: joe
 */

#ifndef SRC_ABSTRACTPERSISTENTSTATE_H_
#define SRC_ABSTRACTPERSISTENTSTATE_H_

#include <glib.h>
#include <cpp-app-utils/Configuration.h>

using namespace CppAppUtils;

namespace retroradio_controller {

class AbstractPersistentState : public Configuration::IConfigurationParserModule {
private:
	bool ReadStateFile();

	char *persFilePath;

	guint commitTimerId;

	static gboolean OnCommitTimeoutElapsed(gpointer userData);

	bool newCommitRequested;

protected:
	void DoCommit();

	void CommitImmediately();

	void CommitDelayed();

	virtual bool DoWriteDataSet(int fd)=0;

	virtual bool DoReadDataSet(int fd)=0;

	virtual void DoResetToDefault()=0;

	const char *ConfigGetStateFileName();

public:
	AbstractPersistentState(Configuration *configuration);

	virtual ~AbstractPersistentState();

	virtual void Init();

	virtual void DeInit();

	virtual bool ParseConfigFileItem(GKeyFile *confFile, const char *group, const char *key);

	virtual bool IsConfigFileGroupKnown(const char *group);
};

} /* namespace retroradio_controller */

#endif /* SRC_ABSTRACTPERSISTENTSTATE_H_ */
