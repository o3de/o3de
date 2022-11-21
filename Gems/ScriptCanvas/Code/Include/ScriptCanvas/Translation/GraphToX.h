/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/chrono/chrono.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>

#include "Configuration.h"
#include "Grammar/Primitives.h"
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
        // for functionality that is shared across translations in generic constructs
        // like scope, functions, constructors, destructors, variables, etc
        class GraphToX
        {
        public:
            bool IsSuccessfull() const;

        protected:
            const Grammar::AbstractCodeModel& m_model;
            const Configuration m_configuration;
            AZ::s32 m_multiReturnCount = 0;

            GraphToX(const Configuration& configuration, const Grammar::AbstractCodeModel& model);

            void AddError(Grammar::ExecutionTreeConstPtr execution, ValidationConstPtr error);
            AZStd::string AddMultiReturnName();
            AZStd::string GetMultiReturnName() const;
            void CloseBlockComment(Writer& writer);
            void CloseFunctionBlock(Writer& writer);
            void CloseScope(Writer& writer);
            void CloseNamespace(Writer& writer, AZStd::string_view ns);
            AZStd::string_view GetGraphName() const;
            AZStd::string_view GetFullPath() const;
            AZStd::sys_time_t GetTranslationDuration() const;
            AZStd::vector<ValidationConstPtr>&& MoveErrors();
            void OpenBlockComment(Writer& writer);
            void OpenFunctionBlock(Writer& writer);
            void OpenNamespace(Writer& writer, AZStd::string_view ns);
            void OpenScope(Writer& writer);
            AZStd::string ResolveScope(const AZStd::vector<AZStd::string>& namespaces);
            void SingleLineComment(Writer& writer);
            void WriteCopyright(Writer& writer);
            void WriteDoNotModify(Writer& writer);
            void WriteLastWritten(Writer& writer);

        protected:
            void MarkTranslationStart();
            void MarkTranslationStop();

        private:
            AZStd::vector<ValidationConstPtr> m_errors;
            AZStd::sys_time_t m_translationDuration;
            AZStd::chrono::steady_clock::time_point m_translationStartTime;
        };
    }

}
