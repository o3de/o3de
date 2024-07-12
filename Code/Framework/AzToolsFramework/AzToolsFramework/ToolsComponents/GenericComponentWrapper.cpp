/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/ComponentExport.h>
#include <AzCore/Slice/SliceComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Components/EditorEntityEvents.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>


namespace AzToolsFramework
{
    namespace Components
    {
        /**
         * Custom export callback for GenericComponentWrapper, invoked by the slice compiler.
         * The Wrapper component simply exports the inner template, which is the runtime/non-editor component.
         */
        AZ::ExportedComponent ExportTemplateComponent(AZ::Component* thisComponent, const AZ::PlatformTagSet& /*platformTags*/)
        {
            GenericComponentWrapper* wrapper = static_cast<GenericComponentWrapper*>(thisComponent);
            return AZ::ExportedComponent(wrapper->GetTemplate(), false);
        }

        ////////////////////////////////////////////////////////////////////////
        // GenericComponentWrapper
        ////////////////////////////////////////////////////////////////////////

        void GenericComponentWrapper::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<GenericComponentWrapper, EditorComponentBase>()
                    ->Field("m_template", &GenericComponentWrapper::m_template);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<GenericComponentWrapper>("GenericComponentWrapper", "This should be hidden!")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &GenericComponentWrapper::GetDisplayName)
                            ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &GenericComponentWrapper::GetDisplayDescription)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::RuntimeExportCallback, &ExportTemplateComponent)
                        ->DataElement("", &GenericComponentWrapper::m_template, "m_template", "")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC_CE("PropertyVisibility_ShowChildrenOnly"));
                }
            }
        }

        GenericComponentWrapper::GenericComponentWrapper()
        {
        }

        GenericComponentWrapper::GenericComponentWrapper(const AZ::SerializeContext::ClassData* templateClassData)
        {
            AZ::ComponentDescriptorBus::EventResult(
                m_template, templateClassData->m_typeId, &AZ::ComponentDescriptorBus::Events::CreateComponent);
        }

        GenericComponentWrapper::GenericComponentWrapper(AZ::Component* templateClassInstance)
        {
            if (templateClassInstance)
            {
                if (templateClassInstance->GetEntity())
                {
                    AZ_Error("GenericComponentWrapper", false, "Component must be detached from entity before placing inside GenericComponentWrapper.");
                }
                else
                {
                    m_id = templateClassInstance->GetId();
                    m_template = templateClassInstance;
                }
            }
        }

        GenericComponentWrapper::~GenericComponentWrapper()
        {
            if (m_template)
            {
                delete m_template;
            }
        }

        GenericComponentWrapper::GenericComponentWrapper(const GenericComponentWrapper& RHS)
            : m_displayName(RHS.m_displayName)
            , m_displayDescription(RHS.m_displayDescription)
        {
            if (GetEntity())
            {
                AZ_Assert(GetEntity()->GetState() <= AZ::Entity::State::Init, "Entity should not be activated when copying components");
            }

            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            m_template = context->CloneObject<AZ::Component>(RHS.m_template);

            m_templateEvents = azrtti_cast<AzFramework::EditorEntityEvents*>(m_template);
        }

        GenericComponentWrapper::GenericComponentWrapper(GenericComponentWrapper&& RHS)
            : m_displayName(AZStd::move(RHS.m_displayName))
            , m_displayDescription(AZStd::move(RHS.m_displayDescription))
        {
            if (GetEntity())
            {
                AZ_Assert(GetEntity()->GetState() <= AZ::Entity::State::Init, "Entity should not be activated when copying components");
            }

            m_template = AZStd::move(RHS.m_template);
            RHS.m_template = nullptr;

            m_templateEvents = azrtti_cast<AzFramework::EditorEntityEvents*>(m_template);
        }

        GenericComponentWrapper& GenericComponentWrapper::operator=(const GenericComponentWrapper& RHS)
        {
            if (GetEntity())
            {
                AZ_Assert(GetEntity()->GetState() <= AZ::Entity::State::Init, "Entity should not be activated when copying components");
            }

            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            m_template = context->CloneObject<AZ::Component>(RHS.m_template);

            m_templateEvents = azrtti_cast<AzFramework::EditorEntityEvents*>(m_template);

            m_displayName = RHS.m_displayName;
            m_displayDescription = RHS.m_displayDescription;

            return *this;
        }

        GenericComponentWrapper& GenericComponentWrapper::operator=(GenericComponentWrapper&& RHS)
        {
            if (GetEntity())
            {
                AZ_Assert(GetEntity()->GetState() <= AZ::Entity::State::Init, "Entity should not be activated when copying components");
            }

            m_template = AZStd::move(RHS.m_template);
            RHS.m_template = nullptr;

            m_templateEvents = azrtti_cast<AzFramework::EditorEntityEvents*>(m_template);

            m_displayName = AZStd::move(RHS.m_displayName);
            m_displayDescription = AZStd::move(RHS.m_displayDescription);

            return *this;
        }

        const char* GenericComponentWrapper::GetDisplayName()
        {
            if (m_displayName.empty())
            {
                if (m_template)
                {
                    m_displayName = GetFriendlyComponentName(m_template);
                }
            }
            return m_displayName.c_str();
        }

        const char* GenericComponentWrapper::GetDisplayDescription()
        {
            if (m_displayDescription.empty())
            {
                if (m_template)
                {
                    m_displayDescription = GetFriendlyComponentDescription(m_template);
                }
            }
            return m_displayDescription.c_str();
        }

        void GenericComponentWrapper::Init()
        {
            EditorComponentBase::Init();

            if (m_template)
            {
                m_displayName = GetFriendlyComponentName(m_template);
                m_displayDescription = GetFriendlyComponentDescription(m_template);
                if (m_displayDescription.empty())
                {
                    m_displayDescription = m_displayName;
                }

                m_templateEvents = azrtti_cast<AzFramework::EditorEntityEvents*>(m_template);
                if (m_templateEvents)
                {
                    m_templateEvents->EditorInit(GetEntityId());
                }
            }
        }

        void GenericComponentWrapper::Activate()
        {
            EditorComponentBase::Activate();

            if (m_templateEvents)
            {
                m_templateEvents->EditorActivate(GetEntityId());
                AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
            }
        }

        void GenericComponentWrapper::Deactivate()
        {
            EditorComponentBase::Deactivate();

            if (m_templateEvents)
            {
                AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
                m_templateEvents->EditorDeactivate(GetEntityId());
            }
        }

        AZ::TypeId GenericComponentWrapper::GetUnderlyingComponentType() const
        {
            if (m_template)
            {
                return m_template->RTTI_GetType();
            }

            return RTTI_GetType();
        }

        void GenericComponentWrapper::BuildGameEntity(AZ::Entity* gameEntity)
        {
            if (m_template)
            {
                AZ::SerializeContext* context = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (!context)
                {
                    AZ_Error("GenericComponentWrapper", false, "Can't get serialize context from component application.");
                    return;
                }

                gameEntity->AddComponent(context->CloneObject(m_template));
            }
        }

        AZ::ComponentValidationResult GenericComponentWrapper::ValidateComponentRequirements(
            const AZ::ImmutableEntityVector& sliceEntities, const AZStd::unordered_set<AZ::Crc32>& platformTags) const
        {
            AZ::ComponentValidationResult baseClassResult = EditorComponentBase::ValidateComponentRequirements(sliceEntities, platformTags);
            if (!baseClassResult.IsSuccess())
            {
                return baseClassResult;
            }

            if (m_template)
            {
                return m_template->ValidateComponentRequirements(sliceEntities, platformTags);
            }

            return AZ::Success();
        }

        void GenericComponentWrapper::DisplayEntityViewport(
            const AzFramework::ViewportInfo& /*viewportInfo*/,
            AzFramework::DebugDisplayRequests& debugDisplay)
        {
            if (m_templateEvents)
            {
                m_templateEvents->EditorDisplay(GetEntityId(), debugDisplay, GetWorldTM());
            }
        }

        void GenericComponentWrapper::SetPrimaryAsset(const AZ::Data::AssetId& assetId)
        {
            if (m_templateEvents)
            {
                m_templateEvents->EditorSetPrimaryAsset(assetId);
            }
        }

        AZ::Component* GenericComponentWrapper::ReleaseTemplate()
        {
            AZ::Component* component = m_template;
            m_template = nullptr;
            return component;
        }

        class GenericComponentWrapperDescriptor
            : public AZ::ComponentDescriptorHelper<GenericComponentWrapper>
        {
        public:
            AZ_CLASS_ALLOCATOR(GenericComponentWrapperDescriptor, AZ::SystemAllocator);
            AZ_TYPE_INFO(GenericComponentWrapperDescriptor, "{3326B218-282B-4985-AEDE-39A77D48AD57}");

            AZ::ComponentDescriptor* GetTemplateDescriptor(const AZ::Component* instance) const
            {
                AZ::ComponentDescriptor* templateDescriptor = nullptr;

                const GenericComponentWrapper* wrapper = azrtti_cast<const GenericComponentWrapper*>(instance);
                if (wrapper && wrapper->GetTemplate())
                {
                    AZ::ComponentDescriptorBus::EventResult(
                        templateDescriptor, wrapper->GetTemplate()->RTTI_GetType(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                }

                return templateDescriptor;
            }

            void Reflect(AZ::ReflectContext* reflection) const override
            {
                GenericComponentWrapper::Reflect(reflection);
            }

            void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetProvidedServices(provided, instance);
                }
            }

            void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetDependentServices(dependent, instance);
                }
            }

            void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetRequiredServices(required, instance);
                }
            }

            void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible, const AZ::Component* instance) const override
            {
                const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
                if (templateDescriptor)
                {
                    templateDescriptor->GetIncompatibleServices(incompatible, instance);
                }
            }
        };

        AZ::ComponentDescriptor* GenericComponentWrapper::CreateDescriptor()
        {
            AZ::ComponentDescriptor* descriptor = nullptr;
            AZ::ComponentDescriptorBus::EventResult(
                descriptor, GenericComponentWrapper::RTTI_Type(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

            return descriptor ? descriptor : aznew GenericComponentWrapperDescriptor();
        }
    }   // namespace Components

    AZ::TypeId GetUnderlyingComponentType(const AZ::Component& component)
    {
        return component.GetUnderlyingComponentType();
    }
} // namespace AzToolsFramework
