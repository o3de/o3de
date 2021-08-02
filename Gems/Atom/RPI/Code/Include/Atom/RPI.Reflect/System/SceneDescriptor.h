/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct SceneDescriptor final
        {
            AZ_TYPE_INFO(SceneDescriptor, "{B561F34F-A60A-4A02-82FD-E8A4A032BFDB}");
            static void Reflect(AZ::ReflectContext* context);

            //! List of feature processors which the scene will initially enable.
            AZStd::vector<AZStd::string> m_featureProcessorNames;
        };
    } // namespace RPI
} // namespace AZ
