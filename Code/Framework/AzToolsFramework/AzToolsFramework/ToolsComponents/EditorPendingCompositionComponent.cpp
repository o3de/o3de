/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorPendingCompositionComponent.h"

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

namespace AzToolsFramework
{
    namespace Components
    {
        AZ_CLASS_ALLOCATOR_IMPL(EditorPendingCompositionComponentSerializer, AZ::SystemAllocator);

        AZ::JsonSerializationResult::Result EditorPendingCompositionComponentSerializer::Load(
            void* outputValue,
            [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<EditorPendingCompositionComponent>() == outputValueTypeId,
                "Unable to deserialize EditorPendingCompositionComponent from JSON because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            auto componentInstance = reinterpret_cast<EditorPendingCompositionComponent*>(outputValue);
            AZ_Assert(componentInstance, "Output value for EditorPendingCompositionComponentSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                JSR::ResultCode pendingComponentsResult(JSR::Tasks::ReadField);

                AZStd::unordered_map<AZStd::string, AZ::Component*> componentMap;

                auto pendingComponentsIter = inputValue.FindMember("PendingComponents");
                if (pendingComponentsIter != inputValue.MemberEnd())
                {
                    if (pendingComponentsIter->value.IsArray())
                    {
                        // If the serialized data is an array type, then convert the data to a map.
                        AZStd::vector<AZ::Component*> componentVector;
                        pendingComponentsResult = ContinueLoadingFromJsonObjectField(
                            &componentVector, azrtti_typeid<decltype(componentVector)>(), inputValue, "PendingComponents", context);

                        AZ::EntityUtils::ConvertComponentVectorToMap(componentVector, componentMap);
                    }
                    else
                    {
                        pendingComponentsResult = ContinueLoadingFromJsonObjectField(
                            &componentMap, azrtti_typeid<decltype(componentMap)>(), inputValue, "PendingComponents", context);
                    }

                    static AZ::TypeId genericComponentWrapperTypeId = azrtti_typeid<GenericComponentWrapper>();

                    for (auto& [componentKey, component] : componentMap)
                    {
                        // If the component didn't serialize (i.e. is null) or the underlying type is GenericComponentWrapper,
                        // the template is null and the component should not be added.
                        if (component && component->GetUnderlyingComponentType() != genericComponentWrapperTypeId)
                        {
                            componentInstance->m_pendingComponents.emplace(componentKey, component);
                        }
                    }
                }

                result.Combine(pendingComponentsResult);
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded EditorPendingCompositionComponent information."
                                                                  : "Failed to load EditorPendingCompositionComponent information.");
        }

        void EditorPendingCompositionComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorPendingCompositionComponent, EditorComponentBase>()
                    ->Version(1)
                    ->Field("PendingComponents", &EditorPendingCompositionComponent::m_pendingComponents)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorPendingCompositionComponent>("Pending Components", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                        ;
                }
            }

            if (auto jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonRegistration->Serializer<EditorPendingCompositionComponentSerializer>()->HandlesType<EditorPendingCompositionComponent>();
            }
        }

        void EditorPendingCompositionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorPendingCompositionService", 0x6b5b794f));
        }

        void EditorPendingCompositionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("EditorPendingCompositionService", 0x6b5b794f));
        }

        void EditorPendingCompositionComponent::GetPendingComponents(AZStd::vector<AZ::Component*>& components)
        {
            for (auto const& pair : m_pendingComponents)
            {
                components.insert(components.end(), pair.second);
            }
        }

        void EditorPendingCompositionComponent::AddPendingComponent(AZ::Component* componentToAdd)
        {
            AZ_Assert(componentToAdd, "Unable to add a pending component that is nullptr.");

            AZStd::string componentAlias = componentToAdd->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to add a pending component that has an empty component alias.");

            bool found = m_pendingComponents.find(componentAlias) != m_pendingComponents.end();
            AZ_Assert(!found, "Unable to add a pending component that was added already.");

            if (componentToAdd && !found)
            {
                m_pendingComponents.emplace(AZStd::move(componentAlias), componentToAdd);

                bool isDuringUndo = false;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsDuringUndoRedo);
                if (isDuringUndo)
                {
                    SetDirty();
                }
            }
        }

        void EditorPendingCompositionComponent::RemovePendingComponent(AZ::Component* componentToRemove)
        {
            AZ_Assert(componentToRemove, "Unable to remove a pending component that is nullptr.");

            AZStd::string componentAlias = componentToRemove->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to remove a pending component that has an empty component alias.");

            bool found = m_pendingComponents.find(componentAlias) != m_pendingComponents.end();
            AZ_Assert(found, "Unable to remove a pending component that has not been added.");

            if (componentToRemove && found)
            {
                m_pendingComponents.erase(componentAlias);

                bool isDuringUndo = false;
                AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(isDuringUndo, &AzToolsFramework::ToolsApplicationRequestBus::Events::IsDuringUndoRedo);
                if (isDuringUndo)
                {
                    SetDirty();
                }
            }
        };

        bool EditorPendingCompositionComponent::IsComponentPending(const AZ::Component* componentToCheck)
        {
            AZ_Assert(componentToCheck, "Unable to check a component that is nullptr.");

            AZStd::string componentAlias = componentToCheck->GetSerializedIdentifier();
            AZ_Assert(!componentAlias.empty(), "Unable to check a component that has an empty component alias.");

            auto componentIt = m_pendingComponents.find(componentAlias);

            if (componentIt != m_pendingComponents.end())
            {
                bool sameComponent = componentIt->second == componentToCheck;
                AZ_Assert(sameComponent, "The component to check shares the same alias but is a different object "
                    "compared to the one referenced in the composition component.");

                return sameComponent;
            }

            return false;
        }

        EditorPendingCompositionComponent::~EditorPendingCompositionComponent()
        {
            for (auto& pair : m_pendingComponents)
            {
                delete pair.second;
            }
            m_pendingComponents.clear();

            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorPendingCompositionRequestBus::Handler::BusDisconnect();
        }

        void EditorPendingCompositionComponent::Init()
        {
            EditorComponentBase::Init();

            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorPendingCompositionRequestBus::Handler::BusConnect(GetEntityId());

            // Set the entity for each pending component.
            for (auto const& [componentAlias, pendingComponent] : m_pendingComponents)
            {
                // It's possible to get null components in the list if errors occur during serialization.
                // Guard against that case so that the code won't crash.
                if (pendingComponent)
                {
                    auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(pendingComponent);
                    AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");

                    editorComponentBaseComponent->SetEntity(GetEntity());
                }
            }
        }

        void EditorPendingCompositionComponent::Activate()
        {
            EditorComponentBase::Activate();
        }

        void EditorPendingCompositionComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
        }
    } // namespace Components
} // namespace AzToolsFramework
