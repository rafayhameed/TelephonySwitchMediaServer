#include "AfnLogger.h"

Logger* Logger::m_Instance = 0;
std::map<std::string, Logger* > AfnLogger::loggerInstances = { { "0",0 } };
pthread_t FileZipThread;

#ifndef GZ_SUFFIX
#  define GZ_SUFFIX ".gz"
#endif
#define SUFFIX_LEN (sizeof(GZ_SUFFIX)-1)

#define DSK_SPACE_CHK_INT 60
#define BUFLEN      64 * 1024
#define MAX_NAME_LEN 1024
#define MAX_FILE_SIZE 80 * 1024 * 1024
#define MIN_FREE_DISK MAX_FILE_SIZE * 2

std::string	AfnLogger::m_FileNamePrefix = "afinitilog";
int 		AfnLogger::m_FileSize = MAX_FILE_SIZE;
int  		AfnLogger::m_MaxQThreshold = 10000;
bool 		AfnLogger::m_ZipFile = true;

AutoResetEvent autoresetevent(false);

void error(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
}

bool gz_compress(FILE *in, gzFile out)
{
	try
	{
		char buf[BUFLEN];
		int len;
		int err;

		for (;;)
		{
			len = (int)fread(buf, 1, sizeof(buf), in);
			if (ferror(in)) {
				perror("fread");
			}
			if (len == 0) break;

			if (gzwrite(out, buf, (unsigned)len) != len)
			{
				error(gzerror(out, &err));
				return false;
			}
		}
		fclose(in);
		if (gzclose(out) != Z_OK)
		{
			error("failed gzclose");
			return false;
		}
		return true;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > gz_compress() -- " << e.what() << " ] " << endl;
		return false;
	}
}

bool Logger::TryCreateFile()
{
	try
	{
		long space = 0;
		if ((space = GetAvailableSpace(m_LogPath)) >= m_MinFreeDisk)
		{
			m_FileName = getFileName();
			m_File.open(m_FileName.c_str(), std::ofstream::out | std::ofstream::trunc);
			return logFileOK = true;
		}
		else
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::TryCreateFile() -- There is not enough space on disk, Available space: " << (space >> 10) << " KB, Space required: " << (m_MinFreeDisk >> 10) << " KB ]" << endl;
			pthread_create(&CheckDiskSpaceThread, NULL, &Logger::CheckDiskSpaceHelper, this);
			pthread_detach(CheckDiskSpaceThread);
			return logFileOK = false;
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::TryCreateFile() -- " << e.what() << " ] " << endl;
		return false;
	}
}

string Logger::prepareLog(unsigned int threadId, struct timespec time, string func, string text, LogLevel loglvl, string ex, LogFormat logFormat) throw()
{
	try
	{
		string data;
		data.append(getTimestamp(time));
		data.append(" [");
		data.append(to_string(threadId));
		data.append("]");
		switch (loglvl)
		{
		case LOG_LEVEL_ERROR:
			data.append(" ERROR : [ ");
			break;
		case LOG_LEVEL_WARN:
			data.append(" WARN  : [ ");
			break;
		case LOG_LEVEL_INFO:
			data.append(" INFO  : [ ");
			break;
		case LOG_LEVEL_DEBUG:
			data.append(" DEBUG : [ ");
			break;
		case LOG_LEVEL_TRACE:
			data.append(" TRACE : [ ");
			break;
		}
		switch (logFormat)
		{
		case LOG_MSG:
			data.append(text);
			break;
		case LOG_FUNC_MSG:
			data.append(func);
			data.append(" > ");
			data.append(text);
			break;
		case LOG_FUNC_EXP:
			data.append(func);
			data.append(" > \n");
			data.append(ex);
			break;
		}
		data.append(" ]~~\n");
		return data;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::prepareLog() -- " << e.what() << " ] " << endl;
		return "";
	}
}

string Logger::ProcessLogData(LogData log)
{
	return prepareLog(log.l_threadId, log.l_time, log.l_funcName, log.l_msg, log.l_logLevel, log.l_exp, log.l_logFormat);
}

void * Logger::LogQProcessorHelper(void * object)
{
	try
	{
		return ((Logger *)object)->LogQProcessor();
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::LogQProcessorHelper() -- " << e.what() << " ] " << endl;
		return NULL;
	}
}
void * Logger::CheckDiskSpaceHelper(void * object)
{
	try
	{
		return ((Logger *)object)->CheckDiskSpace();
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > CheckDiskSpaceHelper() -- " << e.what() << " ] " << endl;
		return NULL;
	}
}

