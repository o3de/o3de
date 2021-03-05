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

// Description : Based on DebugCallStack code.


#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CRASHHANDLER_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CRASHHANDLER_H
#pragma once

#include <AzCore/Debug/TraceMessageBus.h>

//!============================================================================
//!
//! CrashHandler class
//! writes minidump files.
//!
//!============================================================================
class CrashHandler : AZ::Debug::TraceMessageBus::Handler
{
public:
    CrashHandler();
    ~CrashHandler();

    bool OnException(const char* message) override;

    void SetDumpFile(const char* dumpFilename);

private:
    void WriteMinidump();

    char m_dumpFilename[MAX_PATH];
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_CRASHHANDLER_H
