
#ifndef _AFNLOGGER_H_
#define _AFNLOGGER_H_

#include <fstream>
#include <sstream>
#include <string.h>
#include <queue>
#include <map>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <dirent.h>
#include <zlib.h>
#include <sys/statvfs.h>
#include <typeinfo>
#include <mutex>
#include <condition_variable>

#define LOG_PATH "/var/logs/"
#define LOGNAME_FORMAT_STARTUP "%Y_%m_%d-%H_%M_%S_startup.log"
#define LOGNAME_FORMAT "%Y_%m_%d-%H_%M_%S.log"
#define ERRORLOG_FORMAT "errorlog_%Y_%m_%d-%H_%M_%S.log"

using namespace std;

typedef enum LOG_LEVEL
{
	LOG_LEVEL_ERROR = 0,
	LOG_LEVEL_WARN = 1,
	LOG_LEVEL_INFO = 2,
	LOG_LEVEL_DEBUG = 3,
	LOG_LEVEL_TRACE = 4,
}LogLevel;

typedef enum LOG_FORMAT
{
	LOG_MSG = 0,
	LOG_FUNC_MSG = 1,
	LOG_FUNC_EXP = 2,
}LogFormat;

class AutoResetEvent
{
public:
	explicit AutoResetEvent(bool initial = false);

	void Set();
	void Reset();

	bool WaitOne();

private:
	AutoResetEvent(const AutoResetEvent&);
	AutoResetEvent& operator=(const AutoResetEvent&);
	bool flag_;
	std::mutex protect_;
	std::condition_variable signal_;
};

class LogData
{
public:
	unsigned int l_threadId;
	LogLevel l_logLevel;
	std::string l_funcName;
	std::string l_msg;
	struct timespec l_time;
	std::string l_exp;
	LogFormat l_logFormat;

	LogData();

	LogData(unsigned int threadId, struct timespec time, LogLevel loglevel, std::string funcname, std::string msg);
	LogData(unsigned int threadId, struct timespec time, LogLevel loglevel, std::string msg);
	LogData(unsigned int threadId, struct timespec time, LogLevel loglevel, std::string msg, std::exception &exp);

	void SetValues(unsigned int threadId, struct timespec time, LogLevel loglevel, std::string funcname, std::string msg);
	void SetValues(unsigned int threadId, struct timespec time, LogLevel loglevel, std::string msg);
	void SetValues(unsigned int threadId, struct timespec time, LogLevel loglevel, std::string msg, std::exception &exp);

};

class Logger
{
public:
	void setMaxQThreshold(int val);

	static void zipFile(std::string file, const char *mode);

	void error(std::string func, std::string text);
	void warn(std::string func, std::string text) throw();
	void info(std::string func, std::string text) throw();
	void debug(std::string func, std::string text) throw();
	void trace(std::string func, std::string text) throw();

	void error(std::string text, std::exception &e);
	void warn(std::string text, std::exception &e) throw();
	void info(std::string text, std::exception &e) throw();
	void debug(std::string text, std::exception &e) throw();
	void trace(std::string text, std::exception &e) throw();

	void error(std::string text);
	void warn(std::string text) throw();
	void info(std::string text) throw();
	void debug(std::string text) throw();
	void trace(std::string text) throw();

	void updateLogLevel(LogLevel logLevel);

	void setParams(int queueThreshold, int fileSize, bool zip, std::string fileNamePrefix);

protected:
	Logger();
	Logger(std::string pathName, int queueThreshold, int fileSize, bool zipFile, std::string fileNamePrefix);
	~Logger();

	static void closeLogger();

	// Wrapper function for lock/unlock
	// For Extensible feature, lock and unlock should be in protected
	void lock();
	void unlock();
	bool TryCreateFile();
	bool CreateFile(std::string fname);
	std::string getFileName();
	std::string getErrorFileName();

private:
	void closeFile();

	void zipExistingFiles();

	std::string ProcessLogData(LogData log);
	void *LogQProcessor();
	static void * LogQProcessorHelper(void * object);
	void *CheckDiskSpace();
	static void * CheckDiskSpaceHelper(void * object);

	std::string prepareLog(unsigned int threadId, struct timespec  time, std::string func, std::string text, LOG_LEVEL loglvl, std::string ex, LogFormat logFormat) throw();
	void logIntoFile(std::string& data);
	void swapQueues();
	Logger(const Logger& obj) {}
	void operator=(const Logger& obj) {}

private:
	static Logger*          m_Instance;
	std::ofstream			m_File;

	pthread_mutex_t         m_Mutex;
	pthread_mutexattr_t     m_Attr;
	pthread_t				LogQProcessorThread;
	pthread_t				CheckDiskSpaceThread;

	std::string 			m_LogPath;
	std::string 			m_LogNameFormatStartup;
	std::string 			m_LogNameFormat;
	std::string				m_FileName;
	std::string				m_FileNamePrefix;
	unsigned int 			m_FileSize;
	unsigned int			m_MaxQThreshold;
	unsigned int 			m_MinFreeDisk;
	//std::string 			ERRORLOG_FORMAT;  /*= "errorlog_%Y_%m_%d-%H_%M_%S.log";*/
	LogLevel                m_LogLevel;

	std::queue<LogData> Queue1;
	std::queue<LogData> Queue2;
	std::queue<LogData> *inQueue;
	std::queue<LogData> *outQueue;
	bool m_ZipFile;
	bool m_Running;
	bool streamOK;
	bool logFileOK;

	friend class AfnLogger;

};

class AfnLogger
{
public:
	static Logger*	getInstance(std::string path);
	static void   	setParams(int queueThreshold, int fileSize, bool zipFile, std::string fileNamePrefix);
	static bool		closeLogger(std::string path);
	static bool		closeLogger(Logger*);
	static void   	closeAllLoggers();

private:
	static std::map<std::string, Logger*> loggerInstances;
	static std::string	m_FileNamePrefix;
	static int 			m_FileSize;
	static int  		m_MaxQThreshold;
	static bool 		m_ZipFile;
	LogLevel            m_LogLevel;


};
long GetAvailableSpace(string path);
std::string getCurrentTimeMs();
std::string getTimestamp(struct timespec curtime);
struct timespec getCurrentTimeStruct();
void* FileZip(void *);


#endif // End of _AFNLOGGER_H_