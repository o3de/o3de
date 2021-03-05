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

#include <Atom/Feature/Utils/EditorRenderComponentAdapter.h>
#include <AzToolsFramework/Entity/EditorEntityInfoBus.h>

namespace AZ
{
    namespace Render
    {
        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        template<int TVersion>
        bool EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::ConvertToEditorRenderComponentAdapter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < TVersion)
            {
                // Get the and remove the EditorComponentAdapter base class data that was previpously serialized
                AzToolsFramework::Components::EditorComponentAdapter<TController, TRuntimeComponent, TConfiguration> oldBaseClassData;

                if (!classElement.FindSubElementAndGetData(AZ_CRC("BaseClass1", 0xd4925735), oldBaseClassData))
                {
                    AZ_Error("AZ::Render", false, "Failed to get BaseClass1 element");
                    return false;
                }

                if (!classElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735)))
                {
                    AZ_Error("AZ::Render", false, "Failed to remove BaseClass1 element");
                    return false;
                }

                // Replace the old base class data with EditorRenderComponentAdapter
                EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration> newBaseClassData;

                AZ::SerializeContext::DataElementNode& newBaseClassElement = classElement.GetSubElement(
                    classElement.AddElementWithData(context, "BaseClass1", newBaseClassData));

                // Overwrite EditorRenderComponentAdapter base class data with retrieved EditorComponentAdapter base class data
                if (!newBaseClassElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735)))
                {
                    AZ_Error("AZ::Render", false, "Failed to remove BaseClass1 element");
                    return false;
                }

                newBaseClassElement.AddElementWithData(context, "BaseClass1", oldBaseClassData);
            }

            return true;
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Reflect(AZ::ReflectContext* context)
        {
            BaseClass::Reflect(context);

            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorRenderComponentAdapter, BaseClass>()
                    ->Version(0)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EditorRenderComponentAdapter>(
                        "EditorRenderComponentAdapter", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ;
                }
            }
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::EditorRenderComponentAdapter(const TConfiguration& config)
            : BaseClass(config)
        {
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Activate()
        {
            BaseClass::Activate();
            AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler::BusConnect(this->GetEntityId());
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::Deactivate()
        {
            AzToolsFramework::EditorEntityVisibilityNotificationBus::Handler::BusDisconnect();
            BaseClass::Deactivate();
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        bool EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::IsVisible() const
        {
            bool visible = true;
            AzToolsFramework::EditorEntityInfoRequestBus::EventResult(visible, this->GetEntityId(), &AzToolsFramework::EditorEntityInfoRequestBus::Events::IsVisible);
            return visible;
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        bool EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::ShouldActivateController() const
        {
            return IsVisible();
        }

        template<typename TController, typename TRuntimeComponent, typename TConfiguration>
        void EditorRenderComponentAdapter<TController, TRuntimeComponent, TConfiguration>::OnEntityVisibilityChanged([[maybe_unused]] bool visibility)
        {
            this->m_controller.Deactivate();

            if (this->ShouldActivateController())
            {
                this->m_controller.Activate(this->GetEntityId());
            }
        }

    } // namespace Render
} // namespace AZ
