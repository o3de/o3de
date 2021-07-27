
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <JavascriptSystemComponent.h>

namespace Javascript
{
    class JavascriptModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(JavascriptModuleInterface, "{4b5ebd3b-6fbd-470a-befe-a4449879f620}", AZ::Module);
        AZ_CLASS_ALLOCATOR(JavascriptModuleInterface, AZ::SystemAllocator, 0);

        JavascriptModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                JavascriptSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<JavascriptSystemComponent>(),
            };
        }
    };
}// namespace Javascript