void *Logger::LogQProcessor()
{
	try
	{
		LogData log;
		while (m_Running)
		{
			autoresetevent.WaitOne();
			swapQueues();

			if (outQueue->empty())
			{
				//usleep(500);
				continue;
			}
			else
			{
				string logDump;
				while (!outQueue->empty())
				{
					log = outQueue->front();
					outQueue->pop();
					logDump.append(ProcessLogData(log));
				}
				logIntoFile(logDump);
			}
		}
		string logDump;
		while (!outQueue->empty())
		{
			log = outQueue->front();
			outQueue->pop();
			logDump.append(ProcessLogData(log));
		}
		logIntoFile(logDump);
		return NULL;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::LogQProcessor() -- " << e.what() << " ] " << endl;
		return NULL;
	}
}

void Logger::logIntoFile(std::string& data)
{
	try
	{
		if (!logFileOK)
		{
			return;
		}
		long strmPos = m_File.tellp();
		if (strmPos > m_FileSize)
		{
			m_File.close();
			if (m_ZipFile == true)
			{
				char *oldName = strdup(m_FileName.c_str());
				pthread_create(&FileZipThread, NULL, &FileZip, oldName);
				pthread_detach(FileZipThread);
			}
			if (TryCreateFile())
				strmPos = m_File.tellp();
			else
				return;
		}
		if (strmPos >= 0)
		{
			m_File << data;
			m_File.flush();
			streamOK = true;
		}
		else if (streamOK && m_Running)
		{
			streamOK = false;
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::logIntoFile() -- File stream for logging unavailable, Logger might have to be restarted to get the stream again ]" << endl;
			m_File.close();
			TryCreateFile();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::logIntoFile() -- " << e.what() << " ] " << endl;
		return;
	}
}

Logger::Logger()
{
	try
	{
		m_Running = false;
		streamOK = true;
		logFileOK = true;
		inQueue = &Queue1;
		outQueue = &Queue2;

		mkdir(LOG_PATH, 0777);
		zipExistingFiles();
		//string errpath = getErrorFileName();
		//errorOut.open(errpath.c_str(), ofstream::out | ofstream::trunc);
		TryCreateFile();

		m_LogLevel = LOG_LEVEL_TRACE;
		int ret = 0;
		pthread_mutexattr_init(&m_Attr);
		ret = pthread_mutexattr_settype(&m_Attr, PTHREAD_MUTEX_RECURSIVE);
		if (ret != 0)
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::Logger() -- Mutex attribute not initialized!! ]\n";
			exit(0);
		}
		ret = pthread_mutex_init(&m_Mutex, &m_Attr);
		if (ret != 0)
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::Logger() -- Mutex not initialized!! ]\n";
			exit(0);
		}
		m_Running = true;
		pthread_create(&LogQProcessorThread, NULL, &Logger::LogQProcessorHelper, this);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::Logger() -- " << e.what() << " ] " << endl;
		exit(0);
	}
}

Logger::Logger(string pathName, int queueThreshold, int fileSize, bool zipFile, std::string fileNamePrefix)
{
	try
	{
		m_Running = false;
		streamOK = true;
		logFileOK = true;
		inQueue = &Queue1;
		outQueue = &Queue2;

		m_LogPath = pathName;
		m_MaxQThreshold = queueThreshold;
		m_FileSize = fileSize;
		m_ZipFile = zipFile;
		m_MinFreeDisk = m_FileSize * 2;

		m_LogNameFormatStartup = fileNamePrefix + "_" + LOGNAME_FORMAT_STARTUP;
		m_LogNameFormat = fileNamePrefix + "_" + LOGNAME_FORMAT;

		zipExistingFiles();
		//string errpath = getErrorFileName();
		//errorOut.open(errpath.c_str(), ofstream::out | ofstream::trunc);
		TryCreateFile();

		m_LogLevel = LOG_LEVEL_TRACE;
		int ret = 0;
		pthread_mutexattr_init(&m_Attr);
		ret = pthread_mutexattr_settype(&m_Attr, PTHREAD_MUTEX_RECURSIVE);
		if (ret != 0)
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::Logger() -- Mutex attribute not initialized!! ]\n";
			exit(0);
		}
		ret = pthread_mutex_init(&m_Mutex, &m_Attr);
		if (ret != 0)
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::Logger() -- Mutex not initialized!! ]\n";
			exit(0);
		}
		m_Running = true;
		pthread_create(&LogQProcessorThread, NULL, &Logger::LogQProcessorHelper, this);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::Logger() -- " << e.what() << " ] " << endl;
		exit(0);
	}
}
Logger::~Logger()
{
	try
	{
		m_Running = false;
		autoresetevent.Set();
		m_File.close();
		if (m_ZipFile == true)
		{
			zipFile(m_FileName, "wb");
			std::remove(m_FileName.c_str());
		}
		pthread_mutex_destroy(&m_Mutex);
		pthread_cancel(LogQProcessorThread);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::~Logger() -- " << e.what() << " ] " << endl;
		exit(0);
	}
}
/*
Logger* Logger::getInstance() throw ()
{
try
{
if (m_Instance == 0)
{
m_Instance = new Logger();
cout<< "[ Afiniti Logger > getInstance() -- New Instance Created ]\n";
}
return m_Instance;
}
catch (std::exception& e)
{
cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > getInstance() -- " << e.what() << " ] " << endl;
return NULL;
}
}*/

