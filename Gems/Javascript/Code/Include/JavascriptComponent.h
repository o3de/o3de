#ifndef R_JAVASCRIPT_COMPONENT_H
#define R_JAVASCRIPT_COMPONENT_H

#include <AzCore/Component/Component.h>
#include <duktape.h>

namespace REngine {
    class JavascriptComponent : public AZ::Component {
    public:
        AZ_COMPONENT(JavascriptComponent, "{EE09F2F7-A016-48A1-841C-3384CD0E5A5F}", AZ::Component);

        JavascriptComponent();
        ~JavascriptComponent();

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        duk_context* GetContext() { return m_context; }

        void SetScript(const char* script);
        const char* GetScript() { return m_script; }

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        
    private:
        static duk_ret_t PrintLog(duk_context* ctx);

        duk_context* m_context;
        const char* m_script;
    };
}

#endif
