
#include <AzCore/Serialization/SerializeContext.h>

#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>

#include <AtomMaterialEditorSystemComponent.h>

#include <Window/MaterialEditorBrowserInteractions.h>
#include <Window/MaterialEditorWindow.h>
#include <Document/MaterialDocument.h>

namespace AtomMaterialEditor
{
    void AtomMaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AtomMaterialEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    AtomMaterialEditorSystemComponent::AtomMaterialEditorSystemComponent()
        : m_notifyRegisterViewsEventHandler([this]() { RegisterAtomWindow(); })
    {
    }

    AtomMaterialEditorSystemComponent::~AtomMaterialEditorSystemComponent() = default;

    void AtomMaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomMaterialEditorService"));
    }

    void AtomMaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomMaterialEditorService"));
    }

    void AtomMaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AtomToolsDocumentSystemService"));
        required.push_back(AZ_CRC_CE("O3DEMaterialEditorService"));
    }

    void AtomMaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomMaterialEditorSystemComponent::Activate()
    {
        if (auto* o3deMaterialEditor = O3DEMaterialEditor::O3DEMaterialEditorInterface::Get())
        {
            o3deMaterialEditor->ConnectNotifyRegisterViewsEventHandler(m_notifyRegisterViewsEventHandler);
        }
    }

    void AtomMaterialEditorSystemComponent::Deactivate()
    {
        m_notifyRegisterViewsEventHandler.Disconnect();
    }

    void AtomMaterialEditorSystemComponent::RegisterAtomWindow()
    {
        AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Handler::RegisterDocumentType,
            []() { return aznew MaterialEditor::MaterialDocument(); });

        m_materialEditorBrowserInteractions.reset(aznew MaterialEditor::MaterialEditorBrowserInteractions);

        O3DEMaterialEditor::RegisterViewPane<MaterialEditor::MaterialEditorWindow>("Render Materials");
    }

} // namespace MaterialEditor
