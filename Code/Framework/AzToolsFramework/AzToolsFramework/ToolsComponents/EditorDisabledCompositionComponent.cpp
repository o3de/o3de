/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorDisabledCompositionComponent.h"

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>

namespace AzToolsFramework
{
    namespace Components
    {
        AZ_CLASS_ALLOCATOR_IMPL(EditorDisabledCompositionComponentSerializer, AZ::SystemAllocator);

        AZ::JsonSerializationResult::Result EditorDisabledCompositionComponentSerializer::Load(
            void* outputValue,
            [[maybe_unused]] const AZ::Uuid& outputValueTypeId,
            const rapidjson::Value& inputValue,
            AZ::JsonDeserializerContext& context)
        {
            namespace JSR = AZ::JsonSerializationResult;

            AZ_Assert(
                azrtti_typeid<EditorDisabledCompositionComponent>() == outputValueTypeId,
                "Unable to deserialize EditorDisabledCompositionComponent from JSON because the provided type is %s.",
                outputValueTypeId.ToString<AZStd::string>().c_str());

            auto componentInstance = reinterpret_cast<EditorDisabledCompositionComponent*>(outputValue);
            AZ_Assert(componentInstance, "Output value for EditorDisabledCompositionComponentSerializer can't be null.");

            JSR::ResultCode result(JSR::Tasks::ReadField);
            {
                JSR::ResultCode disabledComponentsResult(JSR::Tasks::ReadField);

                AZStd::unordered_map<AZStd::string, AZ::Component*> componentMap;

                auto disabledComponentsIter = inputValue.FindMember("DisabledComponents");
                if (disabledComponentsIter != inputValue.MemberEnd())
                {
                    if (disabledComponentsIter->value.IsArray())
                    {
                        // If the serialized data is an array type, then convert the data to a map.
                        AZ::Entity::ComponentArrayType componentVector;
                        disabledComponentsResult = ContinueLoadingFromJsonObjectField(
                            &componentVector, azrtti_typeid<decltype(componentVector)>(), inputValue, "DisabledComponents", context);

                        AZ::EntityUtils::ConvertComponentVectorToMap(componentVector, componentMap);
                    }
                    else
                    {
                        disabledComponentsResult = ContinueLoadingFromJsonObjectField(
                            &componentMap, azrtti_typeid<decltype(componentMap)>(), inputValue, "DisabledComponents", context);
                    }

                    static AZ::TypeId genericComponentWrapperTypeId = azrtti_typeid<GenericComponentWrapper>();

                    for (auto& [componentKey, component] : componentMap)
                    {
                        // If the component didn't serialize (i.e. is null) or the underlying type is GenericComponentWrapper,
                        // the template is null and the component should not be added.
                        if (component && component->GetUnderlyingComponentType() != genericComponentWrapperTypeId)
                        {
                            // When a component is first added into the disabledComponents list, it will already have
                            // a serialized identifier set, which is then used as the componentKey.
                            // When serializing the component back in, the identifier isn't serialized with the component itself,
                            // so we need to set it manually with the componentKey to restore the state back to what it was
                            // at the original point of serialization.
                            component->SetSerializedIdentifier(componentKey);

                            componentInstance->m_disabledComponents.emplace(componentKey, component);
                        }
                    }
                }

                result.Combine(disabledComponentsResult);
            }

            {
                JSR::ResultCode componentIdResult = ContinueLoadingFromJsonObjectField(
                    &componentInstance->m_id, azrtti_typeid<decltype(componentInstance->m_id)>(), inputValue, "Id", context);
                result.Combine(componentIdResult);
            }

            return context.Report(
                result,
                result.GetProcessing() != JSR::Processing::Halted ? "Successfully loaded EditorDisabledCompositionComponent information."
                                                                  : "Failed to load EditorDisabledCompositionComponent information.");
        }

