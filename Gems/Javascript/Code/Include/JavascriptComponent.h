#ifndef R_JAVASCRIPT_COMPONENT_H
#define R_JAVASCRIPT_COMPONENT_H

#include <AzCore/Component/Component.h>

namespace REngine {
    class JavascriptComponent : public AZ::Component {
    public:
        AZ_COMPONENT(JavascriptComponent, "{EE09F2F7-A016-48A1-841C-3384CD0E5A5F}", AZ::Component);

        JavascriptComponent();

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
    };
}

#endif
