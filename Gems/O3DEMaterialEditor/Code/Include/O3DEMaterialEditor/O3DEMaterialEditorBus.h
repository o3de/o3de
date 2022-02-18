
#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string.h>

class QWidget;

namespace O3DEMaterialEditor
{
    using WidgetCreationFunc = AZStd::function<QWidget* (QWidget*)>;

    class O3DEMaterialEditorRequests
    {
    public:
        AZ_RTTI(O3DEMaterialEditorRequests, "{68bb1a2f-33b1-4906-adf3-c74c460400b1}");
        virtual ~O3DEMaterialEditorRequests() = default;

        //! Event signaled when it's ok to register views.
        using NotifyRegisterViewsEvent = AZ::Event<>;

        //! Connects a handler to the NotifyRegisterViewsEvent.
        void ConnectNotifyRegisterViewsEventHandler(NotifyRegisterViewsEvent::Handler& handler)
        {
            handler.Connect(m_notifyRegisterViewsEvent);
        }

        //! Registers a view pane with the main editor.
        virtual void RegisterViewPane(const AZStd::string& name, const AZStd::string& icon, const WidgetCreationFunc& widgetCreationFunc) = 0;

    protected:
        NotifyRegisterViewsEvent m_notifyRegisterViewsEvent;
    };
    
    using O3DEMaterialEditorInterface = AZ::Interface<O3DEMaterialEditorRequests>;

    //! Registers a view pane with the main editor.
    template <typename TWidget>
    inline void RegisterViewPane(const AZStd::string& name, const AZStd::string& icon)
    {
        auto* o3deMaterialEditor = O3DEMaterialEditorInterface::Get();
        if (o3deMaterialEditor)
        {
            WidgetCreationFunc windowCreationFunc =
                [](QWidget* parent = nullptr)
                {
                    return new TWidget(parent);
                };

            o3deMaterialEditor->RegisterViewPane(name, icon, AZStd::move(windowCreationFunc));
        }
    }

} // namespace O3DEMaterialEditor
