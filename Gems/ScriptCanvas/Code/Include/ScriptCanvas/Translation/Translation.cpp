/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Translation.h"

#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/lua/lua.h>
#include <AzCore/std/string/conversions.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Grammar/AbstractCodeModel.h>
#include <ScriptCanvas/Translation/GraphToCPlusPlus.h>
#include <ScriptCanvas/Translation/GraphToLua.h>
#include <ScriptCanvas/Core/Graph.h>

namespace TranslationCPP
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Translation;

    /*
    AZ::Outcome<AZStd::pair<AZStd::string, AZStd::string>, AZStd::pair<AZStd::string, AZStd::string>> ToCPlusPlus(const Grammar::AbstractCodeModel& model, bool rawSave = false)
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
            if (rawSave)
            {
                auto saveOutcome = SaveDotH(model.GetSource(), dotH);
                if (saveOutcome.IsSuccess())
                {
                    saveOutcome = SaveDotCPP(model.GetSource(), dotCPP);
                }
                if (!saveOutcome.IsSuccess())
                {
                    AZ_TracePrintf("Save failed %s", saveOutcome.GetError().data());
                }
            }
            return AZ::Success(AZStd::make_pair(AZStd::move(dotH), AZStd::move(dotCPP)));
        }
        else
        {
            return AZ::Failure(outcome.TakeError());
        }
    }
    */

    AZ::Outcome<TargetResult, ErrorList> ToLua(const Grammar::AbstractCodeModel& model, bool rawSave = false)
    {
        auto outcome = GraphToLua::Translate(model);
        if (outcome.IsSuccess())
        {
#if defined(SCRIPT_CANVAS_PRINT_FILES_CONSOLE)
            AZ_TracePrintf("ScriptCanvas", "\n\n *** .lua file *\n\n");
            AZ_TracePrintf("ScriptCanvas", lua.data());
            AZ_TracePrintf("ScriptCanvas", "\n\n");
#endif
            if (rawSave)
            {
                auto saveOutcome = SaveDotLua(model.GetSource(), outcome.GetValue().m_text);
                if (!saveOutcome.IsSuccess())
                {
                    AZ_TracePrintf("Save failed %s", saveOutcome.GetError().data());
                }
            }

            return AZ::Success(outcome.TakeValue());
        }
        else
        {
            return AZ::Failure(outcome.TakeError());
        }
    }
}

namespace ScriptCanvas
{
    namespace Translation
    {
        AZ::Outcome<Grammar::AbstractCodeModelConstPtr, AZStd::string> ParseGraph(const Grammar::Request& request)
        {
            AZ::Outcome<Grammar::Source, AZStd::string> sourceOutcome = Grammar::Source::Construct(request);

            if (!sourceOutcome.IsSuccess())
            {
                return AZ::Failure(sourceOutcome.TakeError());
            }

            Grammar::AbstractCodeModelConstPtr model = Grammar::AbstractCodeModel::Parse(sourceOutcome.TakeValue());
            return AZ::Success(model);
        }

        Result ParseAndTranslateGraph(const Grammar::Request& request)
        {
            auto parseOutcome = ParseGraph(request);
            if (!parseOutcome.IsSuccess())
            {
                return Result(parseOutcome.TakeError());
            }

            Grammar::AbstractCodeModelConstPtr model = parseOutcome.TakeValue();
            Translations translations;
            Errors errors;

            if (model->IsErrorFree())
            {
                if (request.translationTargetFlags & TargetFlags::Lua)
                {
                    auto outcomeLua = TranslationCPP::ToLua(*model.get(), request.rawSaveDebugOutput);
                    if (outcomeLua.IsSuccess())
                    {
                        translations.emplace(TargetFlags::Lua, outcomeLua.TakeValue());
                    }
                    else
                    {
                        errors.emplace(TargetFlags::Lua, outcomeLua.TakeError());
                    }
                }

                // Translation to C++ (executed via BehaviorContext calls) has been demonstrated in the past and is partially in progress.
                // These calls allow for users to execute multiple translations from the same abstract code model.
                // More work is required to complete all the latest features of the ACM, and do integrate output files into the build.
                // 
                //                 if (targetFlags & (TargetFlags::Cpp | TargetFlags::Hpp))
                //                 {
                //                     auto outcomeCPP = TranslationCPP::ToCPlusPlus(*model.get(), rawSave);
                //                     if (outcomeCPP.IsSuccess())
                //                     {
                //                         auto hppAndCpp = outcomeCPP.TakeValue();
                //                         
                //                         TargetResult cppResult;
                //                         cppResult.m_text = AZStd::move(hppAndCpp.first);
                //                         translations.emplace(TargetFlags::Hpp, AZStd::move(cppResult));
                //                         TargetResult hppResult;
                //                         hppResult.m_text = AZStd::move(hppAndCpp.second);
                //                         translations.emplace(TargetFlags::Cpp, AZStd::move(hppResult));
                //                     }
                //                     else
                //                     {
                //                         auto hppAndCpp = outcomeCPP.TakeError();
                //                         errors.emplace(TargetFlags::Hpp, AZStd::move(hppAndCpp.first));
                //                         errors.emplace(TargetFlags::Cpp, AZStd::move(hppAndCpp.second));
                //                     }
                //                 }

            }

            return Result(model, AZStd::move(translations), AZStd::move(errors));
        }

        Result ToCPlusPlusAndLua(const Grammar::Request& request)
        {
            Grammar::Request toBoth = request;
            toBoth.translationTargetFlags = TargetFlags::Lua | TargetFlags::Cpp;
            return ParseAndTranslateGraph(toBoth);
        }

        Result ToCPlusPlus(const Grammar::Request& request)
        {
            Grammar::Request toCpp = request;
            toCpp.translationTargetFlags = TargetFlags::Cpp;
            return ParseAndTranslateGraph(toCpp);
        }

        Result ToLua(const Grammar::Request& request)
        {
            Grammar::Request toLua = request;
            toLua.translationTargetFlags = TargetFlags::Lua;
            return ParseAndTranslateGraph(toLua);
        }
    }
}
