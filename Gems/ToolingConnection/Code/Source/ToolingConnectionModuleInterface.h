
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <ToolingConnectionSystemComponent.h>

namespace ToolingConnection
{
    class ToolingConnectionModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(ToolingConnectionModuleInterface, "{737ac146-f2c5-4f21-bb86-4bb665ca5f65}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ToolingConnectionModuleInterface, AZ::SystemAllocator, 0);

        ToolingConnectionModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ToolingConnectionSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ToolingConnectionSystemComponent>(),
            };
        }
    };
}// namespace ToolingConnection
