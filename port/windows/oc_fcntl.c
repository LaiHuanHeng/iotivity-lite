/****************************************************************************
 *
 * Copyright (c) 2023 plgd.dev s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"),
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#include "port/common/posix/oc_fcntl_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>

bool
oc_fcntl_set_blocking(SOCKET sock)
{
  unsigned long mode = 0;
  return ioctlsocket(sock, FIONBIO, &mode) == NO_ERROR;
}

bool
oc_fcntl_set_nonblocking(SOCKET sock)
{
  unsigned long mode = 1;
  return ioctlsocket(sock, FIONBIO, &mode) == NO_ERROR;
}