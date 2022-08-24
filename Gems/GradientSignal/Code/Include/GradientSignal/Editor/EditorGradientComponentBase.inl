/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace GradientSignal
{
    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGradientComponentBase, BaseClassType>()
                ->Version(2)
                ->Field("Previewer", &EditorGradientComponentBase::m_previewer)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorGradientComponentBase>("GradientComponentBase", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")

                    ->DataElement(
                        AZ::Edit::UIHandlers::Default, &EditorGradientComponentBase::m_previewer, "Previewer", "Gradient Previewer")
                    ;
            }
        }
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::Activate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());

        SetSamplerOwnerEntity(m_configuration, GetEntityId());

        // Validation needs to happen after the ownerEntity is set in case the validation needs that data
        if (!ValidateGradientEntityIds(m_configuration))
        {
            SetDirty();
        }

        BaseClassType::Activate();

        m_previewer.Activate(GetEntityId());
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::Deactivate()
    {
        m_previewer.Deactivate();

        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        BaseClassType::Deactivate();
    }

    template<typename TComponent, typename TConfiguration>
    void GradientSignal::EditorGradientComponentBase<TComponent, TConfiguration>::OnCompositionChanged()
    {
        m_previewer.RefreshPreview();
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorGradientComponentBase<TComponent, TConfiguration>::ConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = m_previewer.CancelPreviewRendering();

        auto refreshResult = BaseClassType::ConfigurationChanged();

        // Refresh any of the previews that we cancelled that were still in progress so they can be completed
        m_previewer.RefreshPreviews(entityIds);

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call RefreshPreview explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return refreshResult;
    }
}
