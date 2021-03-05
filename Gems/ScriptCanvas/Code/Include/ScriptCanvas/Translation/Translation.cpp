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
#include "Translation.h"

#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Translation/GraphToCPlusPlus.h>
#include <ScriptCanvas/Translation/GraphToLua.h>

namespace TranslationCPP
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Translation;

    AZ::Outcome<void, AZStd::string> ToCPlusPlus(const Grammar::AbstractCodeModel& model)
    {
        AZStd::string dotH, dotCPP;

        auto outcome = GraphToCPlusPlus::Translate(model, dotH, dotCPP);
        if (outcome.IsSuccess())
        {
#if defined(SCRIPT_CANVAS_PRINT_FILES_CONSOLE)
            AZ_TracePrintf("ScriptCanvas", "\n\n *** .h file ***\n\n");
            AZ_TracePrintf("ScriptCanvas", dotH.data());
            AZ_TracePrintf("ScriptCanvas", "\n\n *** .cpp file *\n\n");
            AZ_TracePrintf("ScriptCanvas", dotCPP.data());
            AZ_TracePrintf("ScriptCanvas", "\n\n");
#endif                
            outcome = SaveDotH(model.GetSource(), dotH);
            if (outcome.IsSuccess())
            {
                outcome = SaveDotCPP(model.GetSource(), dotCPP);
            }
        }

        return outcome;
    }

    AZ::Outcome<void, AZStd::string> ToLua(const Grammar::AbstractCodeModel& model)
    {
        AZStd::string lua;

        auto outcome = GraphToLua::Translate(model, lua);
        if (outcome.IsSuccess())
        {
#if defined(SCRIPT_CANVAS_PRINT_FILES_CONSOLE)
            AZ_TracePrintf("ScriptCanvas", "\n\n *** .lua file *\n\n");
            AZ_TracePrintf("ScriptCanvas", lua.data());
            AZ_TracePrintf("ScriptCanvas", "\n\n");
#endif
            outcome = SaveDotLua(model.GetSource(), lua);
        }

        return outcome;
    }
} // namespace TranslationCPP


namespace ScriptCanvas
{
    namespace Translation
    {
        AZ::Outcome<void, AZStd::string> ToCPlusPlusAndLua(const Graph& graph, const AZStd::string& name, const AZStd::string& path)
        {
            AZ::Outcome<Grammar::Source, AZStd::string> sourceOutcome = Grammar::Source::Construct(graph, name, path);
            if (!sourceOutcome.IsSuccess())
            {
                return AZ::Failure(sourceOutcome.GetError());
            }

            const Grammar::AbstractCodeModel model(sourceOutcome.GetValue());
            
            auto outcome = model.GetOutcome();
            if (outcome.IsSuccess())
            {
                outcome = TranslationCPP::ToCPlusPlus(model);
                if (outcome.IsSuccess())
                {
                    outcome = TranslationCPP::ToLua(model);
                }
            }

            return outcome;
        }

        AZ::Outcome<void, AZStd::string> ToCPlusPlus(const Graph& graph, const AZStd::string& name, const AZStd::string& path)
        {
            AZ::Outcome<Grammar::Source, AZStd::string> sourceOutcome = Grammar::Source::Construct(graph, name, path);
            if (!sourceOutcome.IsSuccess())
            {
                return AZ::Failure(sourceOutcome.GetError());
            }

            const Grammar::AbstractCodeModel model(sourceOutcome.GetValue());

            auto outcome = model.GetOutcome();
            if (outcome.IsSuccess())
            {
                outcome = TranslationCPP::ToCPlusPlus(model);
            }

            return outcome;
        }

        AZ::Outcome<void, AZStd::string> ToLua(const Graph& graph, const AZStd::string& name, const AZStd::string& path)
        {
            AZ::Outcome<Grammar::Source, AZStd::string> sourceOutcome = Grammar::Source::Construct(graph, name, path);
            if (!sourceOutcome.IsSuccess())
            {
                return AZ::Failure(sourceOutcome.GetError());
            }

            const Grammar::AbstractCodeModel model(sourceOutcome.GetValue());

            auto outcome = model.GetOutcome();
            if (outcome.IsSuccess())
            {
                outcome = TranslationCPP::ToLua(model);
            }

            return outcome;
        }

    } // namespace Translation
} // namespace ScriptCanvas