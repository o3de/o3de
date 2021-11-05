/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "HeightfieldProviderBus.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace Physics
{
    void HeightfieldProviderRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<Physics::HeightfieldProviderRequestsBus>("HeightfieldProviderRequestsBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Event("GetHeightfieldGridSpacing", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridSpacing)
                ->Event("GetHeightfieldAabb", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldAabb)
                ->Event("GetHeightfieldTransform", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldTransform)
                ->Event("GetMaterialList", &Physics::HeightfieldProviderRequestsBus::Events::GetMaterialList)
                ->Event("GetHeights", &Physics::HeightfieldProviderRequestsBus::Events::GetHeights)
                ->Event("GetHeightsAndMaterials", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightsAndMaterials)
                ->Event("GetHeightfieldMinHeight", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldMinHeight)
                ->Event("GetHeightfieldMaxHeight", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldMaxHeight)
                ->Event("GetHeightfieldGridColumns", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridColumns)
                ->Event("GetHeightfieldGridRows", &Physics::HeightfieldProviderRequestsBus::Events::GetHeightfieldGridRows)
                ;
        }
    }

    HeightMaterialPoint::HeightMaterialPoint(float height, QuadMeshType type, uint8_t index = 0, uint16_t padding = 0)
        : m_height(height)
        , m_quadMeshType(type)
        , m_materialIndex(index)
        , m_padding(0)
    {
    }

} // namespace Physics
