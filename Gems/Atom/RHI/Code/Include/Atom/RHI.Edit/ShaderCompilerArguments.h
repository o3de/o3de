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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Preprocessor/Enum.h>

namespace AZ
{
    namespace RHI
    {
        AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(MatrixOrder, uint8_t,
            Default,  // nothing
            Column,   // -Zpc
            Row       // -Zpr
        );

        struct ShaderCompilerArguments
        {
            AZ_TYPE_INFO(ShaderCompilerArguments, "{7D0D58C8-EB95-4595-BC96-7390BEE0C048}");

            static void Reflect(ReflectContext* context);

            //! Mix two instances of arguments, by or-ing bools, or by "if different, right hand side wins"
            void Merge(const ShaderCompilerArguments& right);

            //! Determine whether there is a rebuild-worthy difference in arguments for AZSLc
            bool HasDifferentAzslcArguments(const ShaderCompilerArguments& right) const;

            //! Generate the proper command line for AZSLc
            AZStd::string MakeAdditionalAzslcCommandLineString() const;

            //! Warnings are separated from the other arguments because not all AZSLc modes can support passing these.
            AZStd::string MakeAdditionalAzslcWarningCommandLineString() const;

            //! Generate the proper command line for DXC
            AZStd::string MakeAdditionalDxcCommandLineString() const;

            //! AZSLc
            static constexpr uint8_t LevelUnset = std::numeric_limits<uint8_t>::max();
            uint8_t m_azslcWarningLevel = LevelUnset;
            bool m_azslcWarningAsError = false;
            AZStd::string m_azslcAdditionalFreeArguments;
            // note: if you add new sort of arguments here, don't forget to update HasDifferentAzslcArguments()

            //! DXC
            bool m_dxcDisableWarnings = false;
            bool m_dxcWarningAsError = false;
            bool m_dxcDisableOptimizations = false;
            bool m_dxcGenerateDebugInfo = false;
            uint8_t m_dxcOptimizationLevel = LevelUnset;
            AZStd::string m_dxcAdditionalFreeArguments;

            //! both
            MatrixOrder m_defaultMatrixOrder = MatrixOrder::Default;
        };
    }

    AZ_TYPE_INFO_SPECIALIZE(RHI::MatrixOrder, "{69110FCD-8C61-47D0-B08D-999EE39CBDC2}");
}
