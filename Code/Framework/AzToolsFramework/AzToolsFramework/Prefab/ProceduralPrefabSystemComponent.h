/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/ProceduralPrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class ProceduralPrefabSystemComponent
            : public AZ::Component
            , private ProceduralPrefabSystemComponentInterface
            , AzFramework::AssetCatalogEventBus::Handler
        {
        public:
            AZ_COMPONENT(ProceduralPrefabSystemComponent, "{81211818-088A-49E6-894B-7A11764106B1}");

            static void Reflect(AZ::ReflectContext* context);

        protected:
            void Activate() override;
            void Deactivate() override;
            
            void OnCatalogAssetChanged(const AZ::Data::AssetId&) override;
        
            void Track(const AZStd::string& prefabFilePath, TemplateId templateId) override;
            
            AZStd::unordered_map<AZ::Data::AssetId, TemplateId> m_templateToAssetLookup;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

