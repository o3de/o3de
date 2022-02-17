
#include <AzCore/Utils/Utils.h>

#include <QLabel>
#include <QVBoxLayout>

#include <O3DEMaterialEditorWidget.h>
#include <O3DEMaterialEditorSystemComponent.h>

namespace O3DEMaterialEditor
{
    O3DEMaterialEditorWidget::O3DEMaterialEditorWidget(QWidget* parent)
        : AzQtComponents::TabWidget(parent)
    {
        AzQtComponents::TabWidget::applySecondaryStyle(this, false/*bordered*/);

        setMovable(true);

        if (auto* o3deMaterialEditorSystem = GetO3DEMaterialEditorSystem())
        {
            const auto& registeredTabs = o3deMaterialEditorSystem->GetRegisteredTabs();
            for (const auto& registeredTab : registeredTabs)
            {
                const AZStd::string& tabName = registeredTab.first;
                const WidgetCreationFunc& widgetCreationFunc = registeredTab.second;

                // Have render material tab first
                if (tabName == "Render Materials")
                {
                    insertTab(0, widgetCreationFunc(nullptr), tabName.c_str());
                    setCurrentIndex(0);
                }
                else
                {
                    addTab(widgetCreationFunc(nullptr), tabName.c_str());
                }
            }
        }

        // if no systems registered views, add a tab to let the user know.
        if (count() == 0)
        {
            QWidget* noWindowsTab = new QWidget();
            {
                QVBoxLayout* mainLayout = new QVBoxLayout(noWindowsTab);

                QLabel* introLabel = new QLabel("No material editors registered. Enable gems that provide material editors.", noWindowsTab);
                mainLayout->addWidget(introLabel, 0/*stretch*/, Qt::AlignCenter);

                noWindowsTab->setLayout(mainLayout);
            }
            addTab(noWindowsTab, "No Materials");
        }
    }
}

#include <moc_O3DEMaterialEditorWidget.cpp>