void Logger::setMaxQThreshold(int val)
{
	try
	{
		m_MaxQThreshold = val;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::setMaxQThreshold() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::closeLogger()
{
	try
	{
		if (m_Instance != 0)
		{
			delete m_Instance;
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::closeLogger() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::lock()
{
	try
	{
		pthread_mutex_lock(&m_Mutex);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::lock() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::unlock()
{
	try
	{
		pthread_mutex_unlock(&m_Mutex);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::unlock() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::swapQueues()
{
	try
	{
		lock();
		std::queue<LogData> *temp;
		temp = inQueue;
		inQueue = outQueue;
		outQueue = temp;
		unlock();
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::swapQueues() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::closeFile()
{
	try
	{
		m_File.close();
		zipFile(m_FileName, "wb");
		std::remove(m_FileName.c_str());
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::closeFile() -- " << e.what() << " ] " << endl;
		return;
	}
}

string Logger::getFileName()
{
	try
	{
		string filename;
		char buf[200];
		//Current date/time based on current time
		time_t now = time(0);
		struct tm* tm_info;
		tm_info = localtime(&now);

		if (!m_Running)
		{
			strftime(buf, 200, m_LogNameFormatStartup.c_str(), tm_info);
		}
		else
		{
			strftime(buf, 200, m_LogNameFormat.c_str(), tm_info);
		}
		filename = m_LogPath;
		filename.append(buf);
		return filename;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::getFileName() -- " << e.what() << " ] " << endl;
		return "";
	}
}

string Logger::getErrorFileName()
{
	try
	{
		string filename;
		char buf[200];
		//Current date/time based on current time
		time_t now = time(0);
		struct tm* tm_info;
		tm_info = localtime(&now);
		strftime(buf, 200, ERRORLOG_FORMAT, tm_info);
		filename = m_LogPath;
		filename.append(buf);
		return filename;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::getErrorFileName() -- " << e.what() << " ] " << endl;
		return "";
	}
}

void Logger::error(string func, string text)
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_ERROR)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_ERROR, func, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::error", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::error() -- " << e.what() << " ] " << endl;
	}
}

void Logger::warn(string func, string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_WARN)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_WARN, func, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::warn", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::warn() -- " << e.what() << " ] " << endl;
	}
}

void Logger::info(string func, string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_INFO)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_INFO, func, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::info", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::info() -- " << e.what() << " ] " << endl;
	}
}

void Logger::debug(string func, string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_DEBUG)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_DEBUG, func, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::debug", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::debug() -- " << e.what() << " ] " << endl;
	}
}

void Logger::trace(string func, string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_TRACE)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_TRACE, func, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::trace", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::trace() -- " << e.what() << " ] " << endl;
	}
}

// LoggingFuncs with Exception Object
void Logger::error(string func, std::exception &ex)
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_ERROR)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_ERROR, func, ex);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::error", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::error() -- " << e.what() << " ] " << endl;
	}
}

void Logger::warn(string func, std::exception &ex) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_WARN)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_WARN, func, ex);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::warn", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::warn() -- " << e.what() << " ] " << endl;
	}
}

void Logger::info(string func, std::exception &ex) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_INFO)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_INFO, func, ex);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::info", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::info() -- " << e.what() << " ] " << endl;
	}
}

