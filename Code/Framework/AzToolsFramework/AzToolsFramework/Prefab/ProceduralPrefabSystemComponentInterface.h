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
        class ProceduralPrefabSystemComponentInterface
        {
        public:
            AZ_RTTI(ProceduralPrefabSystemComponentInterface, "{C403B89C-63DD-418C-B821-FBCBE1EF42AE}");

            virtual ~ProceduralPrefabSystemComponentInterface() = default;

            // Registers a procedural prefab file + templateId so the system can track changes and handle updates
            virtual void RegisterProceduralPrefab(const AZStd::string& prefabFilePath, TemplateId templateId) = 0;
        };
        
    } // namespace Prefab
} // namespace AzToolsFramework

