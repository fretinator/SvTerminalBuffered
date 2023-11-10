#ifndef DEBUG_UTIL_H
#define DEBUG_UTIL_H

#include <Arduino.h>

enum DEBUG_LEVEL {
	INFO = 0,
	DEBUG = 1,
	WARN = 2,
	ERROR = 3
};

class DebugUtil;

class DebugUtil {
private:
	DEBUG_LEVEL level = WARN;
  Stream* output;

public:
	DebugUtil(Stream* output);
	DebugUtil(DEBUG_LEVEL level, Stream* output);
	void setDebugLevel(DEBUG_LEVEL level);
	void error(String msg, boolean newLine);
  void warn(String msg, boolean newLine);
  void debug(String msg, boolean newLine);
  void info(String msg, boolean newLine);
	void error(String msg);
  void warn(String msg);
  void debug(String msg);
  void info(String msg);  
};

#endif
