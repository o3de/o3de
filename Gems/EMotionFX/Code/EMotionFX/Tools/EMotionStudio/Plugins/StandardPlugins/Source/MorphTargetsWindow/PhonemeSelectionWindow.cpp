/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhonemeSelectionWindow.h"
#include "MorphTargetsWindowPlugin.h"
#include <EMotionFX/Source/MorphSetup.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <MCore/Source/StringConversions.h>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QListWidget>
#include <QFileDialog>
#include <QDropEvent>
#include <QUrl>
#include <QProgressBar>
#include <QScrollBar>
#include <QPushButton>
#include <QDir>
#include <QPainter>
#include <QLinearGradient>
#include <QHeaderView>


namespace EMStudio
{
    // constructor
    VisimeWidget::VisimeWidget(const AZStd::string& filename)
    {
        // set the file name and size hints
        m_fileName = filename;
        m_selected = false;
        m_mouseWithinWidget = false;
        setMinimumHeight(60);
        setMaximumHeight(60);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // extract the pure filename
        AzFramework::StringFunc::Path::GetFileName(filename.c_str(), m_fileNameWithoutExt);

        // load the pixmap
        m_pixmap = new QPixmap(filename.c_str());
    }


    // destructor
    VisimeWidget::~VisimeWidget()
    {
        delete m_pixmap;
    }


    // function to update the interface
    void VisimeWidget::UpdateInterface()
    {
    }


    // draw the visime widget
    void VisimeWidget::paintEvent(QPaintEvent* event)
    {
        MCORE_UNUSED(event);

        // create the painter
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // linear gradient for the background
        QLinearGradient gradient (0, 0, 0, height());
        gradient.setColorAt(0.0, QColor(66, 66, 66));
        gradient.setColorAt(0.5, QColor(94, 102, 110));
        gradient.setColorAt(1.0, QColor(66, 66, 66));
        painter.setPen(QColor(94, 102, 110));

        // set colors
        painter.setPen(QColor(66, 66, 66));

        // draw selection border
        if (m_selected)
        {
            painter.setBrush(QBrush(QColor(244, 156, 28)));
        }
        else if (m_mouseWithinWidget)
        {
            painter.setBrush(QBrush(QColor(153, 160, 178)));
        }
        else
        {
            painter.setBrush(QBrush(QColor(113, 120, 128)));
        }

        painter.drawRoundedRect(0, 2, width(), height() - 4, 5.0, 5.0);

        // draw background
        if (m_selected)
        {
            painter.setBrush(QBrush(QColor(56, 65, 72)));
        }
        else if (m_mouseWithinWidget)
        {
            painter.setBrush(QBrush(QColor(134, 142, 150)));
        }
        else
        {
            painter.setBrush(QBrush(QColor(144, 152, 160)));
        }


        painter.drawRoundedRect(2, 4, width() - 4, height() - 8, 5.0, 5.0);

        // draw visime image
        painter.drawPixmap(5, 5, height() - 10, height() - 10, *m_pixmap);

        // draw visime name
        if (m_selected)
        {
            painter.setPen(QColor(244, 156, 28));
        }
        else
        {
            painter.setPen(QColor(0, 0, 0));
        }

        //painter.setFont( QFont("MS Shell Dlg 2", 8) );
        painter.drawText(70, (height() / 2) + 4, m_fileNameWithoutExt.c_str());
    }


    // constructor
    PhonemeSelectionWindow::PhonemeSelectionWindow(EMotionFX::Actor* actor, size_t lodLevel, EMotionFX::MorphTarget* morphTarget, QWidget* parent)
        : QDialog(parent)
    {
        // set the initial size
        setMinimumWidth(800);
        setMinimumHeight(450);

        m_actor          = actor;
        m_morphTarget    = morphTarget;
        m_lodLevel       = lodLevel;
        m_morphSetup     = actor->GetMorphSetup(lodLevel);
        m_dirtyFlag      = false;

        // init the dialog
        Init();
    }


    // destructor
    PhonemeSelectionWindow::~PhonemeSelectionWindow()
    {
    }


