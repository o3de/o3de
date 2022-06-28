/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AdjustableHeaderWidget.h>
#include <DownloadController.h>
#include <GemCatalog/GemCatalogHeaderWidget.h>
#include <GemCatalog/CreateAGemScreen.h>
#include <GemCatalog/GemDependenciesDialog.h>
#include <GemCatalog/GemFilterWidget.h>
#include <GemCatalog/GemInspector.h>
#include <GemCatalog/GemItemDelegate.h>
#include <GemCatalog/GemListHeaderWidget.h>
#include <GemCatalog/GemListView.h>
#include <GemCatalog/GemModel.h>
#include <GemCatalog/GemRequirementDialog.h>
#include <GemCatalog/GemSortFilterProxyModel.h>
#include <GemCatalog/GemUninstallDialog.h>
#include <GemCatalog/GemUpdateDialog.h>
#include <ProjectUtils.h>
#include <PythonBindingsInterface.h>

#include <PythonBindingsInterface.h>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHash>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QRadioButton>


namespace O3DE::ProjectManager
{
    CreateAGemScreen::CreateAGemScreen(QWidget* parent)
        : ScreenWidget(parent)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout* hLayout = new QHBoxLayout();

        QVBoxLayout* leftPane = new QVBoxLayout();
        leftPaneHeader = new ScreenHeader(leftPane);
        leftPaneHeader->setTitle(tr("Please Choose a Gem Template"));
        leftPaneHeader->setSubTitle(tr("Gems can contain assets such as scripts, animations, meshes, textures, and more."));

        QTextEdit* firstPageString = new QTextEdit(leftPane);
        firstPageString->setPlainText("1. Gem Setup");
        QTextEdit* secondPageString = new QTextEdit(leftPane);
        secondPageString->setPlainText("2. Gem Details");
        QTextEdit* thirdPageString = new QTextEdit(leftPane);
        thirdPageString->setPlainText("3. Creator Details");


        m_stack = new QStackedWidget(this);
        m_stack->setObjectName("body");
        m_stack->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding));

        QRadioButton* button1 = new QRadioButton("Option", m_stack);
        QRadioButton* button2 = new QRadioButton("Option", m_stack);
        QRadioButton* button3 = new QRadioButton("Option", m_stack);
        QRadioButton* button4 = new QRadioButton("Option", m_stack);
        QRadioButton* button5 = new QRadioButton("Option", m_stack);


        hLayout->addWidget(leftPane);
        hLayout->addWidget(m_stack);

        QDialogButtonBox* buttons = new QDialogButtonBox();
        buttons->setObjectName("footer");
        vLayout->addWidget(buttons);

        m_backButton = buttons->addButton(tr("Back"), QDialogButtonBox::ApplyRole);
        m_nextButton = buttons->addButton(tr("Next"), QDialogButtonBox::ResetRole);
        m_backButton->setProperty("secondary", true);

        Update();

        setLayout(vLayout);

    }

} // namespace O3DE::ProjectManager

