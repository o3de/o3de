
#include <MaterialEditorSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace MaterialEditor
{
    void MaterialEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaterialEditorSystemComponent>("MaterialEditor", "[Description of functionality provided by this System Component]")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MaterialEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("MaterialEditorService"));
    }

    void MaterialEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("MaterialEditorService"));
    }

    void MaterialEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void MaterialEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    MaterialEditorSystemComponent::MaterialEditorSystemComponent()
    {
        if (MaterialEditorInterface::Get() == nullptr)
        {
            MaterialEditorInterface::Register(this);
        }
    }

    MaterialEditorSystemComponent::~MaterialEditorSystemComponent()
    {
        if (MaterialEditorInterface::Get() == this)
        {
            MaterialEditorInterface::Unregister(this);
        }
    }

    void MaterialEditorSystemComponent::Init()
    {
    }

    void MaterialEditorSystemComponent::Activate()
    {
        MaterialEditorRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    void MaterialEditorSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        MaterialEditorRequestBus::Handler::BusDisconnect();
    }

    void MaterialEditorSystemComponent::OnTick([[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
    }

} // namespace MaterialEditor
