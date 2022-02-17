
#include <AzCore/Utils/Utils.h>

#include <QLabel>
#include <QVBoxLayout>

#include <Editor/Source/MaterialEditor/Window/MaterialEditorWindow.h>

namespace PhysX
{
    MaterialEditorWindow::MaterialEditorWindow(QWidget* parent)
        : QWidget(parent)
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        QLabel* introLabel = new QLabel("Put your cool stuff here PHYSX!", this);
        mainLayout->addWidget(introLabel, 0, Qt::AlignCenter);

        QString helpText = QString(
            "For help getting started, visit the <a href=\"https://o3de.org/docs/tools-ui/ui-dev-intro/\">UI Development</a> documentation<br/>or come ask a question in the <a href=\"https://discord.gg/R77Wss3kHe\">sig-ui-ux channel</a> on Discord");

        QLabel* helpLabel = new QLabel(this);
        helpLabel->setTextFormat(Qt::RichText);
        helpLabel->setText(helpText);
        helpLabel->setOpenExternalLinks(true);

        mainLayout->addWidget(helpLabel, 0, Qt::AlignCenter);

        setLayout(mainLayout);
    }
}

#include <Editor/Source/MaterialEditor/Window/moc_MaterialEditorWindow.cpp>
