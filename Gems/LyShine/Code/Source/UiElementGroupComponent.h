
#pragma once

#include <LyShine/Bus/UiElementGroupBus.h>

#include <AzCore/Component/Component.h>
#include <LyShine/Bus/UiInitializationBus.h>

class UiElementGroupComponent
    : public AZ::Component
    , public UiInitializationBus::Handler
    , public UiElementGroupBus::Handler
{
    public:
        AZ_COMPONENT_DECL(UiElementGroupComponent);
        
        
        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        
        //Ui Initialization 
        void InGamePostActivate() override;
        
        //Interactive
        bool SetInteractivity(bool enabled) override;
        bool SetParentInteractivity(bool parentEnabled) override;
        void UpdateInteractiveState();
        void PropagateInteractivityToChildren(const AZ::EntityId& parentId, bool parentState);
        
        bool GetInteractiveState() override { return m_isInteractionLocallyEnabled && m_isInteractionParentEnabled; };
        
        //Rendering
        bool SetRendering(bool enabled) override;
        
        bool GetRenderingState() override;
    
    protected:
        void Activate() override;
        void Deactivate() override;
        bool m_isInteractionLocallyEnabled = true;
        bool m_isInteractionParentEnabled = true;
        bool m_isRenderingLocallyEnabled = true;
};
