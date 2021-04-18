/*
 * AbstractPersistence.cpp
 *
 *  Created on: 30.01.2020
 *      Author: joe
 */

#include "AbstractPersistentState.h"

#include "cpp-app-utils/Logger.h"

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>



using namespace retroradio_controller;
using namespace CppAppUtils;

//the delayed commit is written to storage after 2xCOMIT_TIMOUT_MS without
//any further commit in between
#define COMIT_TIMOUT_MS 500

#define PERSISTENCE_CONFIG_GROUP		"Persistence"
#define DEFAULT_PERS_FILE_PATH 			"/var/lib/retroradio.state"
#define PERSISTENCE_CONFIG_TAG_PERSFILE	"FilePath"

AbstractPersistentState::AbstractPersistentState(Configuration *configuration) :
		commitTimerId(0),
		newCommitRequested(false),
		persFilePath(NULL)
{
	configuration->AddConfigurationModule(this);
}

AbstractPersistentState::~AbstractPersistentState()
{
	if (this->persFilePath!=NULL)
		free(this->persFilePath);
}

void AbstractPersistentState::Init()
{
	Logger::LogDebug("AbstractPersistentState::Init - Initializing persistent state.");
	if (!ReadStateFile())
	{
		Logger::LogError("Setting default values.");
		this->DoResetToDefault();
	}
}

void AbstractPersistentState::DeInit()
{
	Logger::LogDebug("AbstractPersistentState::DeInit - DeInitializing persistent state.");
	this->DoCommit();

	this->newCommitRequested=false;
	if (this->commitTimerId==0)
	{
		g_source_remove(this->commitTimerId);
		this->commitTimerId=0;
	}
}

void AbstractPersistentState::DoCommit()
{
	Logger::LogDebug("AbstractPersistentState::DoCommit - Opening state to file %s.", this->ConfigGetStateFileName());
	int fd=open(this->ConfigGetStateFileName(), O_RDWR | O_CREAT | O_SYNC, S_IRUSR | S_IWUSR);
	if (fd==-1)
	{
		Logger::LogError("Unable to open state file %s for writing.", this->ConfigGetStateFileName());
		return;
	}

	Logger::LogDebug("AbstractPersistentState::DoCommit - Writing state.");
	if (!this->DoWriteDataSet(fd))
		Logger::LogError("Unable to write into state file %s.", this->ConfigGetStateFileName());

	close(fd);
}

void AbstractPersistentState::CommitImmediately()
{
	Logger::LogDebug("AbstractPersistentState::CommitImmediately - Immediate commit requested.");
	if (this->commitTimerId!=0)
	{
		g_source_remove(this->commitTimerId);
		this->commitTimerId=0;
	}

	this->DoCommit();
}

void AbstractPersistentState::CommitDelayed()
{
	Logger::LogDebug("AbstractPersistentState::CommitDelayed - Delayed commit requested.");
	if (this->commitTimerId==0)
		this->commitTimerId=g_timeout_add(COMIT_TIMOUT_MS,
				AbstractPersistentState::OnCommitTimeoutElapsed, this);

	this->newCommitRequested=true;
}

gboolean AbstractPersistentState::OnCommitTimeoutElapsed(gpointer userData)
{
	AbstractPersistentState *instance = (AbstractPersistentState *)userData;
	Logger::LogDebug("AbstractPersistentState::OnCommitTimeoutElapsed - Commit timeout elapsed.");

	//wait until after a timeout no one has requested an additional commit anymore
	if (instance->newCommitRequested)
	{
		Logger::LogDebug("AbstractPersistentState::OnCommitTimeoutElapsed - Further commit requests detected. Waiting.");
		instance->newCommitRequested=false;
		return TRUE;
	}

	instance->DoCommit();

	instance->commitTimerId=0;
	return FALSE;
}

bool AbstractPersistentState::ReadStateFile()
{
	bool result;
	Logger::LogDebug("AbstractPersistentState::Init - Opening state file %s.", this->ConfigGetStateFileName());

	int fd=open(this->ConfigGetStateFileName(), O_RDONLY | O_NONBLOCK);
	if (fd==-1)
	{
		Logger::LogError("Unable to open state file %s for reading.", this->ConfigGetStateFileName());
		return false;
	}

	Logger::LogDebug("AbstractPersistentState::Init - File opened. Reading content.");
	result=this->DoReadDataSet(fd);

	if (!result)
		Logger::LogError("State file %s corrupt.", this->ConfigGetStateFileName());

	close(fd);

	return result;
}

const char* AbstractPersistentState::ConfigGetStateFileName()
{
	if (this->persFilePath!=NULL)
		return this->persFilePath;
	else
		return DEFAULT_PERS_FILE_PATH;
}

bool AbstractPersistentState::ParseConfigFileItem(GKeyFile* confFile, const char* group, const char* key)
{
	if (strcasecmp(group, PERSISTENCE_CONFIG_GROUP)!=0) return true;
	if (strcasecmp(key, PERSISTENCE_CONFIG_TAG_PERSFILE)==0)
	{
		char *path;
		Logger::LogDebug("AbstractPersistentState::ParseConfigFileItem - Found key %s.",key);
		if (Configuration::GetStringValueFromKey(confFile,key,group, &path))
		{
			if (this->persFilePath!=NULL)
				free(this->persFilePath);
			this->persFilePath=path;
		}
		else
			return false;
	}

	return true;
}

bool AbstractPersistentState::IsConfigFileGroupKnown(const char* group)
{
	return strcasecmp(group, PERSISTENCE_CONFIG_GROUP);
}
