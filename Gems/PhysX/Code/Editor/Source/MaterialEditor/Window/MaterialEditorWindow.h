
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <QWidget>
#endif

namespace PhysX
{
    class MaterialEditorWindow
        : public QWidget
    {
        Q_OBJECT
    public:
        explicit MaterialEditorWindow(QWidget* parent = nullptr);
    };
}
