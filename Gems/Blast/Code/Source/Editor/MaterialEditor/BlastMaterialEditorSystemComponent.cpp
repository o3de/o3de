
#include <AzCore/Serialization/SerializeContext.h>

#include <Editor/MaterialEditor/BlastMaterialEditorSystemComponent.h>

#include <Editor/MaterialEditor/Window/MaterialEditorWindow.h>

namespace Blast
{
    void BlastMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BlastMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    BlastMaterialEditorSystemComponent::BlastMaterialEditorSystemComponent()
        : m_notifyRegisterViewsEventHandler([this]() { RegisterBlastWindow(); })
    {
    }

    BlastMaterialEditorSystemComponent::~BlastMaterialEditorSystemComponent() = default;

    void BlastMaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("BlastMaterialEditorService"));
    }

    void BlastMaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("BlastMaterialEditorService"));
    }

    void BlastMaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void BlastMaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void BlastMaterialEditorSystemComponent::Activate()
    {
        if (auto* o3deMaterialEditor = O3DEMaterialEditor::O3DEMaterialEditorInterface::Get())
        {
            o3deMaterialEditor->ConnectNotifyRegisterViewsEventHandler(m_notifyRegisterViewsEventHandler);
        }
    }

    void BlastMaterialEditorSystemComponent::Deactivate()
    {
        m_notifyRegisterViewsEventHandler.Disconnect();
    }

    void BlastMaterialEditorSystemComponent::RegisterBlastWindow()
    {
        O3DEMaterialEditor::RegisterViewPane<MaterialEditorWindow>("Blast Materials");
    }

} // namespace Blast
