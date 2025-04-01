/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GraphToX.h"

#include <ctime>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

#include "Grammar/PrimitivesExecution.h"
#include "Grammar/ParsingUtilities.h"

namespace ScriptCanvas
{
    namespace Translation
    {
        GraphToX::GraphToX(const Configuration& configuration, const Grammar::AbstractCodeModel& model)
            : m_model(model)
            , m_configuration(configuration)
        {}

        void GraphToX::AddError(Grammar::ExecutionTreeConstPtr execution, ValidationConstPtr error)
        {
            if (execution)
            {
                AZStd::string pretty;
                Grammar::PrettyPrint(pretty, execution->GetRoot(), execution);
                AZ_TracePrintf("ScriptCanvas", pretty.c_str());
            }

            m_errors.push_back(error);
        }

        AZStd::string GraphToX::AddMultiReturnName()
        {
            ++m_multiReturnCount;
            return GetMultiReturnName();
        }

        void GraphToX::CloseBlockComment(Writer& writer)
        {
            writer.WriteLineIndented(m_configuration.m_blockCommentClose);
        }

        void GraphToX::CloseFunctionBlock(Writer& writer)
        {
            writer.Outdent();
            writer.WriteLineIndented(m_configuration.m_functionBlockClose);
        }

        void GraphToX::CloseNamespace(Writer& writer, AZStd::string_view ns)
        {
            writer.Outdent();
            writer.WriteIndent();
            writer.Write(m_configuration.m_namespaceClose);
            writer.WriteSpace();
            SingleLineComment(writer);
            writer.WriteSpace();
            writer.Write(m_configuration.m_namespaceOpenPrefix);
            writer.WriteSpace();
            writer.Write(ns);
            writer.WriteNewLine();
        }

        void GraphToX::CloseScope(Writer& writer)
        {
            writer.Outdent();
            writer.WriteLineIndented(m_configuration.m_scopeClose);
        }

        AZStd::string_view GraphToX::GetGraphName() const
        {
            return m_model.GetSource().m_name;
        }

        AZStd::string_view GraphToX::GetFullPath() const
        {
            return m_model.GetSource().m_path;
        }

        AZStd::string GraphToX::GetMultiReturnName() const
        {
            return AZStd::string::format("multiReturn_%d", m_multiReturnCount);
        }

        AZStd::sys_time_t GraphToX::GetTranslationDuration() const
        {
            return m_translationDuration;
        }

        bool GraphToX::IsSuccessfull() const
        {
            return m_errors.empty();
        }

        void GraphToX::MarkTranslationStart()
        {
            m_translationStartTime = AZStd::chrono::steady_clock::now();
        }

        void GraphToX::MarkTranslationStop()
        {
            m_translationDuration = AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(AZStd::chrono::steady_clock::now() - m_translationStartTime).count();
        }

        AZStd::vector<ValidationConstPtr>&& GraphToX::MoveErrors()
        {
            return AZStd::move(m_errors);
        }

        void GraphToX::OpenBlockComment(Writer& writer)
        {
            writer.WriteIndent();
            writer.WriteLine(m_configuration.m_blockCommentOpen);
        }

        void GraphToX::OpenFunctionBlock(Writer& writer)
        {
            writer.WriteLineIndented(m_configuration.m_functionBlockOpen);
            writer.Indent();
        }

        void GraphToX::OpenNamespace(Writer& writer, AZStd::string_view ns)
        {
            writer.WriteIndent();
            writer.Write(m_configuration.m_namespaceOpenPrefix);
            writer.WriteSpace();
            writer.Write(ns);
            writer.WriteNewLine();
            writer.Write(m_configuration.m_namespaceOpen);
            writer.WriteNewLine();
            writer.Indent();
        }

        void GraphToX::OpenScope(Writer& writer)
        {
            writer.WriteLineIndented(m_configuration.m_scopeOpen);
            writer.Indent();
        }

        AZStd::string GraphToX::ResolveScope(const AZStd::vector<AZStd::string>& namespaces)
        {
            AZStd::string resolution;

            if (!namespaces.empty())
            {
                resolution = Grammar::ToIdentifier(namespaces[0]);

                for (size_t index = 1; index < namespaces.size(); ++index)
                {
                    resolution += m_configuration.m_lexicalScopeDelimiter;
                    resolution += Grammar::ToIdentifier(namespaces[index]);
                }
            }

            return resolution;
        }

        void GraphToX::SingleLineComment(Writer& writer)
        {
            writer.Write(m_configuration.m_singleLineComment);
        }

        void GraphToX::WriteCopyright(Writer& writer)
        {
            OpenBlockComment(writer);
            writer.WriteLine(GetCopyright());
            CloseBlockComment(writer);
        }

        void GraphToX::WriteDoNotModify(Writer& writer)
        {
            OpenBlockComment(writer);
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteNewLine();
            writer.WriteLine(GetDoNotModifyCommentText());
            writer.WriteNewLine();
            writer.WriteLine("GRAPH NAME: %s", GetGraphName().data());
            writer.WriteLine("FULL PATH: %s", GetFullPath().data());
            WriteLastWritten(writer);
            writer.WriteNewLine();
            writer.WriteLine(GetDoNotModifyCommentText());
            writer.WriteNewLine();
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            CloseBlockComment(writer);
        }

        void GraphToX::WriteLastWritten(Writer& writer)
        {
            char buffer[256];
            time_t t;
            time(&t);

#if !defined(_MSC_VER)
            tm* result = std::localtime(&t);
            strftime(buffer, sizeof(buffer), "%H:%M:%S %m-%d-%Y", result);
#else
            tm result;
            localtime_s(&result, &t);
            strftime(buffer, sizeof(buffer), "%H:%M:%S %m-%d-%Y", &result);
#endif
            writer.Write("Last written: ");
            writer.WriteLine(buffer);
        }
    }
}
