// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#include "SignalHandler.h"
#include "SignalHandlerMgr.h"

using namespace easynet;

void SignalHandler::close()
{
	signalHandlerMgr_->deleteSignalHandler(this);
}