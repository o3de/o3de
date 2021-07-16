/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Shader/ShaderSourceData.h>
#include <Atom/RHI.Edit/Utils.h>
#include <Atom/RHI.Edit/ShaderCompilerArguments.h>
#include <AzCore/std/string/regex.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderSourceData::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderSourceData>()
                    ->Version(4)
                    ->Field("Source", &ShaderSourceData::m_source)
                    ->Field("DrawList", &ShaderSourceData::m_drawListName)
                    ->Field("DepthStencilState", &ShaderSourceData::m_depthStencilState)
                    ->Field("RasterState", &ShaderSourceData::m_rasterState)
                    ->Field("BlendState", &ShaderSourceData::m_blendState)
                    ->Field("ProgramSettings", &ShaderSourceData::m_programSettings)
                    ->Field("CompilerHints", &ShaderSourceData::m_compiler)
                    ->Field("DisabledRHIBackends", &ShaderSourceData::m_disabledRhiBackends)
                    ->Field("Supervariants", &ShaderSourceData::m_supervariants)
                    ;

                serializeContext->Class<ShaderSourceData::ProgramSettings>()
                    ->Version(1)
                    ->Field("EntryPoints", &ProgramSettings::m_entryPoints)
                    ;

                serializeContext->Class<ShaderSourceData::EntryPoint>()
                    ->Version(1)
                    ->Field("Name", &EntryPoint::m_name)
                    ->Field("Type", &EntryPoint::m_type)
                    ;

                serializeContext->Class<ShaderSourceData::SupervariantInfo>()
                    ->Version(1)
                    ->Field("Name", &SupervariantInfo::m_name)
                    ->Field("PlusArguments", &SupervariantInfo::m_plusArguments)
                    ->Field("MinusArguments", &SupervariantInfo::m_minusArguments);

            }
        }

        bool ShaderSourceData::IsRhiBackendDisabled(const AZ::Name& rhiName) const
        {
            return AZStd::any_of(AZ_BEGIN_END(m_disabledRhiBackends), [&](const AZStd::string& currentRhiName)
                {
                    return currentRhiName == rhiName.GetStringView();
                });
        }


        //! Helper function.
        //! Parses a string of command line arguments looking for c-preprocessor macro definitions and appends the name of macro definition arguments.
        //! Example:
        //! Input string: "--switch1 -DMACRO1 -v -DMACRO2=23"
        //! append the following items: ["MACRO1", "MACRO2"]
        static void GetListOfMacroDefinitionNames( 
            const AZStd::string& stringWithArguments, AZStd::vector<AZStd::string>& macroDefinitionNames)
        {
            const AZStd::regex macroRegex(R"(-D\s*(\w+))", AZStd::regex::ECMAScript);

            AZStd::string hayStack(stringWithArguments);
            AZStd::smatch match;
            while (AZStd::regex_search(hayStack.c_str(), match, macroRegex))
            {
                // First pattern is always the entire string
                for (unsigned i = 1; i < match.size(); ++i)
                {
                    if (match[i].matched)
                    {
                        AZStd::string macroToAdd(match[i].str().c_str());
                        const bool isPresent = AZStd::any_of(AZ_BEGIN_END(macroDefinitionNames),
                            [&](AZStd::string_view macroName) -> bool
                            {
                                return macroToAdd == macroName;
                            }
                        );
                        if (isPresent)
                        {
                            continue;
                        }
                        macroDefinitionNames.push_back(macroToAdd);
                    }
                }
                hayStack = match.suffix();
            }
        }

        AZStd::vector<AZStd::string> ShaderSourceData::SupervariantInfo::GetCombinedListOfMacroDefinitionNamesToRemove() const
        {
            AZStd::vector<AZStd::string> macroDefinitionNames;
            GetListOfMacroDefinitionNames(m_minusArguments, macroDefinitionNames);
            GetListOfMacroDefinitionNames(m_plusArguments, macroDefinitionNames);
            return macroDefinitionNames;
        }


        //! Helper function.
        //! Parses a string of command line arguments looking for c-preprocessor macro definitions and appends macro definition
        //! arguments. Example: Input string: "--switch1 -DMACRO1 -v -DMACRO2=23" append the following items: ["MACRO1", "MACRO2=23"]
        static void GetListOfMacroDefinitions(
            const AZStd::string& stringWithArguments, AZStd::vector<AZStd::string>& macroDefinitions)
        {
            const AZStd::regex macroRegex(R"(-D\s*(\w+)(=\w+)?)", AZStd::regex::ECMAScript);

            AZStd::string hayStack(stringWithArguments);
            AZStd::smatch match;
            while (AZStd::regex_search(hayStack.c_str(), match, macroRegex))
            {
                if (match.size() > 1)
                {
                    AZStd::string macro(match[1].str().c_str());
                    if (match.size() > 2)
                    {
                        macro += match[2].str().c_str();
                    }
                    macroDefinitions.push_back(macro);
                }
                hayStack = match.suffix();
            }
        }

        AZStd::vector<AZStd::string> ShaderSourceData::SupervariantInfo::GetMacroDefinitionsToAdd() const
        {
            AZStd::vector<AZStd::string> parsedMacroDefinitions;
            GetListOfMacroDefinitions(m_plusArguments, parsedMacroDefinitions);
            return parsedMacroDefinitions;
        }

        AZStd::string ShaderSourceData::SupervariantInfo::GetCustomizedArgumentsForAzslc(
            const AZStd::string& initialAzslcCompilerArguments) const
        {
            const AZStd::regex macroRegex(R"(-D\s*(\w+(=\S+)?))", AZStd::regex::ECMAScript);

            // We are only concerned with AZSLc arguments. Let's remove the C-Preprocessor macro definitions
            // from @minusArguments.
            const AZStd::string minusArguments = AZStd::regex_replace(m_minusArguments, macroRegex, "");
            const AZStd::string plusArguments = AZStd::regex_replace(m_plusArguments, macroRegex, "");
            AZStd::string azslcArgumentsToRemove = minusArguments + " " + plusArguments;
            AZStd::vector<AZStd::string> azslcArgumentNamesToRemove = RHI::CommandLineArgumentUtils::GetListOfArgumentNames(azslcArgumentsToRemove);

            // At this moment @azslcArgumentsToRemove contains arguments for AZSLc that can be of the form:
            // -<arg>
            // --<arg>[=<value>]
            // We need to remove those from @initialAzslcCompilerArguments.
            AZStd::string customizedArguments = RHI::CommandLineArgumentUtils::RemoveArgumentsFromCommandLineString(
                azslcArgumentNamesToRemove, initialAzslcCompilerArguments);
            customizedArguments += " " + plusArguments;

            return RHI::CommandLineArgumentUtils::RemoveExtraSpaces(customizedArguments);
        }


    } // namespace RPI
} // namespace AZ
