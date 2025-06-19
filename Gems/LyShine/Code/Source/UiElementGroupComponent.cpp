
#include <UiElementGroupComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiInteractableBus.h>

AZ_COMPONENT_IMPL(UiElementGroupComponent, "UiElementGroupComponent", "{B8C5A864-1A98-48B9-BEBB-1FDE06E6D463}");

/*===========*/
/*== Start ==*/
/*===========*/

void UiElementGroupComponent::Activate()
{
    UiElementGroupBus::Handler::BusConnect(GetEntityId());
    UiInitializationBus::Handler::BusConnect(GetEntityId());
}

void UiElementGroupComponent::Deactivate()
{
    UiElementGroupBus::Handler::BusDisconnect();
}

void UiElementGroupComponent::InGamePostActivate()
{
    UiInitializationBus::Handler::BusDisconnect();
    
    if(!m_isInteractionLocallyEnabled) { SetInteractivity(false); }
    if(!m_isRenderingLocallyEnabled) { SetRendering(false); }
}

/*============================*/
/*== Interactivity Handling ==*/
/*============================*/

bool UiElementGroupComponent::SetInteractivity(bool enabled)
{
    m_isInteractionLocallyEnabled = enabled;
    UpdateInteractiveState();
    return true;
}

bool UiElementGroupComponent::SetParentInteractivity(bool parentEnabled)
{
    m_isInteractionParentEnabled = parentEnabled;
    UpdateInteractiveState();
    return true;
}

void UiElementGroupComponent::UpdateInteractiveState()
{
    const bool effectiveState = GetInteractiveState();
    
    //Affect the current entity.
    UiInteractableBus::Event(GetEntityId(), &UiInteractableInterface::SetIsHandlingEvents, effectiveState);
    UiInteractableBus::Event(GetEntityId(), &UiInteractableInterface::SetIsHandlingMultiTouchEvents, effectiveState);
    
    PropagateInteractivityToChildren(GetEntityId(), effectiveState);
}

void UiElementGroupComponent::PropagateInteractivityToChildren(const AZ::EntityId& parentId, bool parentState)
{
    AZStd::vector<AZ::EntityId> children;
    UiElementBus::EventResult(children, parentId, &UiElementInterface::GetChildEntityIds);
    
    for (const AZ::EntityId& child : children)
    {
        //If has ElementGroup, will return true;
        bool hasGroup = false;
        UiElementGroupBus::EventResult(hasGroup, child, &UiElementGroupInterface::SetParentInteractivity, parentState);
        
        //Because no group found, affect child and recurse.
        if (!hasGroup)
        {
            // Affect interactable state directly
            UiInteractableBus::Event(child, &UiInteractableInterface::SetIsHandlingEvents, parentState);
            UiInteractableBus::Event(child, &UiInteractableInterface::SetIsHandlingMultiTouchEvents, parentState);
            
            // Recurse into this child's children
            PropagateInteractivityToChildren(child, parentState);
        }
    }
}

/*========================*/
/*== Rendering Handling ==*/
/*========================*/

bool UiElementGroupComponent::SetRendering(bool enabled)
{
    UiElementBus::Event(GetEntityId(), &UiElementInterface::SetIsRenderEnabled, enabled);
    m_isRenderingLocallyEnabled = enabled;
    return true;
}

/*=============*/
/*== Utility ==*/
/*=============*/

bool UiElementGroupComponent::GetRenderingState()
{
    UiElementBus::EventResult(m_isRenderingLocallyEnabled, GetEntityId(), &UiElementInterface::IsRenderEnabled);
    
    return m_isRenderingLocallyEnabled; 
}

/*============*/
/*== System ==*/
/*============*/

void UiElementGroupComponent::Reflect(AZ::ReflectContext* context)
{
    if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    {
        serializeContext->Class<UiElementGroupComponent, AZ::Component>()
        ->Version(1)
        ->Field("LocalInteraction", &UiElementGroupComponent::m_isInteractionLocallyEnabled)
        ->Field("LocalRendering", &UiElementGroupComponent::m_isRenderingLocallyEnabled)
        ;
        
        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
        {
            editContext->Class<UiElementGroupComponent>("ElementGroup", "A grouping handler that allows interaction and rendering for the entire hierarchy of children.")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
            ->Attribute(AZ::Edit::Attributes::Category, "UI")
            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("UI"))
            ->DataElement(AZ::Edit::UIHandlers::Default, &UiElementGroupComponent::m_isInteractionLocallyEnabled, "Local Interaction", "Whether this group and children will be interactable.")
            ->DataElement(AZ::Edit::UIHandlers::Default, &UiElementGroupComponent::m_isRenderingLocallyEnabled, "Local Rendering", "Whether this group and children will be rendered.")
            ;
        }
    }
    
    if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
    {
        behaviorContext->EBus<UiElementGroupBus>("UiElementGroupBus")
        ->Event("SetInteractivity", &UiElementGroupInterface::SetInteractivity)
        ->Event("SetRendering", &UiElementGroupInterface::SetRendering)
        ;
    }
}

void UiElementGroupComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
{
    provided.push_back(AZ_CRC_CE("UiElementGroupComponentService"));
}

void UiElementGroupComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
{
    incompatible.push_back(AZ_CRC_CE("UiElementGroupComponentService"));
}

void UiElementGroupComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
{
}

void UiElementGroupComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
{
}