    // initialize the preferences window
    void PhonemeSelectionWindow::Init()
    {
        // set window title
        setWindowTitle("Phoneme Selection Window");
        setSizeGripEnabled(false);

        // buttons to add / remove / clear phonemes
        m_addPhonemesButton = new QPushButton("");
        m_addPhonemesButtonArrow = new QPushButton("");
        m_removePhonemesButton = new QPushButton("");
        m_removePhonemesButtonArrow = new QPushButton("");
        m_clearPhonemesButton = new QPushButton("");

        EMStudioManager::MakeTransparentButton(m_addPhonemesButtonArrow,    "Images/Icons/PlayForward.svg",    "Assign the selected phonemes to the morph target.");
        EMStudioManager::MakeTransparentButton(m_removePhonemesButtonArrow, "Images/Icons/PlayBackward.svg",   "Unassign the selected phonemes from the morph target.");
        EMStudioManager::MakeTransparentButton(m_addPhonemesButton,         "Images/Icons/Plus.svg",           "Assign the selected phonemes to the morph target.");
        EMStudioManager::MakeTransparentButton(m_removePhonemesButton,      "Images/Icons/Minus.svg",          "Unassign the selected phonemes from the morph target.");
        EMStudioManager::MakeTransparentButton(m_clearPhonemesButton,       "Images/Icons/Clear.svg",          "Unassign all phonemes from the morph target.");

        // init the visime tables
        m_possiblePhonemeSetsTable = new DragTableWidget(0, 1);
        m_selectedPhonemeSetsTable = new DragTableWidget(0, 1);
        m_possiblePhonemeSetsTable->setCornerButtonEnabled(false);
        m_possiblePhonemeSetsTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_possiblePhonemeSetsTable->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_selectedPhonemeSetsTable->setCornerButtonEnabled(false);
        m_selectedPhonemeSetsTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        m_selectedPhonemeSetsTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        m_possiblePhonemeSetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_selectedPhonemeSetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        // make the table items read only
        m_possiblePhonemeSetsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_selectedPhonemeSetsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // resize to contents and adjust header
        QHeaderView* verticalHeaderPossible = m_possiblePhonemeSetsTable->verticalHeader();
        QHeaderView* verticalHeaderSelected = m_selectedPhonemeSetsTable->verticalHeader();
        QHeaderView* horizontalHeaderPossible = m_possiblePhonemeSetsTable->horizontalHeader();
        QHeaderView* horizontalHeaderSelected = m_selectedPhonemeSetsTable->horizontalHeader();
        verticalHeaderPossible->setVisible(false);
        verticalHeaderSelected->setVisible(false);
        horizontalHeaderPossible->setVisible(false);
        horizontalHeaderSelected->setVisible(false);

        // create the dialog stacks
        m_possiblePhonemeSets    = new MysticQt::DialogStack(this);
        m_selectedPhonemeSets    = new MysticQt::DialogStack(this);

        // create and fill the main layout
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setMargin(0);
        layout->setSpacing(0);

        QVBoxLayout* leftLayout     = new QVBoxLayout();
        QVBoxLayout* centerLayout   = new QVBoxLayout();
        QVBoxLayout* rightLayout    = new QVBoxLayout();

        // the left layout and info label
        QWidget* labelHelperWidgetAdd = new QWidget();
        QVBoxLayout* labelHelperWidgetAddLayout = new QVBoxLayout();
        labelHelperWidgetAddLayout->setSpacing(0);
        labelHelperWidgetAddLayout->setMargin(2);
        labelHelperWidgetAdd->setLayout(labelHelperWidgetAddLayout);
        QLabel* labelAdd = new QLabel("- Use drag&drop or double click to add -");
        labelHelperWidgetAddLayout->addWidget(labelAdd);
        labelHelperWidgetAddLayout->setAlignment(labelAdd, Qt::AlignCenter);

        // create the buttons layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(m_addPhonemesButton);

        leftLayout->addLayout(buttonLayout);
        leftLayout->addWidget(m_possiblePhonemeSetsTable);
        leftLayout->addWidget(labelHelperWidgetAdd);

        // the center layout
        QLabel* seperatorLineTop = new QLabel("");
        QLabel* seperatorLineBottom = new QLabel("");

        // fill the center layout
        centerLayout->addWidget(seperatorLineTop);
        centerLayout->addWidget(m_addPhonemesButtonArrow);
        centerLayout->addWidget(m_removePhonemesButtonArrow);
        centerLayout->addWidget(seperatorLineBottom);

        // the right layout and info label
        QWidget* labelHelperWidgetRemove = new QWidget();
        QVBoxLayout* labelHelperWidgetRemoveLayout = new QVBoxLayout();
        labelHelperWidgetRemoveLayout->setSpacing(0);
        labelHelperWidgetRemoveLayout->setMargin(2);
        labelHelperWidgetRemove->setLayout(labelHelperWidgetRemoveLayout);
        QLabel* labelRemove = new QLabel("- Use drag&drop or double click to remove -");
        labelHelperWidgetRemoveLayout->addWidget(labelRemove);
        labelHelperWidgetRemoveLayout->setAlignment(labelRemove, Qt::AlignCenter);

        // layout for clear/remove buttons
        buttonLayout = new QHBoxLayout();
        buttonLayout->setSpacing(0);
        buttonLayout->setAlignment(Qt::AlignLeft);
        buttonLayout->addWidget(m_removePhonemesButton);
        buttonLayout->addWidget(m_clearPhonemesButton);

        // fill the right layozt
        rightLayout->addLayout(buttonLayout);
        rightLayout->addWidget(m_selectedPhonemeSetsTable);
        rightLayout->addWidget(labelHelperWidgetRemove);


        // helper widgets for the dialog stacks
        QWidget* helperWidgetLeft = new QWidget();
        QWidget* helperWidgetRight = new QWidget();
        helperWidgetLeft->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        helperWidgetRight->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        leftLayout->setSpacing(0);
        leftLayout->setMargin(0);
        rightLayout->setSpacing(0);
        rightLayout->setMargin(0);
        helperWidgetLeft->setLayout(leftLayout);
        helperWidgetRight->setLayout(rightLayout);

        // add helper widgets to the dialog stacks
        m_possiblePhonemeSets->Add(helperWidgetLeft, "Possible Phoneme Sets", false, true, false);
        m_selectedPhonemeSets->Add(helperWidgetRight, "Selected Phoneme Sets", false, true, false);

        // add sublayouts to the main layout
        layout->addWidget(m_possiblePhonemeSets);
        layout->addLayout(centerLayout);
        layout->addWidget(m_selectedPhonemeSets);

        // set the main layout
        setLayout(layout);

        // update the interface
        UpdateInterface();

        // connect signals to the slots
        connect(m_possiblePhonemeSetsTable, &DragTableWidget::itemSelectionChanged, this, &PhonemeSelectionWindow::PhonemeSelectionChanged);
        connect(m_selectedPhonemeSetsTable, &DragTableWidget::itemSelectionChanged, this, &PhonemeSelectionWindow::PhonemeSelectionChanged);
        connect(m_possiblePhonemeSetsTable, &DragTableWidget::dataDropped,          this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
        connect(m_removePhonemesButton,     &QPushButton::clicked,              this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
        connect(m_removePhonemesButtonArrow, &QPushButton::clicked,              this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
        connect(m_addPhonemesButton,        &QPushButton::clicked,              this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(m_addPhonemesButtonArrow,   &QPushButton::clicked,              this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(m_selectedPhonemeSetsTable, &DragTableWidget::dataDropped,          this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(m_clearPhonemesButton,      &QPushButton::clicked,              this, &PhonemeSelectionWindow::ClearSelectedPhonemeSets);
        connect(m_possiblePhonemeSetsTable, &DragTableWidget::itemDoubleClicked,   this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(m_selectedPhonemeSetsTable, &DragTableWidget::itemDoubleClicked,   this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
    }


    void PhonemeSelectionWindow::UpdateInterface()
    {
        // return if morph setup is not valid
        if (m_morphSetup == nullptr)
        {
            return;
        }

        // clear the tables
        m_possiblePhonemeSetsTable->clear();
        m_selectedPhonemeSetsTable->clear();

        // get number of morph targets
        const size_t numMorphTargets = m_morphSetup->GetNumMorphTargets();
        const uint32 numPhonemeSets  = m_morphTarget->GetNumAvailablePhonemeSets();
        int insertPosition = 0;
        for (uint32 i = 1; i < numPhonemeSets; ++i)
        {
            // check if another morph target already has this phoneme set.
            bool phonemeSetFound = false;
            for (size_t j = 0; j < numMorphTargets; ++j)
            {
                EMotionFX::MorphTarget* morphTarget = m_morphSetup->GetMorphTarget(j);
                if (morphTarget->GetIsPhonemeSetEnabled((EMotionFX::MorphTarget::EPhonemeSet)(1 << i)))
                {
                    phonemeSetFound = true;
                    break;
                }
            }

            // skip this phoneme set, because it is already used by another mt
            if (phonemeSetFound)
            {
                continue;
            }

            // get the phoneme set name
            EMotionFX::MorphTarget::EPhonemeSet phonemeSet = (EMotionFX::MorphTarget::EPhonemeSet)(1 << i);
            const AZStd::string phonemeSetName = m_morphTarget->GetPhonemeSetString(phonemeSet).c_str();

            // set the row count for the possible phoneme sets table
            m_possiblePhonemeSetsTable->setRowCount(insertPosition + 1);

            // create dummy table widget item.
            QTableWidgetItem* item = new QTableWidgetItem(phonemeSetName.c_str());
            item->setToolTip(GetPhonemeSetExample(phonemeSet));
            m_possiblePhonemeSetsTable->setItem(insertPosition, 0, item);

            // create the visime widget and add it to the table
            const AZStd::string filename = AZStd::string::format("%s/Images/Visimes/%s.png", MysticQt::GetDataDir().c_str(), phonemeSetName.c_str());
            VisimeWidget* visimeWidget = new VisimeWidget(filename);
            m_possiblePhonemeSetsTable->setCellWidget(insertPosition, 0, visimeWidget);

            // set row and column properties
            m_possiblePhonemeSetsTable->setRowHeight(insertPosition, visimeWidget->height() + 2);

            // increase insert position
            ++insertPosition;
        }

        // fill the table with the selected phoneme sets
        const AZStd::string selectedPhonemeSets = m_morphTarget->GetPhonemeSetString(m_morphTarget->GetPhonemeSets());

        AZStd::vector<AZStd::string> splittedPhonemeSets;
        AzFramework::StringFunc::Tokenize(selectedPhonemeSets.c_str(), splittedPhonemeSets, MCore::CharacterConstants::comma, true /* keep empty strings */, true /* keep space strings */);


        const int numSelectedPhonemeSets = aznumeric_caster(splittedPhonemeSets.size());
        m_selectedPhonemeSetsTable->setRowCount(numSelectedPhonemeSets);
        for (int i = 0; i < numSelectedPhonemeSets; ++i)
        {
            // create dummy table widget item.
            const EMotionFX::MorphTarget::EPhonemeSet phonemeSet = m_morphTarget->FindPhonemeSet(splittedPhonemeSets[i].c_str());
            QTableWidgetItem* item = new QTableWidgetItem(splittedPhonemeSets[i].c_str());
            item->setToolTip(GetPhonemeSetExample(phonemeSet));
            m_selectedPhonemeSetsTable->setItem(i, 0, item);

            // create the visime widget and add it to the table
            const AZStd::string filename = AZStd::string::format("%s/Images/Visimes/%s.png", MysticQt::GetDataDir().c_str(), splittedPhonemeSets[i].c_str());
            VisimeWidget* visimeWidget = new VisimeWidget(filename);
            m_selectedPhonemeSetsTable->setCellWidget(i, 0, visimeWidget);

            // set row and column properties
            m_selectedPhonemeSetsTable->setRowHeight(i, visimeWidget->height() + 2);
        }

        // stretch last section of the tables and disable horizontal header
        QHeaderView* horizontalHeaderSelected = m_selectedPhonemeSetsTable->horizontalHeader();
        horizontalHeaderSelected->setVisible(false);
        horizontalHeaderSelected->setStretchLastSection(true);

        QHeaderView* horizontalHeaderPossible = m_possiblePhonemeSetsTable->horizontalHeader();
        horizontalHeaderPossible->setVisible(false);
        horizontalHeaderPossible->setStretchLastSection(true);

        // disable/enable buttons upon reinit of the tables
        m_addPhonemesButton->setDisabled(true);
        m_addPhonemesButtonArrow->setDisabled(true);
        m_removePhonemesButton->setDisabled(true);
        m_removePhonemesButtonArrow->setDisabled(true);
        m_clearPhonemesButton->setDisabled(m_selectedPhonemeSetsTable->rowCount() == 0);
    }


    // called when the selection of the possible phonemes table changed
    void PhonemeSelectionWindow::PhonemeSelectionChanged()
    {
        // get the table for which the selection changed
        QTableWidget* table = (QTableWidget*)sender();

        // disable/enable buttons
        bool selected = !table->selectedItems().empty();
        if (table == m_possiblePhonemeSetsTable)
        {
            m_addPhonemesButton->setDisabled(!selected);
            m_addPhonemesButtonArrow->setDisabled(!selected);
        }
        else
        {
            m_removePhonemesButton->setDisabled(!selected);
            m_removePhonemesButtonArrow->setDisabled(!selected);
        }

        // adjust selection state of the cell widgetsmActor
        const int numRows = table->rowCount();
        for (int i = 0; i < numRows; ++i)
        {
            // get the table widget item and check if it exists
            QTableWidgetItem* item  = table->item(i, 0);
            if (item == nullptr)
            {
                continue;
            }

            // get the visime widget and set selection parameter
            VisimeWidget* widget = (VisimeWidget*) table->cellWidget(i, 0);
            if (widget)
            {
                widget->SetSelected((item->isSelected()));
            }
        }
    }


    // removes the selected phoneme sets
    void PhonemeSelectionWindow::RemoveSelectedPhonemeSets()
    {
        QList<QTableWidgetItem*> selectedItems = m_selectedPhonemeSetsTable->selectedItems();
        if (selectedItems.empty())
        {
            return;
        }

        // create phoneme sets string from the selected phoneme sets
        AZStd::string phonemeSets;
        for (const QTableWidgetItem* selectedItem : selectedItems)
        {
            phonemeSets += AZStd::string::format("%s,", selectedItem->text().toUtf8().data());
        }

        // call command to remove selected the phoneme sets
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorID %i -lodLevel %zu -name \"%s\" -phonemeAction \"remove\" -phonemeSets \"%s\"", m_actor->GetID(), m_lodLevel,  m_morphTarget->GetName(), phonemeSets.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            m_dirtyFlag = true;
        }
    }


    // adds the selected phoneme sets
    void PhonemeSelectionWindow::AddSelectedPhonemeSets()
    {
        QList<QTableWidgetItem*> selectedItems = m_selectedPhonemeSetsTable->selectedItems();
        if (selectedItems.empty())
        {
            return;
        }

        // create phoneme sets string from the selected phoneme sets
        AZStd::string phonemeSets;
        for (const QTableWidgetItem* selectedItem : selectedItems)
        {
            phonemeSets += AZStd::string::format("%s,", selectedItem->text().toUtf8().data());
        }

        // call command to add the selected phoneme sets
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorID %i -lodLevel %zu -name \"%s\" -phonemeAction \"add\" -phonemeSets \"%s\"", m_actor->GetID(), m_lodLevel, m_morphTarget->GetName(), phonemeSets.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            m_dirtyFlag = true;
        }
    }


    // clear the selected phoneme sets
    void PhonemeSelectionWindow::ClearSelectedPhonemeSets()
    {
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorID %i -lodLevel %zu -name \"%s\" -phonemeAction \"clear\"", m_actor->GetID(), m_lodLevel, m_morphTarget->GetName());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            m_dirtyFlag = true;
        }
    }


    // function to get an example for a given phoneme set
    const char* PhonemeSelectionWindow::GetPhonemeSetExample(EMotionFX::MorphTarget::EPhonemeSet phonemeSet)
    {
        switch (phonemeSet)
        {
        case EMotionFX::MorphTarget::PHONEMESET_NEUTRAL_POSE:
            return "";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_M_B_P_X:
            return "mat, pat";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_AA_AO_OW:
            return "ought, part, Oh!";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_IH_AE_AH_EY_AY_H:
            return "it, at, hut, ate, hide";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_AW:
            return "cow";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_N_NG_CH_J_DH_D_G_T_K_Z_ZH_TH_S_SH:
            return "";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_IY_EH_Y:
            return "eat, ate, young";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_UW_UH_OY:
            return "two, hood";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_F_V:
            return "fresh, vulture";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_L_EL:
            return "lala, along";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_W:
            return "global, quick";
            break;
        case EMotionFX::MorphTarget::PHONEMESET_R_ER:
            return "rear, butter";
            break;
        default:
            return "Unknown Phoneme Set!";
        }
    }


    // reinit the plugin upon closing this dialog
    void PhonemeSelectionWindow::closeEvent(QCloseEvent* event)
    {
        MCORE_UNUSED(event);

        // check if something changed
        if (m_dirtyFlag == false)
        {
            return;
        }

        // reinit plugin if phoneme sets have changed for this morph target
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MorphTargetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return;
        }

        MorphTargetsWindowPlugin* morphTargetsWindow = (MorphTargetsWindowPlugin*)plugin;
        morphTargetsWindow->ReInit(true);
    }


    // key press event
    void PhonemeSelectionWindow::keyPressEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            RemoveSelectedPhonemeSets();
            event->accept();
            return;
        }

        // base class
        QWidget::keyPressEvent(event);
    }


    // key release event
    void PhonemeSelectionWindow::keyReleaseEvent(QKeyEvent* event)
    {
        // delete key
        if (event->key() == Qt::Key_Delete)
        {
            event->accept();
            return;
        }

        // base class
        QWidget::keyReleaseEvent(event);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/moc_PhonemeSelectionWindow.cpp>
