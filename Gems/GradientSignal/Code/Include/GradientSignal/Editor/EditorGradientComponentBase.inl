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

#pragma once

namespace GradientSignal
{
    template <typename TComponent, typename TConfiguration>
    bool EditorGradientComponentBase<TComponent, TConfiguration>::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        // Changed EditorGradientComponentBase to inherit from EditorWrappedComponentBase
        if (classElement.GetVersion() < 1)
        {
            TConfiguration configData;
            AzToolsFramework::Components::EditorComponentBase editorComponentBaseData;

            classElement.FindSubElementAndGetData(AZ_CRC("Configuration", 0xa5e2a5d7), configData);
            classElement.FindSubElementAndGetData(AZ_CRC("BaseClass1", 0xd4925735), editorComponentBaseData);

            classElement.RemoveElementByName(AZ_CRC("Configuration", 0xa5e2a5d7));
            classElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735));

            BaseClassType wrappedComponentBaseInstance;
            int baseIndex = classElement.AddElementWithData(context, "BaseClass1", wrappedComponentBaseInstance);

            auto& wrappedComponentBaseElement = classElement.GetSubElement(baseIndex);
            auto* editorComponentBaseElement = wrappedComponentBaseElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            auto* configurationElement = wrappedComponentBaseElement.FindSubElement(AZ_CRC("Configuration", 0xa5e2a5d7));

            editorComponentBaseElement->SetData(context, editorComponentBaseData);
            configurationElement->SetData(context, configData);

            return true;
        }

        return true;
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorGradientComponentBase, BaseClassType>()
                ->Version(1, &EditorGradientComponentBase::VersionConverter)
                ->Field("PreviewEntity", &EditorGradientComponentBase::m_previewEntityId)
                ->Field("PreviewPosition", &EditorGradientComponentBase::m_previewPosition)
                ->Field("PreviewSize", &EditorGradientComponentBase::m_previewSize)
                ->Field("ConstrainToShape", &EditorGradientComponentBase::m_constrainToShape);

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorGradientComponentBase>("GradientComponentBase", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ->UIElement(AZ_CRC("GradientPreviewer", 0x1dbbba45), "Previewer")
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")
                    ->Attribute(AZ_CRC("GradientEntity", 0xe8531817), &EditorGradientComponentBase::GetGradientEntityId)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview Settings")
                    ->DataElement(0, &EditorGradientComponentBase::m_previewEntityId, "Pin Preview to Shape", "The entity whose shape represents the bounds to render the gradient preview")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientComponentBase::PreviewSettingsAndSettingsVisibilityChanged)
                    ->DataElement(0, &EditorGradientComponentBase::m_previewPosition, "Preview Position", "Center of the preview bounds")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientComponentBase::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorGradientComponentBase::GetPreviewPositionVisibility)
                    ->DataElement(0, &EditorGradientComponentBase::m_previewSize, "Preview Size", "Size of the preview bounds")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientComponentBase::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorGradientComponentBase::GetPreviewSizeVisibility)
                    ->DataElement(0, &EditorGradientComponentBase::m_constrainToShape, "Constrain to Shape", "If checked, only renders the parts of the gradient inside the component's shape and not its entire bounding box")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorGradientComponentBase::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorGradientComponentBase::GetPreviewConstrainToShapeVisibility)
                    ;
            }
        }
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::Activate()
    {
        m_gradientEntityId = GetEntityId();
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        GradientPreviewContextRequestBus::Handler::BusConnect(GetEntityId());

        SetSamplerOwnerEntity(m_configuration, GetEntityId());

        // Validation needs to happen after the ownerEntity is set in case the validation needs that data
        if (!ValidateGradientEntityIds(m_configuration))
        {
            SetDirty();
        }

        BaseClassType::Activate();

        if (!m_previewEntityId.IsValid())
        {
            // Default our preview entity to this entity
            m_previewEntityId = GetEntityId();
            SetDirty();
        }

        UpdatePreviewSettings();
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::Deactivate()
    {
        // If the preview shouldn't be active, use an invalid entityId
        m_gradientEntityId = AZ::EntityId();

        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientPreviewContextRequestBus::Handler::BusDisconnect();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();

        BaseClassType::Deactivate();
    }

    template<typename TComponent, typename TConfiguration>
    void GradientSignal::EditorGradientComponentBase<TComponent, TConfiguration>::OnCompositionChanged()
    {
        UpdatePreviewSettings();
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorGradientComponentBase<TComponent, TConfiguration>::ConfigurationChanged()
    {
        // Cancel any pending preview refreshes before locking, to help ensure the preview itself isn't holding the lock
        auto entityIds = CancelPreviewRendering();

         // block anyone from accessing the buses while the editor deactivates and re-activates the contained component.
        auto& surfaceDataSystemRequestBusContext = SurfaceData::SurfaceDataSystemRequestBus::GetOrCreateContext(false);
        auto& gradientRequestBusContextContext = GradientRequestBus::GetOrCreateContext(false);
        AZStd::scoped_lock< decltype(surfaceDataSystemRequestBusContext.m_contextMutex),
                            decltype(gradientRequestBusContextContext.m_contextMutex)
                          > 
                scopeLock(  surfaceDataSystemRequestBusContext.m_contextMutex,
                            gradientRequestBusContextContext.m_contextMutex);
        auto refreshResult = BaseClassType::ConfigurationChanged();

        // Refresh any of the previews that we cancelled that were still in progress so they can be completed
        for (auto entityId : entityIds)
        {
            GradientSignal::GradientPreviewRequestBus::Event(entityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
        }

        // This OnCompositionChanged notification will refresh our own preview so we don't need to call UpdatePreviewSettings explicitly
        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return refreshResult;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::EntityId EditorGradientComponentBase<TComponent, TConfiguration>::GetPreviewEntity() const
    {
        return m_previewEntityId;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::Aabb EditorGradientComponentBase<TComponent, TConfiguration>::GetPreviewBounds() const
    {
        AZ::Vector3 position = m_previewPosition;

        //if a shape entity was supplied, attempt to use its shape bounds or position
        if (m_previewEntityId.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_previewEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return bounds;
            }

            AZ::TransformBus::EventResult(position, m_previewEntityId, &AZ::TransformBus::Events::GetWorldTranslation);
        }

        return AZ::Aabb::CreateCenterHalfExtents(position, m_previewSize / 2.0f);
    }

    template <typename TComponent, typename TConfiguration>
    bool EditorGradientComponentBase<TComponent, TConfiguration>::GetConstrainToShape() const
    {
        return m_constrainToShape && m_previewEntityId.IsValid();
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::OnSelected()
    {
        UpdatePreviewSettings();
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::OnDeselected()
    {
        UpdatePreviewSettings();
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorGradientComponentBase<TComponent, TConfiguration>::GetPreviewPositionVisibility() const
    {
        return m_previewEntityId.IsValid() ? AZ::Edit::PropertyVisibility::Hide : AZ::Edit::PropertyVisibility::Show;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorGradientComponentBase<TComponent, TConfiguration>::GetPreviewSizeVisibility() const
    {
        if (m_previewEntityId.IsValid())
        {
            AZ::Aabb bounds = AZ::Aabb::CreateNull();
            LmbrCentral::ShapeComponentRequestsBus::EventResult(bounds, m_previewEntityId, &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);
            if (bounds.IsValid())
            {
                return AZ::Edit::PropertyVisibility::Hide;
            }
        }
        return AZ::Edit::PropertyVisibility::Show;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorGradientComponentBase<TComponent, TConfiguration>::GetPreviewConstrainToShapeVisibility() const
    {
        return m_previewEntityId.IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorGradientComponentBase<TComponent, TConfiguration>::PreviewSettingsAndSettingsVisibilityChanged() const
    {
        UpdatePreviewSettings();

        // We've changed the visibility of one or more properties, so refresh the entire component.
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    template <typename TComponent, typename TConfiguration>
    void EditorGradientComponentBase<TComponent, TConfiguration>::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration change)
        GradientSignal::GradientPreviewRequestBus::Event(m_gradientEntityId, &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

    template <typename TComponent, typename TConfiguration>
    AzToolsFramework::EntityIdList EditorGradientComponentBase<TComponent, TConfiguration>::CancelPreviewRendering() const
    {
        AzToolsFramework::EntityIdList entityIds;
        AZ::EBusAggregateResults<AZ::EntityId> cancelledPreviews;
        GradientSignal::GradientPreviewRequestBus::BroadcastResult(cancelledPreviews, &GradientSignal::GradientPreviewRequestBus::Events::CancelRefresh);

        // Gather up the EntityIds for any previews that were in progress when we cancelled them
        for (auto entityId : cancelledPreviews.values)
        {
            if (entityId.IsValid())
            {
                entityIds.push_back(entityId);
            }
        }

        return entityIds;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::EntityId EditorGradientComponentBase<TComponent, TConfiguration>::GetGradientEntityId() const
    {
        return m_gradientEntityId;
    }

}
