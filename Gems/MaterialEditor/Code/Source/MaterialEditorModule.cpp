
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <MaterialEditorSystemComponent.h>

void InitMaterialEditorResources()
{
    // We must register our Qt resources (.qrc file) since this is being loaded from a separate module (gem)
    Q_INIT_RESOURCE(MaterialEditor);
}

namespace MaterialEditor
{
    class MaterialEditorModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(MaterialEditorModule, "{5ac03883-3de1-43f1-a033-1a61c4239f1a}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MaterialEditorModule, AZ::SystemAllocator, 0);

        MaterialEditorModule()
        {
            InitMaterialEditorResources();

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
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<MaterialEditorSystemComponent>(),
            };
        }
    };
}// namespace MaterialEditor

AZ_DECLARE_MODULE_CLASS(Gem_MaterialEditor, MaterialEditor::MaterialEditorModule)
