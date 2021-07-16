/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Render/EditorIntersectorComponent.h>

#include <AzFramework/Entity/EntityContext.h>
#include <AzFramework/Render/Intersector.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AzToolsFramework
{
    namespace Components
    {
        void EditorIntersectorComponent::Activate()
        {
            AzFramework::EntityContextId editorContextId;
            AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
                editorContextId, &AzToolsFramework::EditorEntityContextRequests::GetEditorEntityContextId);

            m_intersector = AZStd::make_unique<AzFramework::RenderGeometry::Intersector>(editorContextId);
        }

        void EditorIntersectorComponent::Deactivate()
        {
            m_intersector.reset();
        }

        void EditorIntersectorComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorIntersectorComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }
    } // namespace Components

} // namespace AzToolsFramework
