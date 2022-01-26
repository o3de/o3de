
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <MaterialEditorSystemComponent.h>

namespace MaterialEditor
{
    class MaterialEditorModuleInterface
        : public AZ::Module
    {
    public:
        AZ_RTTI(MaterialEditorModuleInterface, "{1eec97cc-3173-403b-aabe-612cdcf8cd4b}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MaterialEditorModuleInterface, AZ::SystemAllocator, 0);

        MaterialEditorModuleInterface()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                MaterialEditorSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MaterialEditorSystemComponent>(),
            };
        }
    };
}// namespace MaterialEditor
