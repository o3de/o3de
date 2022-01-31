
#include <AzCore/Utils/Utils.h>

#include <QLabel>
#include <QVBoxLayout>

#include <O3DEMaterialEditorWidget.h>
#include <O3DEMaterialEditorSystemComponent.h>

namespace O3DEMaterialEditor
{
    namespace
    {
        QWidget* CreateTab(const AZStd::string& name)
        {
            QWidget* tab = new QWidget();
            {
                QVBoxLayout* mainLayout = new QVBoxLayout(tab);

                AZStd::string text = AZStd::string::format("Put your cool stuff here %s!", name.c_str());
                QLabel* introLabel = new QLabel(text.c_str(), tab);
                mainLayout->addWidget(introLabel, 0, Qt::AlignCenter);

                QString helpText = QString(
                    "For help getting started, visit the <a href=\"https://o3de.org/docs/tools-ui/ui-dev-intro/\">UI Development</a> documentation<br/>or come ask a question in the <a href=\"https://discord.gg/R77Wss3kHe\">sig-ui-ux channel</a> on Discord");

                QLabel* helpLabel = new QLabel(tab);
                helpLabel->setTextFormat(Qt::RichText);
                helpLabel->setText(helpText);
                helpLabel->setOpenExternalLinks(true);

                mainLayout->addWidget(helpLabel, 0, Qt::AlignCenter);

                tab->setLayout(mainLayout);
            }
            return tab;
        }

        QWidget* CreateTabFromWidgetFunc(const WidgetCreationFunc& widgetCreationFunc)
        {
            QWidget* tab = new QWidget();
            {
                QVBoxLayout* mainLayout = new QVBoxLayout(tab);

                QWidget* content = widgetCreationFunc(tab);
                mainLayout->addWidget(content, 1);

                tab->setLayout(mainLayout);
            }
            return tab;
        }
    }

    O3DEMaterialEditorWidget::O3DEMaterialEditorWidget(QWidget* parent)
        : AzQtComponents::TabWidget(parent)
    {
        setMovable(true);

        if (auto* o3deMaterialEditorSystem = GetO3DEMaterialEditorSystem())
        {
            const auto& registeredTabs = o3deMaterialEditorSystem->GetRegisteredTabs();
            for (const auto& registeredTab : registeredTabs)
            {
                //addTab(registeredTab.second(nullptr), registeredTab.first.c_str());
                //addTab(CreateTabFromWidgetFunc(registeredTab.second), registeredTab.first.c_str());
                addTab(CreateTab(registeredTab.first), registeredTab.first.c_str());
            }
        }

        // if no systems registered views, add a notification tab
        if (count() == 0)
        {
            addTab(CreateTab("NO MATERIAL EDITORS"), "No Material Editors");
        }
    }
}

#include <moc_O3DEMaterialEditorWidget.cpp>
