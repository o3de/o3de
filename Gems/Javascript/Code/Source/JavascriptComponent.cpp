#include "JavascriptComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace REngine {
    JavascriptComponent::JavascriptComponent() :
        m_context(0) {
    }
    JavascriptComponent::~JavascriptComponent()
    {
        if (m_context)
            duk_destroy_heap(m_context);
    }
    void JavascriptComponent::Init()
    {
        m_context = duk_create_heap_default();

        duk_push_c_function(m_context, JavascriptComponent::PrintLog, 1);
        duk_put_global_string(m_context, "log");
    }
    void JavascriptComponent::Activate()
    {
    }
    void JavascriptComponent::Deactivate()
    {
    }
    void JavascriptComponent::SetScript(const char* script)
    {
        AZ_Assert(m_context != nullptr, "Javascript Component is not already initialized!");
        m_script = script;

        duk_eval_string(m_context, script);
    }
    void JavascriptComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context)) {
            serialize->Class<JavascriptComponent, AZ::Component>()->Version(0);

            if (AZ::EditContext* ec = serialize->GetEditContext()) {
                ec->Class<JavascriptComponent>("Javascript", "Run Javascript Code")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Scripting"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }
    void JavascriptComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
    }
    void JavascriptComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
    void JavascriptComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }
    void JavascriptComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
    }
    duk_ret_t JavascriptComponent::PrintLog(duk_context* ctx)
    {
        AZ::Debug::Trace::Output("JavaScript", duk_to_string(ctx, 0));
        return duk_ret_t();
    }
}
