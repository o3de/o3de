
#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <MotionMatching/MotionMatchingBus.h>

namespace MotionMatching
{
    class MotionMatchingSystemComponent
        : public AZ::Component
        , protected MotionMatchingRequestBus::Handler
        , public AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(MotionMatchingSystemComponent, "{158cd35c-b548-4d7b-9493-9a3c5c359e49}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        MotionMatchingSystemComponent();
        ~MotionMatchingSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // MotionMatchingRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZTickBus interface implementation
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        ////////////////////////////////////////////////////////////////////////
    };

} // namespace MotionMatching
