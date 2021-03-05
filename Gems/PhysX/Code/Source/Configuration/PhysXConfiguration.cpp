/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <PhysX_precompiled.h>

#include <PhysX/Configuration/PhysXConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace PhysX
{
    namespace PhysXInternal
    {
        AzPhysics::CollisionConfiguration CreateDefaultCollisionConfiguration()
        {
            AzPhysics::CollisionConfiguration configuration;
            configuration.m_collisionLayers.SetName(AzPhysics::CollisionLayer::Default, "Default");

            configuration.m_collisionGroups.CreateGroup("All", AzPhysics::CollisionGroup::All, AzPhysics::CollisionGroups::Id(), true);
            configuration.m_collisionGroups.CreateGroup("None", AzPhysics::CollisionGroup::None, AzPhysics::CollisionGroups::Id::Create(), true);

#ifdef TOUCHBENDING_LAYER_BIT
            configuration.m_collisionLayers.SetName(AzPhysics::CollisionLayer::TouchBend, "TouchBend");
            configuration.m_collisionGroups.CreateGroup("All_NoTouchBend", AzPhysics::CollisionGroup::All_NoTouchBend, AzPhysics::CollisionGroups::Id::Create(), true);
#endif

            return configuration;
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(WindConfiguration, AZ::SystemAllocator, 0);
    AZ_CLASS_ALLOCATOR_IMPL(PhysXSystemConfiguration, AZ::SystemAllocator, 0);

    /*static*/ void WindConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<WindConfiguration>()
                ->Version(1)
                ->Field("GlobalWindTag", &WindConfiguration::m_globalWindTag)
                ->Field("LocalWindTag", &WindConfiguration::m_localWindTag)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<WindConfiguration>("Wind Configuration", "Wind settings for PhysX")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindConfiguration::m_globalWindTag,
                        "Global wind tag",
                        "Tag value that will be used to mark entities that provide global wind value.\n"
                        "Global wind has no bounds and affects objects across entire level.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindConfiguration::m_localWindTag,
                        "Local wind tag",
                        "Tag value that will be used to mark entities that provide local wind value.\n"
                        "Local wind is only applied within bounds defined by PhysX collider.")
                    ;
            }
        }
    }

    bool WindConfiguration::operator==(const WindConfiguration& other) const
    {
        return m_globalWindTag == other.m_globalWindTag &&
            m_localWindTag == other.m_localWindTag
            ;
    }

    bool WindConfiguration::operator!=(const WindConfiguration& other) const
    {
        return !(*this == other);
    }

    /*static*/ void PhysXSystemConfiguration::Reflect(AZ::ReflectContext* context)
    {
        AzPhysics::SystemConfiguration::Reflect(context);
        WindConfiguration::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PhysXSystemConfiguration, AzPhysics::SystemConfiguration>()
                ->Version(1)
                ->Field("WindConfiguration", &PhysXSystemConfiguration::m_windConfiguration)
                ->Field("MaterialLibrary", &PhysXSystemConfiguration::m_defaultMaterialLibrary)
                ;
        }
    }

    /*static*/ PhysXSystemConfiguration PhysXSystemConfiguration::CreateDefault()
    {
        PhysXSystemConfiguration systemConfig;
        systemConfig.m_collisionConfig = PhysXInternal::CreateDefaultCollisionConfiguration();
        return systemConfig;
    }

    bool PhysXSystemConfiguration::operator==(const PhysXSystemConfiguration& other) const
    {
        return AzPhysics::SystemConfiguration::operator==(other) &&
            m_defaultMaterialLibrary == other.m_defaultMaterialLibrary &&
            m_windConfiguration == other.m_windConfiguration
            ;
    }

    bool PhysXSystemConfiguration::operator!=(const PhysXSystemConfiguration& other) const
    {
        return !(*this == other);
    }
}
