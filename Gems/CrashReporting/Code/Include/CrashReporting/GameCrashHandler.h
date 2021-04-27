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

// LY Editor Crashpad Hook

#pragma once

#include <CrashHandler.h>

namespace CrashHandler
{
    class GameCrashHandler : public CrashHandler::CrashHandlerBase
    {
    public:
        GameCrashHandler() = default;
        virtual ~GameCrashHandler() = default;

        static void InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl = {}, const std::string& crashToken = {}, const std::string& handlerFolder = {}, const CrashHandlerAnnotations& baseAnnotations  = CrashHandlerAnnotations(), const CrashHandlerArguments& argumentVec = CrashHandlerArguments());
    protected:
        virtual const char* GetCrashHandlerExecutableName() const override;
        virtual std::string DetermineAppPath() const override;

        virtual std::string GetCrashSubmissionURL() const override;
        virtual std::string GetCrashSubmissionToken() const override;

        virtual std::string GetCrashHandlerPath(const std::string& lyAppRoot) const override;

    };

}
