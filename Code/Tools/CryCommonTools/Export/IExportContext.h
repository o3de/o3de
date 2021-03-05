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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IEXPORTCONTEXT_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IEXPORTCONTEXT_H
#pragma once


#include <cstdarg>
#include "Exceptions.h"
#include "ILogger.h"

struct IPakSystem;
class ISettings;

class IExportContext
    : public ILogger
{
public:
    // Declare an exception type to report the case where the scene must be saved before exporting.
    struct NeedSaveErrorTag {};
    typedef Exception<NeedSaveErrorTag> NeedSaveError;
    struct PakSystemErrorTag {};
    typedef Exception<PakSystemErrorTag> PakSystemError;

    virtual void SetProgress(float progress) = 0;
    virtual void SetCurrentTask(const std::string& id) = 0;
    virtual IPakSystem* GetPakSystem() = 0;
    virtual ISettings* GetSettings() = 0;
    virtual void GetRootPath(char* buffer, int bufferSizeInBytes) = 0;

protected:
    // ILogger
    virtual void LogImpl(ILogger::ESeverity eSeverity, const char* message) = 0;
};

struct CurrentTaskScope
{
    CurrentTaskScope(IExportContext* context, const std::string& id)
        : context(context) {context->SetCurrentTask(id); }
    ~CurrentTaskScope() {context->SetCurrentTask(""); }
    IExportContext* context;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IEXPORTCONTEXT_H
