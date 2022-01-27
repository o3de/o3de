

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <AtomMaterialEditorSystemComponent.h>

namespace AtomMaterialEditor
{
    class AtomMaterialEditorModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(AtomMaterialEditorModule, "{5AF716C6-114F-41A0-A9BD-0A8E6A3763C4}", AZ::Module);
        AZ_CLASS_ALLOCATOR(AtomMaterialEditorModule, AZ::SystemAllocator, 0);

        AtomMaterialEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                AtomMaterialEditorSystemComponent::CreateDescriptor(),
                });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AtomMaterialEditorSystemComponent>(),
            };
        }
    };
}// namespace MaterialEditor

AZ_DECLARE_MODULE_CLASS(Gem_AtomMaterialEditor, AtomMaterialEditor::AtomMaterialEditorModule)
