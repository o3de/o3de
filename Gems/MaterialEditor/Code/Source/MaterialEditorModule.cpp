
#include <MaterialEditorModuleInterface.h>
#include <MaterialEditorSystemComponent.h>

namespace MaterialEditor
{
    class MaterialEditorModule
        : public MaterialEditorModuleInterface
    {
    public:
        AZ_RTTI(MaterialEditorModule, "{5ac03883-3de1-43f1-a033-1a61c4239f1a}", MaterialEditorModuleInterface);
        AZ_CLASS_ALLOCATOR(MaterialEditorModule, AZ::SystemAllocator, 0);
    };
}// namespace MaterialEditor

AZ_DECLARE_MODULE_CLASS(Gem_MaterialEditor, MaterialEditor::MaterialEditorModule)
