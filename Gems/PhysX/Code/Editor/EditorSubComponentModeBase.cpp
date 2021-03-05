
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
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <Editor/EditorJointTypeDrawer.h>
#include <Editor/EditorSubComponentModeBase.h>

namespace PhysX
{
    EditorSubComponentModeBase::EditorSubComponentModeBase(
        const AZ::EntityComponentIdPair& entityComponentIdPair
        , const AZ::Uuid& componentType
        , const AZStd::string& name)
        : m_entityComponentId(entityComponentIdPair),
        m_name(name)
    {
        // The first time this is called, no object will respond to this bus call as no object is connected at the address,
        // and m_jointTypeDrawer will remain as a nullptr.
        EditorJointTypeDrawerBus::EventResult(m_jointTypeDrawer,
            EditorJointTypeDrawerId(componentType, EditorSubComponentModeNameCrc(name)),
            &EditorJointTypeDrawerBus::Events::GetEditorJointTypeDrawer);

        if (!m_jointTypeDrawer)
        {
            // Once this is called, the bus call to GetEditorJointTypeDrawer above will no longer get a null response, until m_jointTypeDrawer is destroyed.
            m_jointTypeDrawer = AZStd::make_shared<EditorJointTypeDrawer>(componentType,
                AzToolsFramework::GetEntityContextId(),
                m_name);
        }
    }
} // namespace PhysX
