/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/SystemConfiguration.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    namespace
    {
        const float TimestepMin = 0.001f; //1000fps
        const float TimestepMax = 0.1f; //10fps
    }

    AZ_CLASS_ALLOCATOR_IMPL(SystemConfiguration, AZ::SystemAllocator, 0);

    /*static*/ void SystemConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AzPhysics::SystemConfiguration>()
                ->Version(2)
                ->Field("AutoManageSimulationUpdate", &SystemConfiguration::m_autoManageSimulationUpdate)
                ->Field("MaxTimestep", &SystemConfiguration::m_maxTimestep)
                ->Field("FixedTimeStep", &SystemConfiguration::m_fixedTimestep)
                ->Field("RaycastBufferSize", &SystemConfiguration::m_raycastBufferSize)
                ->Field("ShapecastBufferSize", &SystemConfiguration::m_shapecastBufferSize)
                ->Field("OverlapBufferSize", &SystemConfiguration::m_overlapBufferSize)
                ->Field("CollisionConfig", &SystemConfiguration::m_collisionConfig)
                ->Field("DefaultMaterial", &SystemConfiguration::m_defaultMaterialConfiguration)
                ->Field("MaterialLibrary", &SystemConfiguration::m_materialLibraryAsset)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<AzPhysics::SystemConfiguration>("", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemConfiguration::m_maxTimestep, "Max Time Step (sec)", "Max time step in seconds")
                        ->Attribute(AZ::Edit::Attributes::Min, TimestepMin)
                        ->Attribute(AZ::Edit::Attributes::Max, TimestepMax)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &SystemConfiguration::OnMaxTimeStepChanged)//need to clamp m_fixedTimeStep if this value changes
                        ->Attribute(AZ::Edit::Attributes::Decimals, 8)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 8)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemConfiguration::m_fixedTimestep, "Fixed Time Step (sec)", "Fixed time step in seconds. Limited by 'Max Time Step'")
                        ->Attribute(AZ::Edit::Attributes::Min, TimestepMin)
                        ->Attribute(AZ::Edit::Attributes::Max, &SystemConfiguration::GetFixedTimeStepMax)
                        ->Attribute(AZ::Edit::Attributes::Decimals, 8)
                        ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 8)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemConfiguration::m_raycastBufferSize,
                        "Raycast Buffer Size", "Maximum number of hits from a raycast")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemConfiguration::m_shapecastBufferSize,
                        "Shapecast Buffer Size", "Maximum number of hits from a shapecast")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemConfiguration::m_overlapBufferSize,
                        "Overlap Query Buffer Size", "Maximum number of hits from a overlap query")
                        ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ;
            }
        }
    }
    
    bool SystemConfiguration::operator==(const SystemConfiguration& other) const
    {
        return m_autoManageSimulationUpdate == other.m_autoManageSimulationUpdate &&
            m_raycastBufferSize == other.m_raycastBufferSize &&
            m_shapecastBufferSize == other.m_shapecastBufferSize &&
            m_overlapBufferSize == other.m_overlapBufferSize &&
            AZ::IsClose(m_maxTimestep, other.m_maxTimestep) &&
            AZ::IsClose(m_fixedTimestep, other.m_fixedTimestep) &&
            m_collisionConfig == other.m_collisionConfig &&
            m_defaultMaterialConfiguration == other.m_defaultMaterialConfiguration &&
            m_materialLibraryAsset == other.m_materialLibraryAsset
            ;
    }

    bool SystemConfiguration::operator!=(const SystemConfiguration& other) const
    {
        return !(*this == other);
    }

    AZ::u32 SystemConfiguration::OnMaxTimeStepChanged()
    {
        m_fixedTimestep = AZStd::GetMin(m_fixedTimestep, GetFixedTimeStepMax()); //since m_maxTimeStep has changed, m_fixedTimeStep might be larger then the max.
        return AZ::Edit::PropertyRefreshLevels::AttributesAndValues;
    }

    float SystemConfiguration::GetFixedTimeStepMax() const
    {
        return m_maxTimestep;
    }
}
