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

#include <Source/Framework/ScriptCanvasTestVerify.h>

namespace ScriptCanvasTests
{
    ScriptCanvasEditor::UnitTestResult VerifyReporterEditor(const ScriptCanvasEditor::Reporter& reporter)
    {
        ScriptCanvasEditor::UnitTestResult result = ScriptCanvasEditor::UnitTestResult(false, true, "");

        if (!reporter.GetScriptCanvasId().IsValid())
        {
            result.m_consoleOutput += "Graph is not valid\n";
        }
        else if (reporter.IsReportFinished())
        {
            const auto& successes = reporter.GetSuccess();
            for (const auto& success : successes)
            {
                result.m_consoleOutput += "SUCCESS - ";
                result.m_consoleOutput += success.data();
                result.m_consoleOutput += "\n";
            }

            if (!reporter.IsActivated())
            {
                result.m_consoleOutput += "Graph did not activate\n";
            }

            if (!reporter.IsDeactivated())
            {
                result.m_consoleOutput += "Graph did not deactivate\n";
            }

            if (!reporter.IsErrorFree())
            {
                result.m_consoleOutput += "Graph had errors\n";
            }

            const auto& failures = reporter.GetFailure();
            for (const auto& failure : failures)
            {
                result.m_consoleOutput += "FAILURE - ";
                result.m_consoleOutput += failure.data();
                result.m_consoleOutput += "\n";
            }

            if (!reporter.IsComplete())
            {
                result.m_consoleOutput += "Graph was not marked complete\n";
            }

            const auto& checkpoints = reporter.GetCheckpoints();
            if (!checkpoints.empty())            
            {
                AZStd::string checkpointPath = "Checkpoint Path:\n";
                int i = 0;

                for (const auto& checkpoint : checkpoints)
                {
                    checkpointPath += AZStd::string::format("%2d: %s\n", ++i, checkpoint.data());
                }

                result.m_consoleOutput += checkpointPath.data();
            }

            if (reporter.IsComplete() && failures.empty())
            {
                result.m_consoleOutput += "COMPLETE!\n";
                result.m_success = true;

                return result;
            }

        }
        else
        {
            result.m_consoleOutput += "Graph report did not finish\n";
        }

        result.m_consoleOutput += "FAILED!\n";
        result.m_success = false;

        return result;
    }

} // namespace ScriptCanvasTests
