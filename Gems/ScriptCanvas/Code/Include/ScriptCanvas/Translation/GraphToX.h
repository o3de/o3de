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

#pragma once

#include <AzCore/Outcome/Outcome.h>

#include "TranslationUtilities.h"

namespace ScriptCanvas
{
    class Graph;

    namespace Grammar
    {
        class AbstractCodeModel;
    }

    namespace Translation
    {
        struct Configuration
        {
            const char* m_blockCommentOpen = "";
            const char* m_namespaceClose = "";
            const char* m_namespaceOpen = "";
            const char* m_namespaceOpenPrefix = "";
            const char* m_scopeClose = "";
            const char* m_scopeOpen = "";
            const char* m_singleLineComment = "";
            const char* m_blockCommentClose = "";

        }; // struct Configuration

        // for functionality that is shared across translations in generic constructs
        // like scope, functions, constructors, destructors, variables, etc
        class GraphToX
        {
        protected:         
            const Grammar::AbstractCodeModel& m_model;
            const Configuration& m_configuration;
            AZ::Outcome<void, AZStd::string> m_outcome;

            GraphToX(const Configuration& configuration, const Grammar::AbstractCodeModel& model);
                        
            void CloseBlockComment(Writer& writer);
            void CloseScope(Writer& writer);
            void CloseNamespace(Writer& writer, const char* ns);
            const char* GetGraphName() const;
            const char* GetFullPath() const;
            // the model with have parsed the path into namespaces
            const char* GetPath(int depth) const;
            // this is the number of namespaces parsed by the model
            int GetPathDepth() const;
            void SingleLineComment(Writer& writer);
            void OpenBlockComment(Writer& writer);
            void OpenNamespace(Writer& writer, const char* ns);
            void OpenScope(Writer& writer);
            void WriteCopyright(Writer& writer);
            void WriteDoNotModify(Writer& writer);
        };
    } // namespace Translation

} // namespace ScriptCanvas