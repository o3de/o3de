/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
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

            return configuration;
        }

        bool PhysXSystemConfigurationConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            if (dataElement.GetVersion() <= 1)
            {
                dataElement.RemoveElementByName(AZ_CRC_CE("DefaultMaterialLibrary"));
                AZ_Warning("PhysXSystemConfigurationConverter", false,
                    "Old version of PhysX Configuration data found. 'DefaultMaterialLibrary' element removed."); 
            }

            return true;
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(WindConfiguration, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(PhysXSystemConfiguration, AZ::SystemAllocator);

    /*static*/ void WindConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PhysX::WindConfiguration>()
                ->Version(1)
                ->Field("GlobalWindTag", &WindConfiguration::m_globalWindTag)
                ->Field("LocalWindTag", &WindConfiguration::m_localWindTag)
                ;

            if (AZ::EditContext* editContext = serialize->GetEditContext())
            {
                editContext->Class<PhysX::WindConfiguration>("Wind Configuration", "Wind force entity tags.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindConfiguration::m_globalWindTag,
                        "Global wind tag",
                        "Global wind provider tags.\n"
                        "Global winds apply to entire world.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &WindConfiguration::m_localWindTag,
                        "Local wind tag",
                        "Local wind provider tags.\n"
                        "Local winds are constrained to a PhysX collider's boundaries.")
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

        if (auto* serializeContext = azdynamic_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysX::PhysXSystemConfiguration, AzPhysics::SystemConfiguration>()
                ->Version(2, &PhysXInternal::PhysXSystemConfigurationConverter)
                ->Field("WindConfiguration", &PhysXSystemConfiguration::m_windConfiguration)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                // this is needed so the edit context of AzPhysics::SystemConfiguration can be used.
                editContext->Class<PhysX::PhysXSystemConfiguration>("System Configuration", "PhysX system configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
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
            m_windConfiguration == other.m_windConfiguration
            ;
    }

    bool PhysXSystemConfiguration::operator!=(const PhysXSystemConfiguration& other) const
    {
        return !(*this == other);
    }
}
