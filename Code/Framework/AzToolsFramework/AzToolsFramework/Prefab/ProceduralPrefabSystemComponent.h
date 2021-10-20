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
        {
        public:
            AZ_COMPONENT(ProceduralPrefabSystemComponent, "{81211818-088A-49E6-894B-7A11764106B1}");

            static void Reflect(AZ::ReflectContext* context);

        protected:
            void Activate() override;
            void Deactivate() override;
            
            AZStd::string Load(AZ::Data::AssetId assetId) override;
            
            AZStd::unordered_map<TemplateId, AZ::Data::Asset<AZ::Data::AssetData>> m_templateToAssetLookup;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

