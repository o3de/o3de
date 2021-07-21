/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
