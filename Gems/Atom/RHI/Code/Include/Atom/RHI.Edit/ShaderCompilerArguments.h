/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/std/string/string.h>
#include <AtomCore/std/containers/array_view.h>

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

            //! Returns true if either @m_azslcAdditionalFreeArguments or @m_dxcAdditionalFreeArguments contain
            //! macro definitions, e.g. "-D MACRO" or "-D MACRO=VALUE" or "-DMACRO", "-DMACRO=VALUE".
            //! It is used for validation to forbid macro definitions, because the idea is that this struct
            //! is used inside GlobalBuildOptions which has a dedicated variable for macro definitions.
            bool HasMacroDefinitionsInCommandLineArguments();

            //! Mix two instances of arguments, by or-ing bools, or by "if different, right hand side wins"
            void Merge(const ShaderCompilerArguments& right);

            //! [GFX TODO] [ATOM-15472] Remove this function.
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

            //! Remark: To the user, the following parameters are exposed without the
            //! "Dxc" prefix because these are common options for the "main" compiler
            //! for the given RHI. At the moment the only "main" compiler is Dxc, but in
            //! the future AZSLc may transpile from AZSL to some other proprietary language
            //! and in that case the "main" compiler won't be DXC
            bool m_disableWarnings = false;
            bool m_warningAsError = false;
            bool m_disableOptimizations = false;
            bool m_generateDebugInfo = false;
            uint8_t m_optimizationLevel = LevelUnset;
            //! "DxcAdditionalFreeArguments" keeps the "Dxc" prefix because these arguments
            //! are specific to DXC, and it will be relevant only if DXC is the "main" compiler
            //! for a given RHI, otherwise this parameter won't matter.
            AZStd::string m_dxcAdditionalFreeArguments;

            //! both
            MatrixOrder m_defaultMatrixOrder = MatrixOrder::Default;
        };
    }

    AZ_TYPE_INFO_SPECIALIZE(RHI::MatrixOrder, "{69110FCD-8C61-47D0-B08D-999EE39CBDC2}");
}
