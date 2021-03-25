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

#include "Grammar/Primitives.h"
#include "TranslationUtilities.h"
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Translation/Configuration.h>

namespace ScriptCanvas
{
    namespace Translation
    {
        struct Configuration
        {
            AZStd::string_view m_blockCommentClose;
            AZStd::string_view m_blockCommentOpen;
            AZStd::string_view m_dependencyDelimiter;
            AZStd::string_view m_executionStateEntityIdName;
            AZStd::string_view m_executionStateEntityIdRef;
            AZStd::string_view m_executionStateName;
            AZStd::string_view m_executionStateReference;
            AZStd::string_view m_executionStateScriptCanvasIdName;
            AZStd::string_view m_executionStateScriptCanvasIdRef;
            AZStd::string_view m_functionBlockClose;
            AZStd::string_view m_functionBlockOpen;
            AZStd::string_view m_lexicalScopeDelimiter;
            AZStd::string_view m_lexicalScopeVariable;
            AZStd::string_view m_namespaceClose;
            AZStd::string_view m_namespaceOpen;
            AZStd::string_view m_namespaceOpenPrefix;
            AZStd::string_view m_scopeClose;
            AZStd::string_view m_scopeOpen;
            AZStd::string_view m_singleLineComment;
            AZStd::string_view m_suffix;
        };

    } 
} 
