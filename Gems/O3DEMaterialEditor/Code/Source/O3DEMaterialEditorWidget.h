
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/Widgets/TabWidget.h>
#endif

namespace O3DEMaterialEditor
{
    class O3DEMaterialEditorWidget
        : public AzQtComponents::TabWidget
    {
        Q_OBJECT
    public:
        explicit O3DEMaterialEditorWidget(QWidget* parent = nullptr);
    };
} 
