#include "DebugUtil.h"

DebugUtil::DebugUtil(Stream* output) {
	level = WARN;
  this->output = output;
}

DebugUtil::DebugUtil(DEBUG_LEVEL level, Stream* output) {
	this->level = level;
  this->output = output;
}

void DebugUtil::setDebugLevel(DEBUG_LEVEL level) {
	this->level = level;
}

void DebugUtil::error(String msg, boolean newLine) {
  if(level <= ERROR) {
    output->print(msg);
    
    if(newLine) {
      output->println();
    }
  }
}

void DebugUtil::warn(String msg, boolean newLine) {
  if(level <= WARN) {
    output->print(msg);
    
    if(newLine) {
      output->println();
    }
  }
}

void DebugUtil::debug(String msg, boolean newLine) {
  if(level <= DEBUG) {
    output->print(msg);
    
    if(newLine) {
      output->println();
    }
  }
}

void DebugUtil::info(String msg, boolean newLine) {
  if(level <= INFO) {
    output->print(msg);
    
    if(newLine) {
      output->println();
    }
  }
}

void DebugUtil::error(String msg) {
  error(msg, true);
}

void DebugUtil::warn(String msg) {
  warn(msg, true);
}

void DebugUtil::debug(String msg) {
  debug(msg, true);
}

void DebugUtil::info(String msg) {
  info(msg, true);
}
