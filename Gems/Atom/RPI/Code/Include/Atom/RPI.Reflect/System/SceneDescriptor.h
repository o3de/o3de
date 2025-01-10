/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;

    namespace RPI
    {
        struct ATOM_RPI_REFLECT_API SceneDescriptor final
        {
            AZ_TYPE_INFO(SceneDescriptor, "{B561F34F-A60A-4A02-82FD-E8A4A032BFDB}");
            static void Reflect(AZ::ReflectContext* context);

            //! List of feature processors which the scene will initially enable.
            AZStd::vector<AZStd::string> m_featureProcessorNames;

            //! A name used as scene id. It can be used to search a registered scene via RPISystemInterface::GetScene()
            AZ::Name m_nameId;
        };
    } // namespace RPI
} // namespace AZ
