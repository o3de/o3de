/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/ShaderBuildArguments.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzCore/std/string/regex.h>

#include <Atom/RHI.Edit/Utils.h>

namespace AZ
{
    namespace RHI
    {
        void ShaderBuildArguments::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderBuildArguments>()
                    ->Version(1)
                    ->Field("debug", &ShaderBuildArguments::m_generateDebugInfo)
                    ->Field("preprocessor", &ShaderBuildArguments::m_preprocessorArguments)
                    ->Field("azslc", &ShaderBuildArguments::m_azslcArguments)
                    ->Field("dxc", &ShaderBuildArguments::m_dxcArguments)
                    ->Field("spirv-cross", &ShaderBuildArguments::m_spirvCrossArguments)
                    ->Field("metalair", &ShaderBuildArguments::m_metalAirArguments)
                    ->Field("metallib", &ShaderBuildArguments::m_metalLibArguments)
                    ;
            }
        }

        ShaderBuildArguments::ShaderBuildArguments(
                bool generateDebugInfo,
                const AZStd::vector<AZStd::string>& preprocessorArguments,
                const AZStd::vector<AZStd::string>& azslcArguments,
                const AZStd::vector<AZStd::string>& dxcArguments,
                const AZStd::vector<AZStd::string>& spirvCrossArguments,
                const AZStd::vector<AZStd::string>& metalAirArguments,
                const AZStd::vector<AZStd::string>& metalLibArguments)
            : m_generateDebugInfo(generateDebugInfo)
            , m_preprocessorArguments(preprocessorArguments)
            , m_azslcArguments(azslcArguments)
            , m_dxcArguments(dxcArguments)
            , m_spirvCrossArguments(spirvCrossArguments)
            , m_metalAirArguments(metalAirArguments)
            , m_metalLibArguments(metalLibArguments)
        {
        }

        bool ShaderBuildArguments::HasArgument(const AZStd::vector<AZStd::string>& argList, const AZStd::string& arg)
        {
            return AZStd::find(argList.begin(), argList.end(), arg) != argList.end();
        }

        void ShaderBuildArguments::AppendArguments(AZStd::vector<AZStd::string>& out, const AZStd::vector<AZStd::string>& in)
        {
            for (const auto& argument : in)
            {
                if (!HasArgument(out, argument))
                {
                    out.push_back(argument);
                }
            }
        }

        void ShaderBuildArguments::RemoveArguments(AZStd::vector<AZStd::string>& out, const AZStd::vector<AZStd::string>& in)
        {
            out.erase(remove_if(out.begin(), out.end(),
                [&](const AZStd::string& arg)
                {
                    return HasArgument(in, arg);
                }), std::end(out));
        }

        AZStd::string ShaderBuildArguments::ListAsString(const AZStd::vector<AZStd::string>& argList)
        {
            AZStd::string listAsString;
            AZ::StringFunc::Join(listAsString, argList, " ");
            return listAsString;
        }

        int ShaderBuildArguments::AppendDefinitions(const AZStd::vector<AZStd::string>& definitions)
        {
            int oldPreprocessorArgCount = aznumeric_caster(m_preprocessorArguments.size());

            // Roll the Definitions into m_preprocessorArguments.
            AZStd::vector<AZStd::string> definitionsAsArguments;
            for (auto definition : definitions)
            {
                AZ::StringFunc::TrimWhiteSpace(definition, true, true);
                if (definition.empty())
                {
                    AZ_Warning("ShaderBuildArguments", false, "%s Found an empty definition", __FUNCTION__);
                    continue;
                }

                // Spaces in between a definition string are not allowed.
                if ( (AZ::StringFunc::Find(definition, ' ')  != AZStd::string::npos) ||
                     (AZ::StringFunc::Find(definition, '\t') != AZStd::string::npos) )
                {
                    AZ_Assert(false, "The definition <%s> contains spaces, which is not allowed.", definition.c_str());
                    return -1;
                }

                if (definition[0] == '-')
                {
                    AZ_Assert(false, "Definitions can not start with \"-\" or \"-D\"");
                    return -1;
                }
                definitionsAsArguments.emplace_back(AZStd::string::format("-D%s", definition.c_str()));
            }
            AZ::RHI::ShaderBuildArguments::AppendArguments(m_preprocessorArguments, definitionsAsArguments);

            int newPreprocessorArgCount = aznumeric_caster(m_preprocessorArguments.size());
            return newPreprocessorArgCount - oldPreprocessorArgCount;
        }

        ShaderBuildArguments ShaderBuildArguments::operator+(const ShaderBuildArguments& rhs) const
        {
            auto buildArguments = *this;
            buildArguments += rhs;
            return buildArguments;
        }

        ShaderBuildArguments& ShaderBuildArguments::operator+=(const ShaderBuildArguments& rhs)
        {
            m_generateDebugInfo |= rhs.m_generateDebugInfo;
            AppendArguments(m_preprocessorArguments, rhs.m_preprocessorArguments);
            AppendArguments(m_azslcArguments, rhs.m_azslcArguments);
            AppendArguments(m_preprocessorArguments, rhs.m_preprocessorArguments);
            AppendArguments(m_dxcArguments, rhs.m_dxcArguments);
            AppendArguments(m_spirvCrossArguments, rhs.m_spirvCrossArguments);
            AppendArguments(m_metalAirArguments, rhs.m_metalAirArguments);
            AppendArguments(m_metalLibArguments, rhs.m_metalLibArguments);
            return *this;
        }

        ShaderBuildArguments ShaderBuildArguments::operator-(const ShaderBuildArguments& rhs) const
        {
            auto buildArguments = *this;
            buildArguments -= rhs;
            return buildArguments;
        }

        ShaderBuildArguments& ShaderBuildArguments::operator-=(const ShaderBuildArguments& rhs)
        {
            m_generateDebugInfo &= !rhs.m_generateDebugInfo;
            RemoveArguments(m_preprocessorArguments, rhs.m_preprocessorArguments);
            RemoveArguments(m_azslcArguments, rhs.m_azslcArguments);
            RemoveArguments(m_preprocessorArguments, rhs.m_preprocessorArguments);
            RemoveArguments(m_dxcArguments, rhs.m_dxcArguments);
            RemoveArguments(m_spirvCrossArguments, rhs.m_spirvCrossArguments);
            RemoveArguments(m_metalAirArguments, rhs.m_metalAirArguments);
            RemoveArguments(m_metalLibArguments, rhs.m_metalLibArguments);
            return *this;
        }
    }
}
