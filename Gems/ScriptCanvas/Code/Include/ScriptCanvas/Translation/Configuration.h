/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            AZStd::string_view m_executionStateName;
            AZStd::string_view m_executionStateReferenceGraph;
            AZStd::string_view m_executionStateReferenceLocal;
            AZStd::string_view m_executionStateScriptCanvasIdName;
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
            // #scriptcanvas_component_extension
            AZStd::string_view m_executionStateEntityIdRefInitialization;
            AZStd::string_view m_executionStateEntityIdRef;
        };

    } 
} 