        void EditorDisabledCompositionComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EditorDisabledCompositionComponent, EditorComponentBase>()
                    ->Version(1)
                    ->Field("DisabledComponents", &EditorDisabledCompositionComponent::m_disabledComponents)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<EditorDisabledCompositionComponent>("Disabled Components", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                            ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                            ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden)
                        ;
                }
            }

            if (auto jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context))
            {
                jsonRegistration->Serializer<EditorDisabledCompositionComponentSerializer>()->HandlesType<EditorDisabledCompositionComponent>();
            }
        }

        void EditorDisabledCompositionComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorDisabledCompositionService"));
        }

        void EditorDisabledCompositionComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("EditorDisabledCompositionService"));
        }

        void EditorDisabledCompositionComponent::GetDisabledComponents(AZ::Entity::ComponentArrayType& components)
        {
            for (auto const& pair : m_disabledComponents)
            {
                components.insert(components.end(), pair.second);
            }
        }

        void EditorDisabledCompositionComponent::AddDisabledComponent(AZ::Component* componentToAdd)
        {
            if (!componentToAdd)
            {
                AZ_Assert(false, "Unable to add a disabled component that is nullptr.");
                return;
            }

            AZStd::string componentAlias = componentToAdd->GetSerializedIdentifier();

            if (!componentAlias.empty() && m_disabledComponents.find(componentAlias) == m_disabledComponents.end())
            {
                m_disabledComponents.emplace(AZStd::move(componentAlias), componentToAdd);
            }
        }

        void EditorDisabledCompositionComponent::RemoveDisabledComponent(AZ::Component* componentToRemove)
        {
            if (!componentToRemove)
            {
                AZ_Assert(false, "Unable to remove a disabled component that is nullptr.");
                return;
            }

            AZStd::string componentAlias = componentToRemove->GetSerializedIdentifier();

            if (!componentAlias.empty())
            {
                m_disabledComponents.erase(componentAlias);
            }
        };

        bool EditorDisabledCompositionComponent::IsComponentDisabled(const AZ::Component* componentToCheck)
        {
            if (!componentToCheck)
            {
                AZ_Assert(false, "Unable to check a component that is nullptr.");
                return false;
            }

            AZStd::string componentAlias = componentToCheck->GetSerializedIdentifier();

            if (componentAlias.empty())
            {
                return false;
            }

            auto componentIt = m_disabledComponents.find(componentAlias);
            return componentIt != m_disabledComponents.end() && componentIt->second == componentToCheck;
        }

        EditorDisabledCompositionComponent::~EditorDisabledCompositionComponent()
        {
            for (auto& pair : m_disabledComponents)
            {
                delete pair.second;
            }
            m_disabledComponents.clear();

            // We disconnect from the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorDisabledCompositionRequestBus::Handler::BusDisconnect();
        }

        void EditorDisabledCompositionComponent::Init()
        {
            EditorComponentBase::Init();

            // We connect to the bus here because we need to be able to respond even if the entity and component are not active
            // This is a special case for certain EditorComponents only!
            EditorDisabledCompositionRequestBus::Handler::BusConnect(GetEntityId());

            // Set the entity for each disabled component.
            for (auto const& [componentAlias, disabledComponent] : m_disabledComponents)
            {
                // It's possible to get null components in the list if errors occur during serialization.
                // Guard against that case so that the code won't crash.
                if (disabledComponent)
                {
                    auto editorComponentBaseComponent = azrtti_cast<Components::EditorComponentBase*>(disabledComponent);
                    AZ_Assert(editorComponentBaseComponent, "Editor component does not derive from EditorComponentBase");

                    editorComponentBaseComponent->SetEntity(GetEntity());
                }
            }
        }

        void EditorDisabledCompositionComponent::Activate()
        {
            EditorComponentBase::Activate();
        }

        void EditorDisabledCompositionComponent::Deactivate()
        {
            EditorComponentBase::Deactivate();
        }
    } // namespace Components
} // namespace AzToolsFramework
