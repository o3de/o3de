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

#include "precompiled.h"
#include "GraphToX.h"

#include <ScriptCanvas/Grammar/AbstractCodeModel.h>

namespace ScriptCanvas
{
    namespace Translation
    {
        GraphToX::GraphToX(const Configuration& configuration, const Grammar::AbstractCodeModel& model)
            : m_model(model)
            , m_configuration(configuration)
            , m_outcome(AZ::Success())
        {}

        void GraphToX::CloseBlockComment(Writer& writer)
        {
            writer.WriteIndent();
            writer.WriteLine(m_configuration.m_blockCommentClose);
        }

        void GraphToX::CloseNamespace(Writer& writer, const char* ns)
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
            writer.WriteIndent();
            writer.WriteLine(m_configuration.m_scopeClose);
        }

        const char* GraphToX::GetGraphName() const
        {
            // finish me!
            return m_model.GetSource().m_name.c_str();
        }
        
        const char* GraphToX::GetFullPath() const
        {
            return m_model.GetSource().m_path.c_str();
        }

        void GraphToX::SingleLineComment(Writer& writer)
        {
            writer.Write(m_configuration.m_singleLineComment);
        }
        
        void GraphToX::OpenBlockComment(Writer& writer)
        {
            writer.WriteIndent();
            writer.WriteLine(m_configuration.m_blockCommentOpen);
        }
        
        void GraphToX::OpenNamespace(Writer& writer, const char* ns)
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
            writer.WriteIndent();
            writer.WriteLine(m_configuration.m_scopeOpen);
            writer.Indent();
        }

        void GraphToX::WriteCopyright(Writer& writer)
        {
            OpenBlockComment(writer);
            writer.WriteLine(GetAmazonCopyright());
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
            writer.WriteLine("GRAPH NAME: %s", GetGraphName());
            writer.WriteNewLine();
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            writer.WriteLine("***********************************************************************************");
            CloseBlockComment(writer);
        }
    } // namespace Translation
} // namespace ScriptCanvas