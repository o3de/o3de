#pragma once

#include <AzCore/Component/Component.h>
#include <${SanitizedCppName}Interface.h>

namespace ${SanitizedCppName}
{
    class ${SanitizedCppName}Component
        : public AZ::Component
        , public ${SanitizedCppName}RequestBus::Handler
    {
    public:
        AZ_COMPONENT(${SanitizedCppName}Component, "{${Random_Uuid}}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
} // namespace ${SanitizedCppName}
