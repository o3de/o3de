/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        /*!
         * PrefabSystemComponentInterface
         * Interface serviced by the Prefab System Component.
         */
        class ProceduralPrefabSystemComponentInterface
        {
        public:
            AZ_RTTI(ProceduralPrefabSystemComponentInterface, "{7FE01294-4E28-4894-9599-705C510A844E}");

            virtual ~ProceduralPrefabSystemComponentInterface() = default;

            virtual AZStd::string Load(AZ::Data::AssetId assetId) = 0;
        };
        
    } // namespace Prefab
} // namespace AzToolsFramework

