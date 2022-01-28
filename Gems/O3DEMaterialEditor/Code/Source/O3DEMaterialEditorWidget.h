
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include <O3DEMaterialEditor/O3DEMaterialEditorBus.h>
#endif

namespace O3DEMaterialEditor
{
    class O3DEMaterialEditorWidget
        : public AzQtComponents::TabWidget
        , public O3DEMaterialEditorInterface::Registrar
    {
        Q_OBJECT
    public:
        explicit O3DEMaterialEditorWidget(QWidget* parent = nullptr);

        // O3DEMaterialEditorInterface overrides ...
        void RegisterViewPane(const AZStd::string& name, const WidgetCreationFunc& widgetCreationFunc) override;
    };
} 
