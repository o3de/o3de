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
