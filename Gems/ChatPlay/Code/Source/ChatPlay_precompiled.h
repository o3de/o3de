/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or 
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,  
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
*
*/

#pragma once

#include <platform.h> // Many CryCommon files require that this is included first.
#include <algorithm>
#include <vector>
#include <memory>
#include <list>
#include <functional>
#include <limits>

#include <CryName.h>
#include <ISystem.h>
#include <IConsole.h>
#include <ILog.h>
#include <ISerialize.h>

#ifdef GetObject
#undef GetObject
#endif
