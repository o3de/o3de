/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>

namespace ScriptCanvas
{
    //! CRCNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace CRCNodes
    {
        static constexpr const char* k_categoryName = "Math/Crc32";

        AZ_INLINE Data::CRCType FromString(Data::StringType value)
        {
            return AZ::Crc32(value.data());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromString, k_categoryName, "{FB95B93C-4CF6-4436-BB54-30FB48AEE7DF}", "ScriptCanvas_CRCFunctions_FromString");

        using Registrar = RegistrarGeneric<
            FromStringNode
        >;
    }
} 

