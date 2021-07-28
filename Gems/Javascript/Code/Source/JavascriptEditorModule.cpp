
#include <JavascriptModuleInterface.h>
#include <JavascriptEditorSystemComponent.h>
#include <JavascriptComponent.h>

namespace Javascript
{
    class JavascriptEditorModule
        : public JavascriptModuleInterface
    {
    public:
        AZ_RTTI(JavascriptEditorModule, "{996d5f8b-c41d-4644-a0b7-8d439c5fbd3a}", JavascriptModuleInterface);
        AZ_CLASS_ALLOCATOR(JavascriptEditorModule, AZ::SystemAllocator, 0);

        JavascriptEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                REngine::JavascriptComponent::CreateDescriptor()
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<REngine::JavascriptComponent>(),
            };
        }
    };
}// namespace Javascript

AZ_DECLARE_MODULE_CLASS(Gem_Javascript, Javascript::JavascriptEditorModule)
