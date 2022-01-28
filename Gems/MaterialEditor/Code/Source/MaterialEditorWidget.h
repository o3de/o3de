
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <AzQtComponents/Components/Widgets/TabWidget.h>
#endif

namespace MaterialEditor
{
    class MaterialEditorWidget
        : public AzQtComponents::TabWidget
    {
        Q_OBJECT
    public:
        explicit MaterialEditorWidget(QWidget* parent = nullptr);
    };
} 
