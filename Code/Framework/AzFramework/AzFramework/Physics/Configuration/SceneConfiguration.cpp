/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/Configuration/SceneConfiguration.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(SceneConfiguration, AZ::SystemAllocator, 0);

    /*static*/ void SceneConfiguration::Reflect(AZ::ReflectContext* context)
    {
        Physics::WorldConfiguration::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SceneConfiguration>()
                ->Version(1)
                ->Field("LegacyConfig", &SceneConfiguration::m_legacyConfiguration)
                ->Field("LegacyId", &SceneConfiguration::m_legacyId)
                ->Field("Name", &SceneConfiguration::m_sceneName)
                ;
        }
    }

    /*static*/ SceneConfiguration SceneConfiguration::CreateDefault()
    {
        return SceneConfiguration();
    }

    bool SceneConfiguration::operator==(const SceneConfiguration& other) const
    {
        return m_legacyId == other.m_legacyId
            && m_sceneName == other.m_sceneName
            && m_legacyConfiguration == other.m_legacyConfiguration
            ;
    }

    bool SceneConfiguration::operator!=(const SceneConfiguration& other) const
    {
        return !(*this == other);
    }
}
