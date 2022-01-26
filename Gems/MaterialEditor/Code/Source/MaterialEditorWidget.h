
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QWidget>
#endif

namespace MaterialEditor
{
    class MaterialEditorWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit MaterialEditorWidget(QWidget* parent = nullptr);
    };
} 
