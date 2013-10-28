// Copyright 2013 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Author: oschaaf@gmail.com (Otto van der Schaaf)

#include "ats_message_handler.h"

#include <signal.h>

#include "apr_time.h"

#include "net/instaweb/util/public/abstract_mutex.h"
#include "net/instaweb/util/public/debug.h"
#include "net/instaweb/util/public/shared_circular_buffer.h"
#include "net/instaweb/util/public/string_util.h"
#include "net/instaweb/public/version.h"

namespace {

// This will be prefixed to every logged message.
const char kModuleName[] = "ats_pagespeed";

}  // namespace

namespace net_instaweb {

AtsMessageHandler::AtsMessageHandler(AbstractMutex* mutex)
    : mutex_(mutex),
      buffer_(NULL) {
  SetPidString(static_cast<int64>(getpid()));
}


bool AtsMessageHandler::Dump(Writer* writer) {
  // Can't dump before SharedCircularBuffer is set up.
  if (buffer_ == NULL) {
    return false;
  }
  return buffer_->Dump(writer, &handler_);
}

void AtsMessageHandler::set_buffer(SharedCircularBuffer* buff) {
  ScopedMutex lock(mutex_.get());
  buffer_ = buff;
}

void AtsMessageHandler::MessageVImpl(MessageType type, const char* msg,
                                     va_list args) {
  GoogleString formatted_message = Format(msg, args);
  
  TSDebug("ats-speed", "[%s %s] %s", kModuleName, kModPagespeedVersion,
          formatted_message.c_str());
 
  // Prepare a log message for the SharedCircularBuffer only.
  // Prepend time and severity to message.
  // Format is [time] [severity] [pid] message.
  GoogleString message;
  char time_buffer[APR_CTIME_LEN + 1];
  const char* time = time_buffer;
  apr_status_t status = apr_ctime(time_buffer, apr_time_now());
  if (status != APR_SUCCESS) {
    time = "?";
  }
  StrAppend(&message, "[", time, "] ",
            "[", MessageTypeToString(type), "] ");
  StrAppend(&message, pid_string_, " ", formatted_message, "\n");
  {
    ScopedMutex lock(mutex_.get());
    if (buffer_ != NULL) {
      buffer_->Write(message);
    }
  }
}

void AtsMessageHandler::FileMessageVImpl(MessageType type, const char* file,
                                         int line, const char* msg,
                                         va_list args) {
  GoogleString formatted_message = Format(msg, args);
  TSDebug("ats-speed", "[%s %s] %s:%d:%s",
                kModuleName, kModPagespeedVersion, file, line,
                formatted_message.c_str());
}

// TODO(sligocki): It'd be nice not to do so much string copying.
GoogleString AtsMessageHandler::Format(const char* msg, va_list args) {
  GoogleString buffer;

  // Ignore the name of this routine: it formats with vsnprintf.
  // See base/stringprintf.cc.
  StringAppendV(&buffer, msg, args);
  return buffer;
}

}  // namespace net_instaweb
