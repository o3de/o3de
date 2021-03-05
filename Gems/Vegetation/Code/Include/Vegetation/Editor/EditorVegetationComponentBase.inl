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

namespace Vegetation
{
    template<typename TComponent, typename TConfiguration>
    void EditorVegetationComponentBase<TComponent, TConfiguration>::OnCryEditorEndLevelExport(bool /*success*/)
    {
        // Restore the activation state of our components after the export is complete.
        if (m_visible)
        {
            m_component.Activate();
        }
    }

    template<typename TComponent, typename TConfiguration>
    void EditorVegetationComponentBase<TComponent, TConfiguration>::OnCryEditorBeginLevelExport()
    {
        // We need to deactivate our game components at the start of level exports because any vegetation meshes that are loaded
        // or instances that are spawned can end up in our static vegetation level data.
        m_component.Deactivate();
    }

    template<typename TComponent, typename TConfiguration>
    AZ::u32 EditorVegetationComponentBase<TComponent, TConfiguration>::ConfigurationChanged()
    {
        auto refreshResult = BaseClassType::ConfigurationChanged();

        LmbrCentral::DependencyNotificationBus::Event(GetEntityId(), &LmbrCentral::DependencyNotificationBus::Events::OnCompositionChanged);

        return refreshResult;
    }

    template<typename TComponent, typename TConfiguration>
    void EditorVegetationComponentBase<TComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
    {
        BaseClassType::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);

        if (serializeContext)
        {
            serializeContext->Class<EditorVegetationComponentBase, BaseClassType>()
                ->Version(0)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorVegetationComponentBase>(
                    "Editor Vegetation Component Base", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::DisplayOrder, 50) // There's no special meaning to 50, we just need this class to move down and display below any children
                    ;
            }
        }
    }

    template<typename TComponent, typename TConfiguration>
    void EditorVegetationComponentBase<TComponent, TConfiguration>::Activate()
    {
        GradientSignal::SetSamplerOwnerEntity(m_configuration, GetEntityId());
        BaseClassType::Activate();
        CrySystemEventBus::Handler::BusConnect();
    }

    template<typename TComponent, typename TConfiguration>
    void EditorVegetationComponentBase<TComponent, TConfiguration>::Deactivate()
    {
        CrySystemEventBus::Handler::BusDisconnect();
        BaseClassType::Deactivate();
    }

    inline AZ::SerializeContext::DataElementNode* GetParentByIndex(AZ::SerializeContext::DataElementNode& node, int level)
    {
        AZ::SerializeContext::DataElementNode* searchNode = &node;

        for (int i = 0; i < level; ++i)
        {
            searchNode = searchNode->FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));

            if (searchNode == nullptr)
            {
                return nullptr;
            }
        }

        return searchNode;
    }

    inline void CopySubElements(AZ::SerializeContext::DataElementNode& source, AZ::SerializeContext::DataElementNode& target)
    {
        for (int subElementIndex = 0; subElementIndex < source.GetNumSubElements(); ++subElementIndex)
        {
            auto& subElement = source.GetSubElement(subElementIndex);
            target.RemoveElementByName(subElement.GetName());
            target.AddElement(subElement);
        }
    }

    template<typename TComponent, typename TConfiguration>
    bool EditorVegetationComponentBaseVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 1)
        {
            TConfiguration configData;
            AzToolsFramework::Components::EditorComponentBase editorComponentBaseData;

            AZ::SerializeContext::DataElementNode* oldEditorComponentBaseElement = GetParentByIndex(classElement, 2);

            if (!oldEditorComponentBaseElement || !oldEditorComponentBaseElement->GetData(editorComponentBaseData))
            {
                AZ_Error("Vegetation", false, "Failed to find or get data for EditorComponentBase element");
                return false;
            }

            if (!classElement.FindSubElementAndGetData(AZ_CRC("Configuration", 0xa5e2a5d7), configData))
            {
                AZ_Error("Vegetation", false, "Failed to find and get data for Configuration element")
                    return false;
            }

            if (!classElement.RemoveElementByName(AZ_CRC("Configuration", 0xa5e2a5d7))
                || !classElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735)))
            {
                AZ_Error("Vegetation", false, "Failed to remove Configuration or BaseClass1 element");
                return false;
            }

            EditorVegetationComponentBase<TComponent, TConfiguration> vegetationComponentBaseData;
            int baseIndex = classElement.AddElementWithData(context, "BaseClass1", vegetationComponentBaseData);

            // Find the EditorWrappedComponentBase and copy in the Configuration and EditorComponentBase data
            auto* editorWrappedComponentBase = GetParentByIndex(classElement, 2);

            if (!editorWrappedComponentBase)
            {
                AZ_Error("Vegetation", false, "Failed to find EditorWrappedComponentBase element");
                return false;
            }

            auto* editorComponentBaseElement = editorWrappedComponentBase->FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));
            auto* configElement = editorWrappedComponentBase->FindSubElement(AZ_CRC("Configuration", 0xa5e2a5d7));

            if (!editorComponentBaseElement || !configElement)
            {
                AZ_Error("Vegetation", false, "Failed to find BaseClass1 element or Configuration element");
                return false;
            }

            if (!editorComponentBaseElement->SetData(context, editorComponentBaseData))
            {
                AZ_Error("Vegetation", false, "Failed to SetData for EditorComponentBase element");
                return false;
            }

            if (!configElement->SetData(context, configData))
            {
                AZ_Error("Vegetation", false, "Failed to SetData for Configuration element");
                return false;
            }
        }

        return true;
    }
}
