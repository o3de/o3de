/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace Vegetation
{
    template<typename TComponent, typename TConfiguration>
    bool EditorAreaComponentBaseVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 1)
        {
            TConfiguration configData;
            AzToolsFramework::Components::EditorComponentBase editorComponentBaseData;

            AZ::SerializeContext::DataElementNode oldEditorAreaComponentBaseElement = *GetParentByIndex(classElement, 1);
            AZ::SerializeContext::DataElementNode* oldEditorComponentBaseElement = GetParentByIndex(classElement, 3);

            if (!oldEditorComponentBaseElement->GetData(editorComponentBaseData))
            {
                return false;
            }

            if (!classElement.FindSubElementAndGetData(AZ_CRC_CE("Configuration"), configData))
            {
                return false;
            }

            if (!oldEditorAreaComponentBaseElement.RemoveElementByName(AZ_CRC_CE("BaseClass1"))
                || !classElement.RemoveElementByName(AZ_CRC_CE("Configuration"))
                || !classElement.RemoveElementByName(AZ_CRC_CE("BaseClass1")))
            {
                return false;
            }

            EditorAreaComponentBase<TComponent, TConfiguration> areaComponentBaseData;
            int baseIndex = classElement.AddElementWithData(context, "BaseClass1", areaComponentBaseData);

            // Copy in the EditorAreaComponentBase settings
            auto& editorAreaComponentBaseElement = classElement.GetSubElement(baseIndex);
            CopySubElements(oldEditorAreaComponentBaseElement, editorAreaComponentBaseElement);

            // Find the EditorWrappedComponentBase and copy in the Configuration and EditorComponentBase data
            auto* editorWrappedComponentBase = GetParentByIndex(classElement, 3);

            if (!editorWrappedComponentBase)
            {
                AZ_Error("Vegetation", false, "Failed to get EditorWrappedComponentBase element");
                return false;
            }

            auto* editorComponentBaseElement = editorWrappedComponentBase->FindSubElement(AZ_CRC_CE("BaseClass1"));
            auto* configElement = editorWrappedComponentBase->FindSubElement(AZ_CRC_CE("Configuration"));

            if (!editorComponentBaseElement || !configElement)
            {
                AZ_Error("Vegetation", false, "Failed to get EditorComponentBase element or Configuration element");
                return false;
            }

            if (!editorComponentBaseElement->SetData(context, editorComponentBaseData))
            {
                AZ_Error("Vegetation", false, "Failed to get SetData on EditorComponentBase element");
                return false;
            }

            if (!configElement->SetData(context, configData))
            {
                AZ_Error("Vegetation", false, "Failed to get SetData on Configuration element");
                return false;
            }
        }

        return true;
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAreaComponentBase, BaseClassType>()
                ->Version(0)
                ->Field("OverridePreviewSettings", &EditorAreaComponentBase::m_overridePreviewSettings)
                ->Field("PreviewEntity", &EditorAreaComponentBase::m_previewEntityId)
                ->Field("PreviewPosition", &EditorAreaComponentBase::m_previewPosition)
                ->Field("PreviewSize", &EditorAreaComponentBase::m_previewSize)
                ->Field("ConstrainToShape", &EditorAreaComponentBase::m_constrainToShape)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAreaComponentBase>("EditorAreaComponentBase", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Preview Settings")
                    ->DataElement(0, &EditorAreaComponentBase::m_overridePreviewSettings, "Override Preview Settings", "")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAreaComponentBase::PreviewSettingsAndSettingsVisibilityChanged)
                    ->DataElement(0, &EditorAreaComponentBase::m_previewEntityId, "Pin Preview to Shape", "The entity whose shape represents the bounds to render the gradient preview")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAreaComponentBase::PreviewSettingsAndSettingsVisibilityChanged)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorAreaComponentBase::GetPreviewGroupVisibility)
                    ->DataElement(0, &EditorAreaComponentBase::m_previewPosition, "Preview Position", "Center of the preview bounds")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAreaComponentBase::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorAreaComponentBase::GetPreviewPositionVisibility)
                    ->DataElement(0, &EditorAreaComponentBase::m_previewSize, "Preview Size", "Size of the preview bounds")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAreaComponentBase::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorAreaComponentBase::GetPreviewSizeVisibility)
                    ->DataElement(0, &EditorAreaComponentBase::m_constrainToShape, "Constrain to Shape", "If checked, only renders the parts of the gradient inside the component's shape and not its entire bounding box")
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &EditorAreaComponentBase::UpdatePreviewSettings)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &EditorAreaComponentBase::GetPreviewConstrainToShapeVisibility)
                    ;
            }
        }
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::Activate()
    {
        BaseClassType::Activate();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusConnect(GetEntityId());
        LmbrCentral::DependencyNotificationBus::Handler::BusConnect(GetEntityId());
        GradientSignal::GradientPreviewContextRequestBus::Handler::BusConnect(GetEntityId());

        if (!m_previewEntityId.IsValid())
        {
            m_previewEntityId = GetEntityId();
            SetDirty();
        }

        UpdatePreviewSettings();
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::Deactivate()
    {
        LmbrCentral::DependencyNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EntitySelectionEvents::Bus::Handler::BusDisconnect();
        GradientSignal::GradientPreviewContextRequestBus::Handler::BusDisconnect();
        BaseClassType::Deactivate();
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::OnCompositionChanged()
    {
        UpdatePreviewSettings();
    }

    template<typename TComponent, typename TConfiguration>
    AZ::EntityId EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewEntity() const
    {
        return m_overridePreviewSettings && m_previewEntityId.IsValid() ? m_previewEntityId : GetEntityId();
    }

    template<typename TComponent, typename TConfiguration>
    AZ::Aabb EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewBounds() const
    {
        if (m_overridePreviewSettings)
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

        AZ::EntityId id = GetEntityId();
        AZ::Aabb bounds = AZ::Aabb::CreateNull();
        AreaInfoBus::EventResult(bounds, id, &AreaInfoBus::Events::GetEncompassingAabb);
        return bounds;
    }

    template<typename TComponent, typename TConfiguration>
    bool EditorAreaComponentBase<TComponent, TConfiguration>::GetConstrainToShape() const
    {
        return m_overridePreviewSettings && m_constrainToShape && m_previewEntityId.IsValid();
    }

    template<typename TComponent, typename TConfiguration>
    GradientSignal::GradientPreviewContextPriority EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewContextPriority() const
    {
        return GradientSignal::GradientPreviewContextPriority::Superior;
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::OnSelected()
    {
        UpdatePreviewSettings();
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::OnDeselected()
    {
        UpdatePreviewSettings();
    }

    template<typename TComponent, typename TConfiguration>
    AZ::u32 EditorAreaComponentBase<TComponent, TConfiguration>::ConfigurationChanged()
    {
        auto refreshResult = BaseClassType::ConfigurationChanged();

        UpdatePreviewSettings();

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);
        return refreshResult;
    }

    template<typename TComponent, typename TConfiguration>
    AZ::u32 EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewGroupVisibility() const
    {
        return m_overridePreviewSettings ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    template<typename TComponent, typename TConfiguration>
    AZ::u32 EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewPositionVisibility() const
    {
        return m_overridePreviewSettings && !m_previewEntityId.IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    template<typename TComponent, typename TConfiguration>
    AZ::u32 EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewSizeVisibility() const
    {
        if (m_overridePreviewSettings)
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
        return AZ::Edit::PropertyVisibility::Hide;
    }

    template<typename TComponent, typename TConfiguration>
    AZ::u32 EditorAreaComponentBase<TComponent, TConfiguration>::GetPreviewConstrainToShapeVisibility() const
    {
        return m_overridePreviewSettings && m_previewEntityId.IsValid() ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }

    template <typename TComponent, typename TConfiguration>
    AZ::u32 EditorAreaComponentBase<TComponent, TConfiguration>::PreviewSettingsAndSettingsVisibilityChanged() const
    {
        UpdatePreviewSettings();

        // We've changed the visibility of one or more properties, so refresh the entire component.
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    template<typename TComponent, typename TConfiguration>
    void EditorAreaComponentBase<TComponent, TConfiguration>::UpdatePreviewSettings() const
    {
        // Trigger an update just for our specific preview (this means there was a preview-specific change, not an actual configuration change)
        GradientSignal::GradientPreviewRequestBus::Event(GetEntityId(), &GradientSignal::GradientPreviewRequestBus::Events::Refresh);
    }

} //namespace Vegetation
