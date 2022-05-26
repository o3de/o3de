
#include <ToolingConnectionModuleInterface.h>
#include <ToolingConnectionEditorSystemComponent.h>

namespace ToolingConnection
{
    class ToolingConnectionEditorModule
        : public ToolingConnectionModuleInterface
    {
    public:
        AZ_RTTI(ToolingConnectionEditorModule, "{86ed333f-1f40-497f-ac31-9de31dee9371}", ToolingConnectionModuleInterface);
        AZ_CLASS_ALLOCATOR(ToolingConnectionEditorModule, AZ::SystemAllocator, 0);

        ToolingConnectionEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                ToolingConnectionEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<ToolingConnectionEditorSystemComponent>(),
            };
        }
    };
}// namespace ToolingConnection

AZ_DECLARE_MODULE_CLASS(Gem_ToolingConnection, ToolingConnection::ToolingConnectionEditorModule)
