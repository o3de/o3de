/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mFileName = filename;
        mSelected = false;
        mMouseWithinWidget = false;
        setMinimumHeight(60);
        setMaximumHeight(60);
        setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // extract the pure filename
        AzFramework::StringFunc::Path::GetFileName(filename.c_str(), mFileNameWithoutExt);

        // load the pixmap
        mPixmap = new QPixmap(filename.c_str());
    }


    // destructor
    VisimeWidget::~VisimeWidget()
    {
        delete mPixmap;
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
        if (mSelected)
        {
            painter.setBrush(QBrush(QColor(244, 156, 28)));
        }
        else if (mMouseWithinWidget)
        {
            painter.setBrush(QBrush(QColor(153, 160, 178)));
        }
        else
        {
            painter.setBrush(QBrush(QColor(113, 120, 128)));
        }

        painter.drawRoundedRect(0, 2, width(), height() - 4, 5.0, 5.0);

        // draw background
        if (mSelected)
        {
            painter.setBrush(QBrush(QColor(56, 65, 72)));
        }
        else if (mMouseWithinWidget)
        {
            painter.setBrush(QBrush(QColor(134, 142, 150)));
        }
        else
        {
            painter.setBrush(QBrush(QColor(144, 152, 160)));
        }


        painter.drawRoundedRect(2, 4, width() - 4, height() - 8, 5.0, 5.0);

        // draw visime image
        painter.drawPixmap(5, 5, height() - 10, height() - 10, *mPixmap);

        // draw visime name
        if (mSelected)
        {
            painter.setPen(QColor(244, 156, 28));
        }
        else
        {
            painter.setPen(QColor(0, 0, 0));
        }

        //painter.setFont( QFont("MS Shell Dlg 2", 8) );
        painter.drawText(70, (height() / 2) + 4, mFileNameWithoutExt.c_str());
    }


    // constructor
    PhonemeSelectionWindow::PhonemeSelectionWindow(EMotionFX::Actor* actor, uint32 lodLevel, EMotionFX::MorphTarget* morphTarget, QWidget* parent)
        : QDialog(parent)
    {
        // set the initial size
        setMinimumWidth(800);
        setMinimumHeight(450);

        mActor          = actor;
        mMorphTarget    = morphTarget;
        mLODLevel       = lodLevel;
        mMorphSetup     = actor->GetMorphSetup(lodLevel);
        mDirtyFlag      = false;

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
        mAddPhonemesButton = new QPushButton("");
        mAddPhonemesButtonArrow = new QPushButton("");
        mRemovePhonemesButton = new QPushButton("");
        mRemovePhonemesButtonArrow = new QPushButton("");
        mClearPhonemesButton = new QPushButton("");

        EMStudioManager::MakeTransparentButton(mAddPhonemesButtonArrow,    "Images/Icons/PlayForward.svg",    "Assign the selected phonemes to the morph target.");
        EMStudioManager::MakeTransparentButton(mRemovePhonemesButtonArrow, "Images/Icons/PlayBackward.svg",   "Unassign the selected phonemes from the morph target.");
        EMStudioManager::MakeTransparentButton(mAddPhonemesButton,         "Images/Icons/Plus.svg",           "Assign the selected phonemes to the morph target.");
        EMStudioManager::MakeTransparentButton(mRemovePhonemesButton,      "Images/Icons/Minus.svg",          "Unassign the selected phonemes from the morph target.");
        EMStudioManager::MakeTransparentButton(mClearPhonemesButton,       "Images/Icons/Clear.svg",          "Unassign all phonemes from the morph target.");

        // init the visime tables
        mPossiblePhonemeSetsTable = new DragTableWidget(0, 1);
        mSelectedPhonemeSetsTable = new DragTableWidget(0, 1);
        mPossiblePhonemeSetsTable->setCornerButtonEnabled(false);
        mPossiblePhonemeSetsTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mPossiblePhonemeSetsTable->setContextMenuPolicy(Qt::DefaultContextMenu);
        mSelectedPhonemeSetsTable->setCornerButtonEnabled(false);
        mSelectedPhonemeSetsTable->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mSelectedPhonemeSetsTable->setContextMenuPolicy(Qt::DefaultContextMenu);

        // set the table to row single selection
        mPossiblePhonemeSetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        mSelectedPhonemeSetsTable->setSelectionBehavior(QAbstractItemView::SelectRows);

        // make the table items read only
        mPossiblePhonemeSetsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mSelectedPhonemeSetsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

        // resize to contents and adjust header
        QHeaderView* verticalHeaderPossible = mPossiblePhonemeSetsTable->verticalHeader();
        QHeaderView* verticalHeaderSelected = mSelectedPhonemeSetsTable->verticalHeader();
        QHeaderView* horizontalHeaderPossible = mPossiblePhonemeSetsTable->horizontalHeader();
        QHeaderView* horizontalHeaderSelected = mSelectedPhonemeSetsTable->horizontalHeader();
        verticalHeaderPossible->setVisible(false);
        verticalHeaderSelected->setVisible(false);
        horizontalHeaderPossible->setVisible(false);
        horizontalHeaderSelected->setVisible(false);

        // create the dialog stacks
        mPossiblePhonemeSets    = new MysticQt::DialogStack(this);
        mSelectedPhonemeSets    = new MysticQt::DialogStack(this);

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
        buttonLayout->addWidget(mAddPhonemesButton);

        leftLayout->addLayout(buttonLayout);
        leftLayout->addWidget(mPossiblePhonemeSetsTable);
        leftLayout->addWidget(labelHelperWidgetAdd);

        // the center layout
        QLabel* seperatorLineTop = new QLabel("");
        QLabel* seperatorLineBottom = new QLabel("");

        // fill the center layout
        centerLayout->addWidget(seperatorLineTop);
        centerLayout->addWidget(mAddPhonemesButtonArrow);
        centerLayout->addWidget(mRemovePhonemesButtonArrow);
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
        buttonLayout->addWidget(mRemovePhonemesButton);
        buttonLayout->addWidget(mClearPhonemesButton);

        // fill the right layozt
        rightLayout->addLayout(buttonLayout);
        rightLayout->addWidget(mSelectedPhonemeSetsTable);
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
        mPossiblePhonemeSets->Add(helperWidgetLeft, "Possible Phoneme Sets", false, true, false);
        mSelectedPhonemeSets->Add(helperWidgetRight, "Selected Phoneme Sets", false, true, false);

        // add sublayouts to the main layout
        layout->addWidget(mPossiblePhonemeSets);
        layout->addLayout(centerLayout);
        layout->addWidget(mSelectedPhonemeSets);

        // set the main layout
        setLayout(layout);

        // update the interface
        UpdateInterface();

        // connect signals to the slots
        connect(mPossiblePhonemeSetsTable, &DragTableWidget::itemSelectionChanged, this, &PhonemeSelectionWindow::PhonemeSelectionChanged);
        connect(mSelectedPhonemeSetsTable, &DragTableWidget::itemSelectionChanged, this, &PhonemeSelectionWindow::PhonemeSelectionChanged);
        connect(mPossiblePhonemeSetsTable, &DragTableWidget::dataDropped,          this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
        connect(mRemovePhonemesButton,     &QPushButton::clicked,              this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
        connect(mRemovePhonemesButtonArrow, &QPushButton::clicked,              this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
        connect(mAddPhonemesButton,        &QPushButton::clicked,              this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(mAddPhonemesButtonArrow,   &QPushButton::clicked,              this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(mSelectedPhonemeSetsTable, &DragTableWidget::dataDropped,          this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(mClearPhonemesButton,      &QPushButton::clicked,              this, &PhonemeSelectionWindow::ClearSelectedPhonemeSets);
        connect(mPossiblePhonemeSetsTable, &DragTableWidget::itemDoubleClicked,   this, &PhonemeSelectionWindow::AddSelectedPhonemeSets);
        connect(mSelectedPhonemeSetsTable, &DragTableWidget::itemDoubleClicked,   this, &PhonemeSelectionWindow::RemoveSelectedPhonemeSets);
    }


    void PhonemeSelectionWindow::UpdateInterface()
    {
        // return if morph setup is not valid
        if (mMorphSetup == nullptr)
        {
            return;
        }

        // clear the tables
        mPossiblePhonemeSetsTable->clear();
        mSelectedPhonemeSetsTable->clear();

        // get number of morph targets
        const uint32 numMorphTargets = mMorphSetup->GetNumMorphTargets();
        const uint32 numPhonemeSets  = mMorphTarget->GetNumAvailablePhonemeSets();
        uint32 insertPosition = 0;
        for (uint32 i = 1; i < numPhonemeSets; ++i)
        {
            // check if another morph target already has this phoneme set.
            bool phonemeSetFound = false;
            for (uint32 j = 0; j < numMorphTargets; ++j)
            {
                EMotionFX::MorphTarget* morphTarget = mMorphSetup->GetMorphTarget(j);
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
            const AZStd::string phonemeSetName = mMorphTarget->GetPhonemeSetString(phonemeSet).c_str();

            // set the row count for the possible phoneme sets table
            mPossiblePhonemeSetsTable->setRowCount(insertPosition + 1);

            // create dummy table widget item.
            QTableWidgetItem* item = new QTableWidgetItem(phonemeSetName.c_str());
            item->setToolTip(GetPhonemeSetExample(phonemeSet));
            mPossiblePhonemeSetsTable->setItem(insertPosition, 0, item);

            // create the visime widget and add it to the table
            const AZStd::string filename = AZStd::string::format("%s/Images/Visimes/%s.png", MysticQt::GetDataDir().c_str(), phonemeSetName.c_str());
            VisimeWidget* visimeWidget = new VisimeWidget(filename);
            mPossiblePhonemeSetsTable->setCellWidget(insertPosition, 0, visimeWidget);

            // set row and column properties
            mPossiblePhonemeSetsTable->setRowHeight(insertPosition, visimeWidget->height() + 2);

            // increase insert position
            ++insertPosition;
        }

        // fill the table with the selected phoneme sets
        const AZStd::string selectedPhonemeSets = mMorphTarget->GetPhonemeSetString(mMorphTarget->GetPhonemeSets());

        AZStd::vector<AZStd::string> splittedPhonemeSets;
        AzFramework::StringFunc::Tokenize(selectedPhonemeSets.c_str(), splittedPhonemeSets, MCore::CharacterConstants::comma, true /* keep empty strings */, true /* keep space strings */);


        const uint32 numSelectedPhonemeSets = static_cast<uint32>(splittedPhonemeSets.size());
        mSelectedPhonemeSetsTable->setRowCount(numSelectedPhonemeSets);
        for (uint32 i = 0; i < numSelectedPhonemeSets; ++i)
        {
            // create dummy table widget item.
            const EMotionFX::MorphTarget::EPhonemeSet phonemeSet = mMorphTarget->FindPhonemeSet(splittedPhonemeSets[i].c_str());
            QTableWidgetItem* item = new QTableWidgetItem(splittedPhonemeSets[i].c_str());
            item->setToolTip(GetPhonemeSetExample(phonemeSet));
            mSelectedPhonemeSetsTable->setItem(i, 0, item);

            // create the visime widget and add it to the table
            const AZStd::string filename = AZStd::string::format("%s/Images/Visimes/%s.png", MysticQt::GetDataDir().c_str(), splittedPhonemeSets[i].c_str());
            VisimeWidget* visimeWidget = new VisimeWidget(filename);
            mSelectedPhonemeSetsTable->setCellWidget(i, 0, visimeWidget);

            // set row and column properties
            mSelectedPhonemeSetsTable->setRowHeight(i, visimeWidget->height() + 2);
        }

        // stretch last section of the tables and disable horizontal header
        QHeaderView* horizontalHeaderSelected = mSelectedPhonemeSetsTable->horizontalHeader();
        horizontalHeaderSelected->setVisible(false);
        horizontalHeaderSelected->setStretchLastSection(true);

        QHeaderView* horizontalHeaderPossible = mPossiblePhonemeSetsTable->horizontalHeader();
        horizontalHeaderPossible->setVisible(false);
        horizontalHeaderPossible->setStretchLastSection(true);

        // disable/enable buttons upon reinit of the tables
        mAddPhonemesButton->setDisabled(true);
        mAddPhonemesButtonArrow->setDisabled(true);
        mRemovePhonemesButton->setDisabled(true);
        mRemovePhonemesButtonArrow->setDisabled(true);
        mClearPhonemesButton->setDisabled(mSelectedPhonemeSetsTable->rowCount() == 0);
    }


    // called when the selection of the possible phonemes table changed
    void PhonemeSelectionWindow::PhonemeSelectionChanged()
    {
        // get the table for which the selection changed
        QTableWidget* table = (QTableWidget*)sender();

        // disable/enable buttons
        bool selected = (table->selectedItems().size() > 0);
        if (table == mPossiblePhonemeSetsTable)
        {
            mAddPhonemesButton->setDisabled(!selected);
            mAddPhonemesButtonArrow->setDisabled(!selected);
        }
        else
        {
            mRemovePhonemesButton->setDisabled(!selected);
            mRemovePhonemesButtonArrow->setDisabled(!selected);
        }

        // adjust selection state of the cell widgetsmActor
        const uint32 numRows = table->rowCount();
        for (uint32 i = 0; i < numRows; ++i)
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
        QList<QTableWidgetItem*> selectedItems = mSelectedPhonemeSetsTable->selectedItems();
        const uint32 numSelectedItems = selectedItems.size();
        if (numSelectedItems == 0)
        {
            return;
        }

        // create phoneme sets string from the selected phoneme sets
        AZStd::string phonemeSets;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            phonemeSets += AZStd::string::format("%s,", selectedItems[i]->text().toUtf8().data());
        }

        // call command to remove selected the phoneme sets
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorID %i -lodLevel %i -name \"%s\" -phonemeAction \"remove\" -phonemeSets \"%s\"", mActor->GetID(), mLODLevel,  mMorphTarget->GetName(), phonemeSets.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            mDirtyFlag = true;
        }
    }


    // adds the selected phoneme sets
    void PhonemeSelectionWindow::AddSelectedPhonemeSets()
    {
        QList<QTableWidgetItem*> selectedItems = mSelectedPhonemeSetsTable->selectedItems();
        const uint32 numSelectedItems = selectedItems.size();
        if (numSelectedItems == 0)
        {
            return;
        }

        // create phoneme sets string from the selected phoneme sets
        AZStd::string phonemeSets;
        for (uint32 i = 0; i < numSelectedItems; ++i)
        {
            phonemeSets += AZStd::string::format("%s,", selectedItems[i]->text().toUtf8().data());
        }

        // call command to add the selected phoneme sets
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorID %i -lodLevel %i -name \"%s\" -phonemeAction \"add\" -phonemeSets \"%s\"", mActor->GetID(), mLODLevel, mMorphTarget->GetName(), phonemeSets.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            mDirtyFlag = true;
        }
    }


    // clear the selected phoneme sets
    void PhonemeSelectionWindow::ClearSelectedPhonemeSets()
    {
        const AZStd::string command = AZStd::string::format("AdjustMorphTarget -actorID %i -lodLevel %i -name \"%s\" -phonemeAction \"clear\"", mActor->GetID(), mLODLevel, mMorphTarget->GetName());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
        else
        {
            mDirtyFlag = true;
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
        if (mDirtyFlag == false)
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
