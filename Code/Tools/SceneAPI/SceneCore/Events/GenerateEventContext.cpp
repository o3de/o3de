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

#include <SceneAPI/SceneCore/Events/GenerateEventContext.h>

namespace AZ::SceneAPI::Events
{
    /////////////
    // PreGenerateEventContext
    /////////////

    GenerateEventBaseContext::GenerateEventBaseContext(Containers::Scene& scene, const char* platformIdentifier)
        : m_scene(scene)
        , m_platformIdentifier(platformIdentifier)
    {
    }

    Containers::Scene& GenerateEventBaseContext::GetScene() const
    {
        return m_scene;
    }

    const char* GenerateEventBaseContext::GetPlatformIdentifier() const
    {
        return m_platformIdentifier;
    }
} // namespace AZ::SceneAPI::Events
