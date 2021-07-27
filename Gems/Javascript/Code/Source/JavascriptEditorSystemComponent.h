
#pragma once

#include <JavascriptSystemComponent.h>

#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace Javascript
{
    /// System component for Javascript editor
    class JavascriptEditorSystemComponent
        : public JavascriptSystemComponent
        , private AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = JavascriptSystemComponent;
    public:
        AZ_COMPONENT(JavascriptEditorSystemComponent, "{5eb40ea6-a42f-4292-ad45-800f96690883}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        JavascriptEditorSystemComponent();
        ~JavascriptEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace Javascript
