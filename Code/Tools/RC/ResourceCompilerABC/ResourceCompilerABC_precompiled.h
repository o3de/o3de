/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//


#pragma once

// When STD disables exceptions, it moves "exception" to stdext
// When this happens, alembic ends up with an underfined symbol : void Alembic::Abc::v11::ErrorHandler::operator()(class stdext::exception&, ...
// Since alembic was compiled with exceptions enabled, (instead of compile it with - D_HAS_EXCEPTIONS = 0), we need to here enable it so that
// symbol doesnt get mixed up
#undef _HAS_EXCEPTIONS
#define _HAS_EXCEPTIONS 1

#include <assert.h>

#define CRY_ASSERT(condition) assert(condition)
#define CRY_ASSERT_TRACE(condition, message) assert(condition)
#define CRY_ASSERT_MESSAGE(condition, message) assert(condition)

// Define this to prevent including CryAssert (there is no proper hook for turning this off, like the above).
#define CRYINCLUDE_CRYCOMMON_CRYASSERT_H

#include <platform.h>

#include <functional>
#include <vector>
#include <map>

#include <stdio.h>

#include <Cry_Math.h>
#include <Cry_Geo.h>

#include "CryFile.h"

#include "IXml.h"

#include "IRCLog.h"

#include <VertexFormats.h>

AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
#include <Alembic/Abc/All.h>
AZ_POP_DISABLE_WARNING

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreFactory/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/Util/All.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/base.h>

#include "SwapEndianness.h"