void Logger::debug(string func, std::exception &ex) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_DEBUG)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_DEBUG, func, ex);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::debug", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::debug() -- " << e.what() << " ] " << endl;
	}
}

void Logger::trace(string func, std::exception &ex) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_TRACE)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_TRACE, func, ex);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::trace", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::trace() -- " << e.what() << " ] " << endl;
	}
}

// LoggingFuncs with only String
void Logger::error(string text)
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_ERROR)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_ERROR, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::error", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::error() -- " << e.what() << " ] " << endl;
	}
}

void Logger::warn(string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_WARN)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_WARN, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::warn", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::warn() -- " << e.what() << " ] " << endl;
	}
}

void Logger::info(string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_INFO)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_INFO, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::info", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::info() -- " << e.what() << " ] " << endl;
	}
}

void Logger::debug(string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_DEBUG)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_DEBUG, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::debug", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::debug() -- " << e.what() << " ] " << endl;
	}
}

void Logger::trace(string text) throw()
{
	try
	{
		if (m_LogLevel >= LOG_LEVEL_TRACE)
		{
			LogData logData;
			logData.SetValues((unsigned int)pthread_self(), getCurrentTimeStruct(), LOG_LEVEL_TRACE, text);
			lock();
			if (inQueue->size() >= m_MaxQThreshold)
			{
				std::queue<LogData> empty;
				std::swap(*inQueue, empty);
				logData.SetValues(logData.l_threadId, logData.l_time, LOG_LEVEL_ERROR, "Logger::trace", "Discarding messages as queue is full");
			}
			inQueue->push(logData);
			unlock();
			autoresetevent.Set();
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::trace() -- " << e.what() << " ] " << endl;
	}
}

void Logger::zipFile(string filename, const char *mode)
{
	try
	{
		char * file = strdup(filename.c_str());
		char outfile[MAX_NAME_LEN];
		FILE *in;
		gzFile out;

		if (strlen(file) + strlen(GZ_SUFFIX) < sizeof(outfile))
		{
#if !defined(NO_snprintf) && !defined(NO_vsnprintf)
			snprintf(outfile, sizeof(outfile), "%s%s", file, GZ_SUFFIX);
#else
			strcpy(outfile, file);
			strcat(outfile, GZ_SUFFIX);
#endif

			in = fopen(file, "rb");
			if (in != NULL)
			{
				out = gzopen(outfile, mode);
				if (out != NULL) {
					if (gz_compress(in, out))
					{
						std::remove(filename.c_str());
					}
				}
				else
				{
					cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::zipFile() -- can't gzopen " << outfile << " ] " << endl;
				}
			}
			else
			{
				perror(file);
				cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::zipFile() -- Error In Opening File ]" << endl;
			}
		}
		else
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::zipFile() -- Filename too long ]" << endl;
		}
		free((char *)file);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::zipFile() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::zipExistingFiles()
{
	try
	{
		struct dirent *de;
		DIR *dr = opendir(m_LogPath.c_str());

		if (dr != NULL)
		{
			while ((de = readdir(dr)) != NULL)
			{
				if (strstr(de->d_name, ".gz") == NULL && strstr(de->d_name, ".log") != NULL) {
					string filename(m_LogPath);
					filename.append(de->d_name);
					zipFile(filename, "wb");
				}
			}
		}
		else
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::ZipExistingFiles() -- Could not open specified directory DIR: " << m_LogPath << " ] " << endl;
		}
		closedir(dr);
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::zipExistingFiles() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::updateLogLevel(LogLevel logLevel)
{
	try
	{
		m_LogLevel = logLevel;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::updateLogLevel() -- " << e.what() << " ] " << endl;
		return;
	}
}

void Logger::setParams(int queueThreshold, int fileSize, bool zipFile, string fileNamePrefix)
{
	try
	{
		m_MaxQThreshold = queueThreshold;
		m_FileSize = fileSize * 1024 * 1024;
		m_MinFreeDisk = m_FileSize * 2;
		m_ZipFile = zipFile;
		if (!fileNamePrefix.empty())
		{
			m_LogNameFormatStartup = fileNamePrefix + "_" + LOGNAME_FORMAT_STARTUP;
			m_LogNameFormat = fileNamePrefix + "_" + LOGNAME_FORMAT;
		}

	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::setParams() -- " << e.what() << " ] " << endl;
		return;
	}

}

