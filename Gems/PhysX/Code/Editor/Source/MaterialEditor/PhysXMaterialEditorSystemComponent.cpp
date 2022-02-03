
#include <AzCore/Serialization/SerializeContext.h>

#include <Editor/Source/MaterialEditor/PhysXMaterialEditorSystemComponent.h>

#include <Editor/Source/MaterialEditor/Window/MaterialEditorWindow.h>

namespace PhysX
{
    void PhysXMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PhysXMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    PhysXMaterialEditorSystemComponent::PhysXMaterialEditorSystemComponent()
        : m_notifyRegisterViewsEventHandler([this]() { RegisterAtomWindow(); })
    {
    }

    PhysXMaterialEditorSystemComponent::~PhysXMaterialEditorSystemComponent() = default;

    void PhysXMaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysXMaterialEditorService"));
    }

    void PhysXMaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXMaterialEditorService"));
    }

    void PhysXMaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void PhysXMaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void PhysXMaterialEditorSystemComponent::Activate()
    {
        if (auto* o3deMaterialEditor = O3DEMaterialEditor::O3DEMaterialEditorInterface::Get())
        {
            o3deMaterialEditor->ConnectNotifyRegisterViewsEventHandler(m_notifyRegisterViewsEventHandler);
        }
    }

    void PhysXMaterialEditorSystemComponent::Deactivate()
    {
        m_notifyRegisterViewsEventHandler.Disconnect();
    }

    void PhysXMaterialEditorSystemComponent::RegisterAtomWindow()
    {
        O3DEMaterialEditor::RegisterViewPane<MaterialEditorWindow>("PhysX Materials");
    }

} // namespace PhysX
