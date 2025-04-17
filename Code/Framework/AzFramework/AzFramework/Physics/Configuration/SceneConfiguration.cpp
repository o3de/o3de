/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(SceneConfiguration, AZ::SystemAllocator);

    /*static*/ void SceneConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneConfiguration>()
                ->Version(2)
                ->Field("Name", &SceneConfiguration::m_sceneName)
                ->Field("WorldBounds", &SceneConfiguration::m_worldBounds)
                ->Field("Gravity", &SceneConfiguration::m_gravity)
                ->Field("EnableCcd", &SceneConfiguration::m_enableCcd)
                ->Field("MaxCcdPasses", &SceneConfiguration::m_maxCcdPasses)
                ->Field("EnableCcdResweep", &SceneConfiguration::m_enableCcdResweep)
                ->Field("EnableActiveActors", &SceneConfiguration::m_enableActiveActors)
                ->Field("EnablePcm", &SceneConfiguration::m_enablePcm)
                ->Field("BounceThresholdVelocity", &SceneConfiguration::m_bounceThresholdVelocity)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SceneConfiguration>("Scene Configuration", "Default scene configuration")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_worldBounds, "World Bounds", "World bounds")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_gravity, "Gravity", "Gravity")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Continuous Collision Detection")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_enableCcd, "Enable CCD", "Enabled continuous collision detection in the world")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_maxCcdPasses,
                        "Max CCD Passes", "Maximum number of continuous collision detection passes")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &SceneConfiguration::GetCcdVisibility)
                    ->Attribute(AZ::Edit::Attributes::Min, 1u)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_enableCcdResweep,
                        "Enable CCD Resweep", "Enable a more accurate but more expensive continuous collision detection method")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &SceneConfiguration::GetCcdVisibility)
                    ->EndGroup()

                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_enablePcm, "Persistent Contact Manifold", "Enabled the persistent contact manifold narrow-phase algorithm")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SceneConfiguration::m_bounceThresholdVelocity,
                        "Bounce Threshold Velocity", "Relative velocity below which colliding objects will not bounce")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ;
            }
        }
    }

    /*static*/ SceneConfiguration SceneConfiguration::CreateDefault()
    {
        return SceneConfiguration();
    }

    bool SceneConfiguration::operator==(const SceneConfiguration& other) const
    {
        return m_sceneName == other.m_sceneName
            && m_enableCcd == other.m_enableCcd
            && m_enableCcdResweep == other.m_enableCcdResweep
            && m_enableActiveActors == other.m_enableActiveActors
            && m_enablePcm == other.m_enablePcm
            && m_kinematicFiltering == other.m_kinematicFiltering
            && m_kinematicStaticFiltering == other.m_kinematicStaticFiltering
            && m_customUserData == other.m_customUserData
            && m_maxCcdPasses == other.m_maxCcdPasses
            && AZ::IsClose(m_bounceThresholdVelocity, other.m_bounceThresholdVelocity)
            && m_gravity.IsClose(other.m_gravity)
            && m_worldBounds == other.m_worldBounds
        ;
    }

    bool SceneConfiguration::operator!=(const SceneConfiguration& other) const
    {
        return !(*this == other);
    }

    AZ::Crc32 SceneConfiguration::GetCcdVisibility() const
    {
        return m_enableCcd ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }
}
