/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/EditorSystemComponent.h>
#include <EMStudio/AtomRenderPlugin.h>

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

namespace AZ::EMotionFXAtom
{
    void EditorSystemComponent::Reflect(ReflectContext* context)
    {
        if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
        {
            serialize->Class<EditorSystemComponent, Component>()->Version(0);
        }
    }

    void EditorSystemComponent::Activate()
    {
        EMotionFX::Integration::SystemNotificationBus::Handler::BusConnect();
    }

    void EditorSystemComponent::Deactivate()
    {
        EMotionFX::Integration::SystemNotificationBus::Handler::BusDisconnect();
    }

    void EditorSystemComponent::OnRegisterPlugin()
    {
        EMStudio::PluginManager* pluginManager = EMStudio::EMStudioManager::GetInstance()->GetPluginManager();
        pluginManager->RegisterPlugin(aznew EMStudio::AtomRenderPlugin());
    }
} // namespace AZ::EMotionFXAtom
