// Copyright 2017, Shenghua Fang. All rights reserved.
// Use of this source code is governed by a BSD 2-Clause license that can be found in the License file.
// Author: Shenghua Fang

#ifndef _EASYNET_RLIMITE_H_
#define _EASYNET_RLIMITE_H_

#include <unistd.h>

namespace easynet
{

int getRlimit(int resource, size_t &softLimit, size_t &hardLimit);
int setRlimit(int resource, size_t softLimit, size_t hardLimit);

} // namespace easynet


#endif
 