/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Render/GameIntersectorComponent.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Render/Intersector.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

namespace AzFramework
{
    namespace RenderGeometry
    {
        void GameIntersectorComponent::Activate()
        {
            AzFramework::EntityContextId gameContextId;
            AzFramework::GameEntityContextRequestBus::BroadcastResult(
                gameContextId, &AzFramework::GameEntityContextRequests::GetGameEntityContextId);

            m_intersector = AZStd::make_unique<AzFramework::RenderGeometry::Intersector>(gameContextId);
        }

        void GameIntersectorComponent::Deactivate()
        {
            m_intersector.reset();
        }

        void GameIntersectorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<GameIntersectorComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }
    }
}
