/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/ShaderCompilerArguments.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/regex.h>

#include <Atom/RHI.Edit/Utils.h>

namespace AZ
{
    namespace RHI
    {
        template< typename AzEnumType >
        static void RegisterEnumerators(::AZ::SerializeContext* context)
        {
            auto enumMaker = context->Enum<AzEnumType>();
            for (auto&& member : AzEnumTraits< AzEnumType >::Members)
            {
                enumMaker.Value(member.m_string.data(), member.m_value);
            }
        }

        void ShaderCompilerArguments::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                RegisterEnumerators<MatrixOrder>(serializeContext);

                serializeContext->Class<ShaderCompilerArguments>()
                    ->Version(2)
                    ->Field("AzslcWarningLevel", &ShaderCompilerArguments::m_azslcWarningLevel)
                    ->Field("AzslcWarningAsError", &ShaderCompilerArguments::m_azslcWarningAsError)
                    ->Field("AzslcAdditionalFreeArguments", &ShaderCompilerArguments::m_azslcAdditionalFreeArguments)
                    ->Field("DxcDisableWarnings", &ShaderCompilerArguments::m_dxcDisableWarnings)
                    ->Field("DxcWarningAsError", &ShaderCompilerArguments::m_dxcWarningAsError)
                    ->Field("DxcDisableOptimizations", &ShaderCompilerArguments::m_dxcDisableOptimizations)
                    ->Field("DxcGenerateDebugInfo", &ShaderCompilerArguments::m_dxcGenerateDebugInfo)
                    ->Field("DxcOptimizationLevel", &ShaderCompilerArguments::m_dxcOptimizationLevel)
                    ->Field("DxcAdditionalFreeArguments", &ShaderCompilerArguments::m_dxcAdditionalFreeArguments)
                    ->Field("DefaultMatrixOrder", &ShaderCompilerArguments::m_defaultMatrixOrder)
                    ;
            }
        }

        bool ShaderCompilerArguments::HasMacroDefinitionsInCommandLineArguments()
        {
            return CommandLineArgumentUtils::HasMacroDefinitions(m_azslcAdditionalFreeArguments) ||
                CommandLineArgumentUtils::HasMacroDefinitions(m_dxcAdditionalFreeArguments);
        }

        void ShaderCompilerArguments::Merge(const ShaderCompilerArguments& right)
        {
            if (right.m_azslcWarningLevel != LevelUnset)
            {
                m_azslcWarningLevel = right.m_azslcWarningLevel;
            }
            m_azslcWarningAsError = m_azslcWarningAsError || right.m_azslcWarningAsError;
            m_azslcAdditionalFreeArguments = CommandLineArgumentUtils::MergeCommandLineArguments(m_azslcAdditionalFreeArguments, right.m_azslcAdditionalFreeArguments);
            m_dxcDisableWarnings = m_dxcDisableWarnings || right.m_dxcDisableWarnings;
            m_dxcWarningAsError = m_dxcWarningAsError || right.m_dxcWarningAsError;
            m_dxcDisableOptimizations = m_dxcDisableOptimizations || right.m_dxcDisableOptimizations;
            m_dxcGenerateDebugInfo = m_dxcGenerateDebugInfo || right.m_dxcGenerateDebugInfo;
            if (right.m_dxcOptimizationLevel != LevelUnset)
            {
                m_dxcOptimizationLevel = right.m_dxcOptimizationLevel;
            }
            m_dxcAdditionalFreeArguments = CommandLineArgumentUtils::MergeCommandLineArguments(m_dxcAdditionalFreeArguments, right.m_dxcAdditionalFreeArguments);
            if (right.m_defaultMatrixOrder != MatrixOrder::Default)
            {
                m_defaultMatrixOrder = right.m_defaultMatrixOrder;
            }
        }

        //! [GFX TODO] [ATOM-15472] Remove this function.
        bool ShaderCompilerArguments::HasDifferentAzslcArguments(const ShaderCompilerArguments& right) const
        {
            auto isSet = +[](uint8_t level) { return level != LevelUnset; };
            return (isSet(m_azslcWarningLevel) && isSet(right.m_azslcWarningLevel) && (m_azslcWarningLevel != right.m_azslcWarningLevel)) // both set and different
                || (m_azslcWarningAsError != right.m_azslcWarningAsError)
                || !right.m_azslcAdditionalFreeArguments.empty();
        }

        //! Generate the proper command line for AZSLc
        AZStd::string ShaderCompilerArguments::MakeAdditionalAzslcCommandLineString() const
        {
            AZStd::string arguments;
            if (m_defaultMatrixOrder == MatrixOrder::Column)
            {
                arguments += " --Zpc";
            }
            else if (m_defaultMatrixOrder == MatrixOrder::Row)
            {
                arguments += " --Zpr";
            }
            if (!m_azslcAdditionalFreeArguments.empty())
            {
                // strip spaces at both sides
                AZStd::string azslcFreeArguments = m_azslcAdditionalFreeArguments;
                AzFramework::StringFunc::TrimWhiteSpace(azslcFreeArguments, true, true);
                if (!azslcFreeArguments.empty())
                {
                    arguments += " " + azslcFreeArguments;
                }
            }
            
            return arguments;
        }

        //! Warnings are separated from the other arguments because not all AZSLc modes can support passing these.
        AZStd::string ShaderCompilerArguments::MakeAdditionalAzslcWarningCommandLineString() const
        {
            AZStd::string arguments;
            if (m_azslcWarningAsError)
            {
                arguments += " --Wx";
            }
            if (m_azslcWarningLevel <= 3)
            {
                arguments += " --W" + AZStd::to_string(m_azslcWarningLevel);
            }
            return arguments;
        }

        //! generate the proper command line for DXC
        AZStd::string ShaderCompilerArguments::MakeAdditionalDxcCommandLineString() const
        {
            AZStd::string arguments;
            if (m_dxcDisableWarnings)
            {
                arguments += " -no-warnings";
            }
            else if (m_dxcWarningAsError)
            {
                arguments += " -WX";
            }
            if (m_dxcDisableOptimizations)
            {
                arguments += " -Od";
            }
            else if (m_dxcOptimizationLevel <= 3)
            {
                arguments = " -O" + AZStd::to_string(m_dxcOptimizationLevel);
            }
            if (m_defaultMatrixOrder == MatrixOrder::Column)
            {
                arguments += " -Zpc";
            }
            else if (m_defaultMatrixOrder == MatrixOrder::Row)
            {
                arguments += " -Zpr";
            }
            // strip spaces at both sides
            AZStd::string dxcAdditionalFreeArguments = m_dxcAdditionalFreeArguments;
            AzFramework::StringFunc::TrimWhiteSpace(dxcAdditionalFreeArguments, true, true);
            if (!dxcAdditionalFreeArguments.empty())
            {
                arguments += " " + dxcAdditionalFreeArguments;
            }
            return arguments;
        }
    }
}
