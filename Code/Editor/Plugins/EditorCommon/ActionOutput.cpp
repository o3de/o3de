/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorCommon_precompiled.h"
#include <ActionOutput.h>
#include <QWidget>

namespace AZ
{
    ActionOutput::ActionOutput()
        : m_errorCount(0)
        , m_warningCount(0)
    {
    }

    void ActionOutput::AddError(const AZStd::string& error)
    {
        AddError(error, "");
    }

    void ActionOutput::AddError(const AZStd::string& error, const AZStd::string& details)
    {
        m_errorToDetails[error].push_back(details);
        ++m_errorCount;
    }

    bool ActionOutput::HasAnyErrors() const
    {
        return m_errorCount > 0;
    }

    AZStd::string ActionOutput::BuildErrorMessage() const
    {
        return BuildMessage(m_errorToDetails);
    }

    void ActionOutput::AddWarning(const AZStd::string& warning)
    {
        AddWarning(warning, "");
    }

    void ActionOutput::AddWarning(const AZStd::string& warning, const AZStd::string& details)
    {
        m_warningToDetails[warning].push_back(details);
        ++m_warningCount;
    }

    bool ActionOutput::HasAnyWarnings() const
    {
        return m_warningCount > 0;
    }

    AZStd::string ActionOutput::BuildWarningMessage() const
    {
        return BuildMessage(m_warningToDetails);
    }

    AZStd::string ActionOutput::BuildMessage(const IssueToDetails& issues) const
    {
        AZStd::string message;
        for (const auto& it : issues)
        {
            message += it.first;
            message += ":\n";
            const DetailList& details = it.second;
            for (size_t i = 0; i < details.size(); ++i)
            {
                message += "    ";
                message += details[i];
                message += "\n";
            }

            message += "\n";
        }

        return message;
    }
}
