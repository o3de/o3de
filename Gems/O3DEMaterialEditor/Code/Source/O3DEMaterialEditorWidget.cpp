
#include <AzCore/Utils/Utils.h>

#include <QLabel>
#include <QVBoxLayout>

#include <AzQtComponents/Components/StyledDockWidget.h>

#include <O3DEMaterialEditorWidget.h>

namespace O3DEMaterialEditor
{
    QDockWidget* CreateDockTab(const AZStd::string& name)
    {
        AzQtComponents::StyledDockWidget* tab = new AzQtComponents::StyledDockWidget(name.c_str());
        {
            QDockWidget::DockWidgetFeatures features = QDockWidget::NoDockWidgetFeatures;
            //features |= QDockWidget::DockWidgetClosable;
            features |= QDockWidget::DockWidgetFloatable;
            features |= QDockWidget::DockWidgetMovable;
            //features |= QDockWidget::DockWidgetVerticalTitleBar;
            tab->setFeatures(features);

            QWidget* mainWidget = new QWidget(tab);

            QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);

            AZStd::string text = AZStd::string::format("Put your cool stuff here %s!", name.c_str());
            QLabel* introLabel = new QLabel(text.c_str(), mainWidget);
            mainLayout->addWidget(introLabel, 0, Qt::AlignCenter);

            QString helpText = QString(
                "For help getting started, visit the <a href=\"https://o3de.org/docs/tools-ui/ui-dev-intro/\">UI Development</a> documentation<br/>or come ask a question in the <a href=\"https://discord.gg/R77Wss3kHe\">sig-ui-ux channel</a> on Discord");

            QLabel* helpLabel = new QLabel(mainWidget);
            helpLabel->setTextFormat(Qt::RichText);
            helpLabel->setText(helpText);
            helpLabel->setOpenExternalLinks(true);

            mainLayout->addWidget(helpLabel, 0, Qt::AlignCenter);

            tab->setWidget(mainWidget);
        }
        return tab;
    }

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

    O3DEMaterialEditorWidget::O3DEMaterialEditorWidget(QWidget* parent)
        : AzQtComponents::TabWidget(parent)
    {
        setMovable(true);

        m_notifyRegisterViewsEvent.Signal();
    }

    void O3DEMaterialEditorWidget::RegisterViewPane(
        [[maybe_unused]] const AZStd::string& name,
        [[maybe_unused]] const WidgetCreationFunc& widgetCreationFunc)
    {

    }
}

#include <moc_O3DEMaterialEditorWidget.cpp>
