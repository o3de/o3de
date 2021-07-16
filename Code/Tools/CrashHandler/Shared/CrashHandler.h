/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Crashpad Hook

#pragma once

#include <map>
#include <string>
#include <vector>

namespace CrashHandler
{
    static const char* defaultCrashFolder = "CrashDB/";
    static const char* O3DEProductName = "O3DE";

    using CrashHandlerAnnotations = std::map<std::string, std::string>;
    using CrashHandlerArguments = std::vector<std::string>;

    class CrashHandlerBase
    {
    public:
        CrashHandlerBase() = default;
        virtual ~CrashHandlerBase() = default;

        static void InitCrashHandler(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl = {}, const std::string& crashToken = {}, const std::string& handlerFolder = {}, const CrashHandlerAnnotations& baseAnnotations = CrashHandlerAnnotations(), const CrashHandlerArguments& argumentVec = CrashHandlerArguments());

        void Initialize(const std::string& moduleTag, const std::string& devRoot);
        void Initialize(const std::string& moduleTag, const std::string& devRoot, const std::string& crashUrl, const std::string& crashToken, const std::string& handlerFolder = {}, const CrashHandlerAnnotations& baseAnnotations = CrashHandlerAnnotations(), const CrashHandlerArguments& argumentVec = CrashHandlerArguments());

        // Helper to add an annotation after initialization - must have already called InitCrashHandler
        static void AddAnnotation(const std::string& keyName, const std::string& valueStr);
    protected:
        virtual std::string GetCrashReportFolder(const std::string& lyAppRoot) const;
        virtual const char* GetDefaultCrashFolder() const { return defaultCrashFolder; }

        virtual std::string GetCrashHandlerPath(const std::string& lyAppRoot = {}) const;
        virtual const char* GetCrashHandlerExecutableName() const;

        virtual std::string DetermineAppPath() const;

        virtual const char* GetProductName() const { return O3DEProductName; }

        virtual bool CreateCrashHandlerDB(const std::string& reportPath) const;

        virtual std::string GetCrashSubmissionURL() const { return{}; }
        virtual std::string GetCrashSubmissionToken() const { return{}; }

        static void AppendSep(std::string& lyAppRoot);

        virtual void GetBuildAnnotations(CrashHandlerAnnotations& annotations) const;

        virtual void GetUserAnnotations(CrashHandlerAnnotations& ) const {};
        // OS Dependent
        virtual std::string GetAppRootFromCWD() const;
        virtual void GetOSAnnotations(CrashHandlerAnnotations& annotations) const;
        const std::string& GetConfigSubmissionToken() const;
    private:
        void ReadConfigFile();
        std::string m_submissionToken;
    };

}