AutoResetEvent::AutoResetEvent(bool initial)
	: flag_(initial)
{
}

void AutoResetEvent::Set()
{
	std::lock_guard<std::mutex> _(protect_);
	flag_ = true;
	signal_.notify_one();
}

void AutoResetEvent::Reset()
{
	std::lock_guard<std::mutex> _(protect_);
	flag_ = false;
}

bool AutoResetEvent::WaitOne()
{
	std::unique_lock<std::mutex> lk(protect_);
	while (!flag_)
		signal_.wait(lk);
	flag_ = false;
	return true;
}

LogData::LogData()
{}

LogData::LogData(unsigned int threadId, struct timespec  time, LogLevel loglevel, std::string funcname, std::string msg)
{
	try
	{
		l_threadId = threadId;
		l_time = time;
		l_logLevel = loglevel;
		l_funcName = funcname;
		l_msg = msg;
		l_logFormat = LOG_FUNC_MSG;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > LogData::LogData() -- " << e.what() << " ] " << endl;
		return;
	}
}

LogData::LogData(unsigned int threadId, struct timespec  time, LogLevel loglevel, std::string msg)
{
	try
	{
		l_threadId = threadId;
		l_time = time;
		l_logLevel = loglevel;
		l_msg = msg;
		l_logFormat = LOG_MSG;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > LogData::LogData() -- " << e.what() << " ] " << endl;
		return;
	}
}

LogData::LogData(unsigned int threadId, struct timespec  time, LogLevel loglevel, std::string funcname, std::exception &exp)
{
	try
	{
		l_threadId = threadId;
		l_time = time;
		l_logLevel = loglevel;
		l_funcName = funcname;
		l_exp = exp.what();
		l_logFormat = LOG_FUNC_EXP;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > LogData::LogData() -- " << e.what() << " ] " << endl;
		return;
	}
}

void LogData::SetValues(unsigned int threadId, struct timespec  time, LogLevel loglevel, std::string funcname, std::string msg)
{
	try
	{
		l_threadId = threadId;
		l_time = time;
		l_logLevel = loglevel;
		l_funcName = funcname;
		l_msg = msg;
		l_logFormat = LOG_FUNC_MSG;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > LogData::SetValues() -- " << e.what() << " ] " << endl;
		return;
	}
}

void LogData::SetValues(unsigned int threadId, struct timespec  time, LogLevel loglevel, std::string msg)
{
	try
	{
		l_threadId = threadId;
		l_time = time;
		l_logLevel = loglevel;
		l_msg = msg;
		l_logFormat = LOG_MSG;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > LogData::SetValues() -- " << e.what() << " ] " << endl;
		return;
	}
}

void LogData::SetValues(unsigned int threadId, struct timespec  time, LogLevel loglevel, std::string funcname, std::exception &exp)
{
	try
	{
		l_threadId = threadId;
		l_time = time;
		l_logLevel = loglevel;
		l_funcName = funcname;
		l_exp = exp.what();
		l_logFormat = LOG_FUNC_EXP;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > LogData::SetValues() -- " << e.what() << " ] " << endl;
		return;
	}
}
//AfnLogger::loggerInstances
Logger* AfnLogger::getInstance(string path)
{
	try
	{
		system(("mkdir -p " + path).c_str());
		if (loggerInstances.find(path) != loggerInstances.end())
		{
			return loggerInstances[path];
		}
		loggerInstances.insert(pair<string, Logger*>(path, new Logger(path, m_MaxQThreshold, m_FileSize, m_ZipFile, m_FileNamePrefix)));
		return loggerInstances[path];
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > AfnLogger::getInstance() -- " << e.what() << " ] " << endl;
		return NULL;
	}
}

void AfnLogger::setParams(int queueThreshold, int fileSize, bool zipFile, string fileNamePrefix)
{
	try
	{
		m_MaxQThreshold = queueThreshold;
		m_FileSize = fileSize * 1024 * 1024;
		m_ZipFile = zipFile;
		m_FileNamePrefix = fileNamePrefix;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > AfnLogger::setParams() -- " << e.what() << " ] " << endl;
		return;
	}
}

bool AfnLogger::closeLogger(string path)
{
	try
	{
		if (loggerInstances.find(path) != loggerInstances.end())
		{
			delete loggerInstances[path];
			loggerInstances.erase(path);
			return true;
		}

		return false;
		//loggerInstances.clear();
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > AfnLogger::closeLogger() -- " << e.what() << " ] " << endl;
		return false;
	}
}

