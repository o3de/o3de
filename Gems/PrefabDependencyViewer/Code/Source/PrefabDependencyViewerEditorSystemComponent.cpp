
#include <AzCore/Serialization/SerializeContext.h>
#include <PrefabDependencyViewerEditorSystemComponent.h>

namespace PrefabDependencyViewer
{
    void PrefabDependencyViewerEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PrefabDependencyViewerEditorSystemComponent, AZ::Component>()
                ->Version(0);
        }
    }

    PrefabDependencyViewerEditorSystemComponent::PrefabDependencyViewerEditorSystemComponent() = default;

    void PrefabDependencyViewerEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PrefabDependencyViewerEditorService"));
    }

    void PrefabDependencyViewerEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PrefabDependencyViewerEditorService"));
    }

    void PrefabDependencyViewerEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void PrefabDependencyViewerEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void PrefabDependencyViewerEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        //AZ_Error("DependencyViewer", false, "The Prefab Dependency Viewer has booted up");
    }

    void PrefabDependencyViewerEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

} // namespace PrefabDependencyViewer
