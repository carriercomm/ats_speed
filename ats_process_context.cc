// Copyright 2013 We-Amp B.V.
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
//
// Author: oschaaf@we-amp.com (Otto van der Schaaf)
#include "ats_process_context.h"

#include "ats_rewrite_driver_factory.h"
#include "ats_server_context.h"
#include "ats_message_handler.h"
#include "ats_thread_system.h"

#include "net/instaweb/automatic/public/proxy_fetch.h"
#include "net/instaweb/util/public/pthread_shared_mem.h"

namespace net_instaweb {

AtsProcessContext::AtsProcessContext() {
  AtsThreadSystem* ts = new AtsThreadSystem();
  message_handler_.reset(new AtsMessageHandler(ts->NewMutex()));
  driver_factory_.reset(new AtsRewriteDriverFactory(ts));
  server_context_ = driver_factory()->MakeAtsServerContext();

  AtsRewriteOptions* root_options_ = (AtsRewriteOptions*)driver_factory_->default_options();
  AtsRewriteOptions* server_options = root_options_->Clone();
  AtsRewriteOptions* options = new AtsRewriteOptions(driver_factory_->thread_system());
  server_options->Merge(*options);
  delete options;

  server_context_->global_options()->Merge(*server_options);
  delete server_options;

  message_handler_->Message(kInfo,"global default options:\r\n[%s]",driver_factory_->default_options()->OptionsToString().c_str());
  message_handler_->Message(kInfo,"server ctx default options:\r\n[%s]",server_context_->global_options()->OptionsToString().c_str());
  Statistics* statistics =
      driver_factory_->MakeGlobalSharedMemStatistics(*(SystemRewriteOptions*)server_context_->global_options());
  AtsRewriteDriverFactory::InitStats(statistics);
    
  driver_factory()->RootInit();
  driver_factory()->ChildInit();

  proxy_fetch_factory_.reset(new ProxyFetchFactory(server_context_));
  message_handler_->Message(kInfo, "Process context constructed");
}

AtsProcessContext::~AtsProcessContext() {
}

}  // namespace net_instaweb