bool AfnLogger::closeLogger(Logger* logInst)
{
	try
	{
		for (std::map<string, Logger*>::iterator it = loggerInstances.begin(); it != loggerInstances.end(); ++it)
		{
			if (it->second == logInst)
			{
				delete it->second;
				loggerInstances.erase(it);
				return true;
			}
		}
		return false;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > AfnLogger::closeLogger() -- " << e.what() << " ] " << endl;
		return false;
	}
}

void AfnLogger::closeAllLoggers()
{
	try
	{
		for (std::map<string, Logger*>::iterator it = loggerInstances.begin(); it != loggerInstances.end(); ++it)
		{
			if (it->second != 0)
				delete it->second;
		}
		loggerInstances.clear();
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > AfnLogger::closeAllLoggers() -- " << e.what() << " ] " << endl;
		return;
	}
}

string getCurrentTimeMs()
{
	try
	{
		char buffer[30];
		struct timeval tv;
		time_t curtime;

		gettimeofday(&tv, NULL);
		curtime = tv.tv_sec;

		strftime(buffer, 30, "%F %T,", localtime(&curtime));

		string currTime = buffer;
		snprintf(buffer, 4, "%03ld", (tv.tv_usec));
		currTime.append(buffer);

		return currTime;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > getCurrentTimeMs() -- " << e.what() << " ] " << endl;
		return "";
	}
}

void* FileZip(void * arg)
{
	try
	{
		string filename = string((char*)arg);
		Logger::zipFile(filename, "wb");
		free((char*)arg);
	}
	catch (exception &e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << "[ Afiniti Logger > FileZip() -- " << e.what() << " ] " << endl;
	}
	return NULL;
}

long GetAvailableSpace(string path)
{
	try
	{
		struct statvfs stat;

		if (statvfs(path.c_str(), &stat) != 0)
		{
			cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > GetAvailableSpace() -- Call to statvfs() failed ] " << endl;
			return -1;
		}
		return stat.f_bsize * stat.f_bavail;
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > GetAvailableSpace() -- " << e.what() << " ] " << endl;
		return -1;
	}
}

void *Logger::CheckDiskSpace()
{
	try
	{
		int sleepTime = 0;
		long space = 0;
		while (m_Running)
		{
			if (sleepTime >= DSK_SPACE_CHK_INT)
			{
				sleepTime = 0;
				if ((space = GetAvailableSpace(m_LogPath)) >= m_MinFreeDisk)
				{
					m_FileName = getFileName();
					m_File.open(m_FileName.c_str(), std::ofstream::out | std::ofstream::trunc);
					logFileOK = true;
					break;
				}
				else
				{
					cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > Logger::CheckDiskSpace() -- There is not enough space on disk, Available space: " << (space >> 10) << " KB, Space required: " << (m_MinFreeDisk >> 10) << " KB ] " << endl;
				}
			}
			sleep(1);
			sleepTime++;
		}
	}
	catch (std::exception& e)
	{
		cerr << getTimestamp(getCurrentTimeStruct()) << " [ Afiniti Logger > CheckDiskSpace() -- " << e.what() << " ] " << endl;
	}
	return NULL;
}

struct timespec getCurrentTimeStruct()
{
	struct timespec curtime = { 0 };
	try
	{
		clock_gettime(CLOCK_REALTIME, &curtime);
		return curtime;
	}
	catch (std::exception& e)
	{
		cerr << " [ Afiniti Logger > getCurrentTimeStruct() -- " << e.what() << " ] " << endl;
		return curtime;
	}
}


string getTimestamp(struct timespec curtime)
{
	try
	{
		time_t seconds;
		char timestring[32];
		char   timebuffer[32] = { 0 };
		char millisec[4];
		struct tm       gmtval = { 0 };

		// Set the fields
		seconds = curtime.tv_sec;
		snprintf(millisec, 4, "%03ld", curtime.tv_nsec);
		gmtime_r(&seconds, &gmtval);
		strftime(timebuffer, sizeof timebuffer, "%Y-%m-%d %H:%M:%S", &gmtval);
		snprintf(timestring, sizeof timestring, "%s,%s", timebuffer, millisec);

		string Ttime = timestring;
		return Ttime;
	}
	catch (std::exception& e)
	{
		cerr << " [ Afiniti Logger > getTimestamp() -- " << e.what() << " ] " << endl;
		return "";
	}

}