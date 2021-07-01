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
    class ToolsCrashHandler : public CrashHandler::CrashHandlerBase
    {
    public:
        ToolsCrashHandler() = default;
        virtual ~ToolsCrashHandler() = default;

        static void InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl = {}, const std::string& crashToken = {}, const std::string& handlerFolder = {}, const CrashHandlerAnnotations& baseAnnotations = CrashHandlerAnnotations(), const CrashHandlerArguments& arguments = CrashHandlerArguments());
    protected:

        virtual std::string DetermineAppPath() const override;

        virtual std::string GetCrashSubmissionURL() const override;
        virtual std::string GetCrashSubmissionToken() const override;

        // OS Dependent
        virtual std::string GetAppRootFromCWD() const override;
        virtual void GetOSAnnotations(CrashHandlerAnnotations& annotations) const override;
    };

}
