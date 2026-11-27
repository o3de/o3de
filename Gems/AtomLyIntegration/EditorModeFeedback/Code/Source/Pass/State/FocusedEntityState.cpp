/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/FocusedEntityState.h>

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AZ::Render
{
    static PassNameList CreateFocusedEntityChildPasses()
    {
        // Effect chain for the non-focused entities.
        return PassNameList
        {
            // Black and white effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeDesaturationTemplate"),

            // Darkening effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeTintTemplate"),

            // Blurring effect for unfocused entities (scaled by distance)
            AZ::Name("EditorModeBlurParentTemplate")
        };
    }

    FocusedEntityState::FocusedEntityState()
        : EditorStateBase(EditorState::FocusMode, "FocusMode", CreateFocusedEntityChildPasses())
    {
        SetEnabled(AzToolsFramework::PrefabEditModeEffectEnabled());
    }

    void FocusedEntityState::UpdatePassDataForPipelines()
    {
        m_focusRoots.clear();
        const auto* focus = AZ::Interface<AzToolsFramework::Prefab::PrefabFocusInterface>::Get();
        if (auto* contexts = RPI::ViewportContextRequests::Get(); focus && contexts)
        {
            contexts->EnumerateViewportContexts([this, focus](RPI::ViewportContextPtr context)
            {
                if (context && context->GetCurrentPipeline())
                {
                    if (const auto root = focus->GetViewportFocusRoot(context->GetId()); root.IsValid())
                    {
                        m_focusRoots.emplace(context->GetCurrentPipeline().get(), root);
                    }
                }
            });
        }
    }

    bool FocusedEntityState::IsEnabled(const RPI::RenderPipeline* pipeline) const
    {
        return EditorStateBase::IsEnabled(pipeline) && m_focusRoots.contains(pipeline);
    }

    AzToolsFramework::EntityIdList FocusedEntityState::GetMaskedEntities(const RPI::RenderPipeline* pipeline) const
    {
        AzToolsFramework::EntityIdList entities;
        if (const auto it = m_focusRoots.find(pipeline); it != m_focusRoots.end())
        {
            AZ::TransformBus::EventResult(entities, it->second, &AZ::TransformInterface::GetEntityAndAllDescendants);
        }
        return entities;
    }
} // namespace AZ::Render
