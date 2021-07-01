/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MirrorSetupWindow.h"
#include <AzCore/Casting/numeric_cast.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QListWidget>
#include <QTableWidget>
#include <QLabel>
#include <QSplitter>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/NodeMap.h>
#include <EMotionFX/Source/Pose.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <MysticQt/Source/MysticQtManager.h>
#include "SceneManagerPlugin.h"
#include <EMotionStudio/EMStudioSDK/Source/NotificationWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>


namespace EMStudio
{
    // constructor
    MirrorSetupWindow::MirrorSetupWindow(QWidget* parent, SceneManagerPlugin* plugin)
        : QDialog(parent)
    {
        mPlugin = plugin;

        // set the window title
        setWindowTitle("Mirror Setup");

        // set the minimum size
        setMinimumWidth(800);
        setMinimumHeight(600);

        // load some icons
        const QDir dataDir{ QString(MysticQt::GetDataDir().c_str()) };
        mBoneIcon   = new QIcon(dataDir.filePath("Images/Icons/Bone.svg"));
        mNodeIcon   = new QIcon(dataDir.filePath("Images/Icons/Node.svg"));
        mMeshIcon   = new QIcon(dataDir.filePath("Images/Icons/Mesh.svg"));
        mMappedIcon = new QIcon(dataDir.filePath("Images/Icons/Confirm.svg"));

        // create the main layout
        QVBoxLayout* mainLayout = new QVBoxLayout();
        mainLayout->setMargin(3);
        mainLayout->setSpacing(1);
        setLayout(mainLayout);

        // create the layout that contains the two listboxes next to each other, and in the middle the link button
        QHBoxLayout* topPartLayout = new QHBoxLayout();
        topPartLayout->setMargin(0);

        QHBoxLayout* toolBarLayout = new QHBoxLayout();
        toolBarLayout->setMargin(0);
        toolBarLayout->setSpacing(0);
        mainLayout->addLayout(toolBarLayout);
        mButtonOpen = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonOpen,    "Images/Icons/Open.svg",       "Load and apply a mapping template.");
        connect(mButtonOpen, &QPushButton::clicked, this, &MirrorSetupWindow::OnLoadMapping);
        mButtonSave = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonSave,    "Images/Menu/FileSave.svg",    "Save the currently setup mapping as template.");
        connect(mButtonSave, &QPushButton::clicked, this, &MirrorSetupWindow::OnSaveMapping);
        mButtonClear = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonClear,   "Images/Icons/Clear.svg",      "Clear the currently setup mapping entirely.");
        connect(mButtonClear, &QPushButton::clicked, this, &MirrorSetupWindow::OnClearMapping);

        mButtonGuess = new QPushButton();
        EMStudioManager::MakeTransparentButton(mButtonGuess,   "Images/Icons/Character.svg",  "Perform name based mapping.");
        connect(mButtonGuess, &QPushButton::clicked, this, &MirrorSetupWindow::OnBestGuess);

        toolBarLayout->addWidget(mButtonOpen, 0, Qt::AlignLeft);
        toolBarLayout->addWidget(mButtonSave, 0, Qt::AlignLeft);
        toolBarLayout->addWidget(mButtonClear, 0, Qt::AlignLeft);

        toolBarLayout->addSpacerItem(new QSpacerItem(100, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

        QHBoxLayout* leftRightLayout = new QHBoxLayout();
        leftRightLayout->addWidget(new QLabel("Left:"), 0, Qt::AlignRight);
        mLeftEdit = new QLineEdit("Bip01 L");
        mLeftEdit->setMaximumWidth(75);
        leftRightLayout->addWidget(mLeftEdit, 0, Qt::AlignRight);
        mRightEdit = new QLineEdit("Bip01 R");
        mRightEdit->setMaximumWidth(75);
        leftRightLayout->addWidget(new QLabel("Right:"), 0, Qt::AlignRight);
        leftRightLayout->addWidget(mRightEdit, 0, Qt::AlignRight);
        leftRightLayout->addWidget(mButtonGuess, 0, Qt::AlignRight);
        leftRightLayout->setSpacing(6);
        leftRightLayout->setMargin(0);

        toolBarLayout->addLayout(leftRightLayout);
        /*  QWidget* spacerWidget = new QWidget();
            spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
            toolBarLayout->addWidget(spacerWidget);
        */
        // create the two widgets used to create a splitter in between
        QWidget* upperWidget = new QWidget();
        QWidget* lowerWidget = new QWidget();
        QSplitter* splitter = new QSplitter(Qt::Vertical);
        splitter->addWidget(upperWidget);
        splitter->addWidget(lowerWidget);
        mainLayout->addWidget(splitter);
        upperWidget->setLayout(topPartLayout);

        // left listbox part
        QVBoxLayout* leftListLayout = new QVBoxLayout();
        leftListLayout->setMargin(0);
        leftListLayout->setSpacing(1);
        topPartLayout->addLayout(leftListLayout);

        // add the search filter button
        QHBoxLayout* curSearchLayout = new QHBoxLayout();
        leftListLayout->addLayout(curSearchLayout);
        QLabel* curLabel = new QLabel("<b>Left Nodes</b>");
        curLabel->setTextFormat(Qt::RichText);
        curSearchLayout->addWidget(curLabel);
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        curSearchLayout->addWidget(spacerWidget);
        m_searchWidgetCurrent = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidgetCurrent, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &MirrorSetupWindow::OnCurrentTextFilterChanged);
        curSearchLayout->addWidget(m_searchWidgetCurrent);
        curSearchLayout->setSpacing(6);
        curSearchLayout->setMargin(0);

        mCurrentList = new QTableWidget();
        mCurrentList->setAlternatingRowColors(true);
        mCurrentList->setGridStyle(Qt::SolidLine);
        mCurrentList->setSelectionBehavior(QAbstractItemView::SelectRows);
        mCurrentList->setSelectionMode(QAbstractItemView::SingleSelection);
        //mCurrentList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mCurrentList->setCornerButtonEnabled(false);
        mCurrentList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mCurrentList->setContextMenuPolicy(Qt::DefaultContextMenu);
        mCurrentList->setColumnCount(3);
        mCurrentList->setColumnWidth(0, 20);
        mCurrentList->setColumnWidth(1, 20);
        mCurrentList->setSortingEnabled(true);
        QHeaderView* verticalHeader = mCurrentList->verticalHeader();
        verticalHeader->setVisible(false);
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mCurrentList->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mCurrentList->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mCurrentList->setHorizontalHeaderItem(2, headerItem);
        mCurrentList->horizontalHeader()->setStretchLastSection(true);
        mCurrentList->horizontalHeader()->setSortIndicatorShown(false);
        mCurrentList->horizontalHeader()->setSectionsClickable(false);
        connect(mCurrentList, &QTableWidget::itemSelectionChanged, this, &MirrorSetupWindow::OnCurrentListSelectionChanged);
        connect(mCurrentList, &QTableWidget::itemDoubleClicked, this, &MirrorSetupWindow::OnCurrentListDoubleClicked);
        leftListLayout->addWidget(mCurrentList);

        // add link button middle part
        QVBoxLayout* middleLayout = new QVBoxLayout();
        middleLayout->setMargin(0);
        topPartLayout->addLayout(middleLayout);
        QPushButton* linkButton = new QPushButton("link");
        connect(linkButton, &QPushButton::clicked, this, &MirrorSetupWindow::OnLinkPressed);
        middleLayout->addWidget(linkButton);

        // right listbox part
        QVBoxLayout* rightListLayout = new QVBoxLayout();
        rightListLayout->setMargin(0);
        rightListLayout->setSpacing(1);
        topPartLayout->addLayout(rightListLayout);

        // add the search filter button
        QHBoxLayout* sourceSearchLayout = new QHBoxLayout();
        rightListLayout->addLayout(sourceSearchLayout);
        QLabel* sourceLabel = new QLabel("<b>Right Nodes</b>");
        sourceLabel->setTextFormat(Qt::RichText);
        sourceSearchLayout->addWidget(sourceLabel);
        //QPushButton* loadSourceButton = new QPushButton();
        //EMStudioManager::MakeTransparentButton( loadSourceButton, "Images/Icons/Open.svg",   "Load a source actor" );
        //connect( loadSourceButton, SIGNAL(clicked()), this, SLOT(OnLoadSourceActor()) );
        //sourceSearchLayout->addWidget( loadSourceButton, 0, Qt::AlignLeft );
        spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        sourceSearchLayout->addWidget(spacerWidget);
        m_searchWidgetSource = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidgetSource, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &MirrorSetupWindow::OnSourceTextFilterChanged);
        sourceSearchLayout->addWidget(m_searchWidgetSource);
        sourceSearchLayout->setSpacing(6);
        sourceSearchLayout->setMargin(0);

        mSourceList = new QTableWidget();
        mSourceList->setAlternatingRowColors(true);
        mSourceList->setGridStyle(Qt::SolidLine);
        mSourceList->setSelectionBehavior(QAbstractItemView::SelectRows);
        mSourceList->setSelectionMode(QAbstractItemView::SingleSelection);
        //mSourceList->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mSourceList->setCornerButtonEnabled(false);
        mSourceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mSourceList->setContextMenuPolicy(Qt::DefaultContextMenu);
        mSourceList->setColumnCount(3);
        mSourceList->setColumnWidth(0, 20);
        mSourceList->setColumnWidth(1, 20);
        mSourceList->setSortingEnabled(true);
        verticalHeader = mSourceList->verticalHeader();
        verticalHeader->setVisible(false);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mSourceList->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mSourceList->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mSourceList->setHorizontalHeaderItem(2, headerItem);
        mSourceList->horizontalHeader()->setStretchLastSection(true);
        mSourceList->horizontalHeader()->setSortIndicatorShown(false);
        mSourceList->horizontalHeader()->setSectionsClickable(false);
        connect(mSourceList, &QTableWidget::itemSelectionChanged, this, &MirrorSetupWindow::OnSourceListSelectionChanged);
        rightListLayout->addWidget(mSourceList);

        // create the mapping table
        QVBoxLayout* lowerLayout = new QVBoxLayout();
        lowerLayout->setMargin(0);
        lowerLayout->setSpacing(3);
        lowerWidget->setLayout(lowerLayout);

        QHBoxLayout* mappingLayout = new QHBoxLayout();
        mappingLayout->setMargin(0);
        lowerLayout->addLayout(mappingLayout);
        mappingLayout->addWidget(new QLabel("Mapping:"), 0, Qt::AlignLeft | Qt::AlignVCenter);
        //mButtonGuess = new QPushButton();
        //EMStudioManager::MakeTransparentButton( mButtonGuess, "Images/Icons/Character.svg",  "Best guess mapping" );
        //connect( mButtonGuess, SIGNAL(clicked()), this, SLOT(OnBestGuessGeometrical()) );
        //mappingLayout->addWidget( mButtonGuess, 0, Qt::AlignLeft );
        spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        mappingLayout->addWidget(spacerWidget);

        mMappingTable = new QTableWidget();
        lowerLayout->addWidget(mMappingTable);
        mMappingTable->setAlternatingRowColors(true);
        mMappingTable->setGridStyle(Qt::SolidLine);
        mMappingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        mMappingTable->setSelectionMode(QAbstractItemView::SingleSelection);
        //mMappingTable->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mMappingTable->setCornerButtonEnabled(false);
        mMappingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mMappingTable->setContextMenuPolicy(Qt::DefaultContextMenu);
        mMappingTable->setContentsMargins(3, 1, 3, 1);
        mMappingTable->setColumnCount(2);
        mMappingTable->setColumnWidth(0, mMappingTable->width() / 2);
        mMappingTable->setColumnWidth(1, mMappingTable->width() / 2);
        verticalHeader = mMappingTable->verticalHeader();
        verticalHeader->setVisible(false);

        // add the table headers
        headerItem = new QTableWidgetItem("Node");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mMappingTable->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Mapped to");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        mMappingTable->setHorizontalHeaderItem(1, headerItem);
        mMappingTable->horizontalHeader()->setStretchLastSection(true);
        mMappingTable->horizontalHeader()->setSortIndicatorShown(false);
        mMappingTable->horizontalHeader()->setSectionsClickable(false);
        connect(mMappingTable, &QTableWidget::itemDoubleClicked, this, &MirrorSetupWindow::OnMappingTableDoubleClicked);
        connect(mMappingTable, &QTableWidget::itemSelectionChanged, this, &MirrorSetupWindow::OnMappingTableSelectionChanged);
    }


    // destructor
    MirrorSetupWindow::~MirrorSetupWindow()
    {
        delete mBoneIcon;
        delete mNodeIcon;
        delete mMeshIcon;
        delete mMappedIcon;
    }


    // double clicked on an item in the table
    void MirrorSetupWindow::OnMappingTableDoubleClicked(QTableWidgetItem* item)
    {
        MCORE_UNUSED(item);
        // TODO: open a node hierarchy widget, where we can select a node from the mSourceActor
        // the problem is that the node hierarchy widget works with actor instances, which we don't have and do not really want to create either
        // I think the node hierarchy widget shouldn't use actor instances only, but should support actors as well
    }


    // when we double click on an item in the current node list
    void MirrorSetupWindow::OnCurrentListDoubleClicked(QTableWidgetItem* item)
    {
        // get the currently selected actor
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        if (!currentActor)
        {
            return;
        }

        if (item->column() != 0)
        {
            return;
        }

        // get the node name
        const uint32 rowIndex = item->row();
        const AZStd::string nodeName = mCurrentList->item(rowIndex, 2)->text().toUtf8().data();

        // find its index in the current actor, and remove its mapping
        EMotionFX::Node* node = currentActor->GetSkeleton()->FindNodeByName(nodeName.c_str());
        MCORE_ASSERT(node);

        // remove the mapping for this node
        PerformMapping(node->GetNodeIndex(), MCORE_INVALIDINDEX32);
    }


    // current list selection changed
    void MirrorSetupWindow::OnCurrentListSelectionChanged()
    {
        QList<QTableWidgetItem*> items = mCurrentList->selectedItems();
        if (items.count() > 0)
        {
            //const uint32 currentListRow = items[0]->row();
            QTableWidgetItem* nameItem = mCurrentList->item(items[0]->row(), 2);
            QList<QTableWidgetItem*> mappingTableItems = mMappingTable->findItems(nameItem->text(), Qt::MatchExactly);

            for (int32 i = 0; i < mappingTableItems.count(); ++i)
            {
                if (mappingTableItems[i]->column() != 0)
                {
                    continue;
                }

                const uint32 rowIndex = mappingTableItems[i]->row();

                mMappingTable->selectRow(rowIndex);
                mMappingTable->setCurrentItem(mappingTableItems[i]);
            }
        }
    }


    // source list selection changed
    void MirrorSetupWindow::OnSourceListSelectionChanged()
    {
        QList<QTableWidgetItem*> items = mSourceList->selectedItems();
        if (items.count() > 0)
        {
            //const uint32 currentListRow = items[0]->row();
            QTableWidgetItem* nameItem = mSourceList->item(items[0]->row(), 2);
            QList<QTableWidgetItem*> mappingTableItems = mMappingTable->findItems(nameItem->text(), Qt::MatchExactly);

            for (int32 i = 0; i < mappingTableItems.count(); ++i)
            {
                if (mappingTableItems[i]->column() != 1)
                {
                    continue;
                }

                const uint32 rowIndex = mappingTableItems[i]->row();

                mMappingTable->selectRow(rowIndex);
                mMappingTable->setCurrentItem(mappingTableItems[i]);
            }
        }
    }


    // selection changed in the table
    void MirrorSetupWindow::OnMappingTableSelectionChanged()
    {
        // select both items in the list widgets as well
        QList<QTableWidgetItem*> items = mMappingTable->selectedItems();
        if (items.count() > 0)
        {
            const uint32 rowIndex = items[0]->row();

            QTableWidgetItem* item = mMappingTable->item(rowIndex, 0);
            if (item)
            {
                QList<QTableWidgetItem*> listItems = mCurrentList->findItems(item->text(), Qt::MatchExactly);
                if (listItems.count() > 0)
                {
                    mCurrentList->selectRow(listItems[0]->row());
                    mCurrentList->setCurrentItem(listItems[0]);
                }
            }

            item = mMappingTable->item(rowIndex, 1);
            if (item)
            {
                QList<QTableWidgetItem*> listItems = mSourceList->findItems(item->text(), Qt::MatchExactly);
                if (listItems.count() > 0)
                {
                    mSourceList->selectRow(listItems[0]->row());
                    mSourceList->setCurrentItem(listItems[0]);
                }
            }
        }
    }


    // reinitialize
    void MirrorSetupWindow::Reinit(bool reInitMap)
    {
        // clear the filter strings
        m_searchWidgetCurrent->ClearTextFilter();
        m_searchWidgetSource->ClearTextFilter();

        // get the currently selected actor
        EMotionFX::Actor* currentActor = GetSelectedActor();

        // extract the list of bones
        if (currentActor)
        {
            currentActor->ExtractBoneList(0, &mCurrentBoneList);
        }

        // clear the node map
        if (reInitMap)
        {
            mMap.clear();
            if (currentActor)
            {
                const size_t numNodes = aznumeric_caster(currentActor->GetNumNodes());
                mMap.resize(numNodes);
                for (uint32 i = 0; i < numNodes; ++i)
                {
                    mMap[i] = MCORE_INVALIDINDEX32;
                }
            }
        }

        // fill the contents
        InitMappingTableFromMotionSources(currentActor);
        FillCurrentListWidget(currentActor, "");
        FillSourceListWidget(currentActor, "");
        FillMappingTable(currentActor, currentActor);

        // enable or disable the filter fields
        m_searchWidgetCurrent->setEnabled((currentActor));
        m_searchWidgetSource->setEnabled((currentActor));

        //
        UpdateToolBar();
    }


    // current actor filter change
    void MirrorSetupWindow::OnCurrentTextFilterChanged(const QString& text)
    {
        FillCurrentListWidget(GetSelectedActor(), text);
    }


    // source actor filter change
    void MirrorSetupWindow::OnSourceTextFilterChanged(const QString& text)
    {
        FillSourceListWidget(GetSelectedActor(), text);
    }


    // fill the current list widget
    void MirrorSetupWindow::FillCurrentListWidget(EMotionFX::Actor* actor, const QString& filterString)
    {
        if (!actor)
        {
            mCurrentList->setRowCount(0);
            return;
        }

        // fill the left list widget
        QString currentName;
        const uint32 numNodes = actor->GetNumNodes();

        // count the number of rows
        uint32 numRows = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            currentName = actor->GetSkeleton()->GetNode(i)->GetName();
            if (currentName.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                numRows++;
            }
        }
        mCurrentList->setRowCount(numRows);

        // fill the rows
        uint32 rowIndex = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            currentName = node->GetName();
            if (currentName.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                // mark if there is a mapping or not
                const bool mapped = (mMap[node->GetNodeIndex()] != MCORE_INVALIDINDEX32);
                QTableWidgetItem* mappedItem = new QTableWidgetItem();
                mappedItem->setIcon(mapped ? *mMappedIcon : QIcon());
                mCurrentList->setItem(rowIndex, 0, mappedItem);

                // pick the right icon for the type column
                QTableWidgetItem* typeItem = new QTableWidgetItem();
                if (actor->GetMesh(0, node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mMeshIcon);
                }
                else
                if (mCurrentBoneList.Contains(node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mBoneIcon);
                }
                else
                {
                    typeItem->setIcon(*mNodeIcon);
                }

                mCurrentList->setItem(rowIndex, 1, typeItem);

                // set the name
                QTableWidgetItem* currentTableItem = new QTableWidgetItem(currentName);
                mCurrentList->setItem(rowIndex, 2, currentTableItem);

                // set the row height and add one index
                mCurrentList->setRowHeight(rowIndex, 21);
                ++rowIndex;
            }
        }
    }


    // fill the source list widget
    void MirrorSetupWindow::FillSourceListWidget(EMotionFX::Actor* actor, const QString& filterString)
    {
        if (!actor)
        {
            mSourceList->setRowCount(0);
            return;
        }

        // fill the left list widget
        QString name;
        const uint32 numNodes = actor->GetNumNodes();

        // count the number of rows
        uint32 numRows = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            name = actor->GetSkeleton()->GetNode(i)->GetName();
            if (name.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                numRows++;
            }
        }
        mSourceList->setRowCount(numRows);

        // fill the rows
        uint32 rowIndex = 0;
        for (uint32 i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            name = node->GetName();
            if (name.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                // mark if there is a mapping or not
                const bool mapped = AZStd::find(mMap.begin(), mMap.end(), i) != mMap.end();
                QTableWidgetItem* mappedItem = new QTableWidgetItem();
                mappedItem->setIcon(mapped ? *mMappedIcon : QIcon());
                mSourceList->setItem(rowIndex, 0, mappedItem);

                // pick the right icon for the type column
                QTableWidgetItem* typeItem = new QTableWidgetItem();
                if (actor->GetMesh(0, node->GetNodeIndex()))
                {
                    typeItem->setIcon(*mMeshIcon);
                }
                else
                if (AZStd::find(mSourceBoneList.begin(), mSourceBoneList.end(), node->GetNodeIndex()) != mSourceBoneList.end())
                {
                    typeItem->setIcon(*mBoneIcon);
                }
                else
                {
                    typeItem->setIcon(*mNodeIcon);
                }

                mSourceList->setItem(rowIndex, 1, typeItem);

                // set the name
                QTableWidgetItem* currentTableItem = new QTableWidgetItem(name);
                mSourceList->setItem(rowIndex, 2, currentTableItem);

                // set the row height and add one index
                mSourceList->setRowHeight(rowIndex, 21);
                ++rowIndex;
            }
        }
    }


    // fill the mapping table
    void MirrorSetupWindow::FillMappingTable(EMotionFX::Actor* currentActor, EMotionFX::Actor* sourceActor)
    {
        if (!currentActor)
        {
            mMappingTable->setRowCount(0);
            return;
        }

        // fill the table
        QString currentName;
        QString sourceName;
        const uint32 numNodes = currentActor->GetNumNodes();
        mMappingTable->setRowCount(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            currentName = currentActor->GetSkeleton()->GetNode(i)->GetName();

            QTableWidgetItem* currentTableItem = new QTableWidgetItem(currentName);
            mMappingTable->setItem(i, 0, currentTableItem);
            mMappingTable->setRowHeight(i, 21);

            if (mMap[i] != MCORE_INVALIDINDEX32)
            {
                sourceName = sourceActor->GetSkeleton()->GetNode(mMap[i])->GetName();
                currentTableItem = new QTableWidgetItem(sourceName);
                mMappingTable->setItem(i, 1, currentTableItem);
            }
            else
            {
                mMappingTable->setItem(i, 1, new QTableWidgetItem());
            }
        }
        //mMappingTable->resizeColumnsToContents();
        //mMappingTable->setColumnWidth(0, mMappingTable->columnWidth(0) + 25);
    }


    // pressing the link button
    void MirrorSetupWindow::OnLinkPressed()
    {
        if (mCurrentList->currentRow() == -1 || mSourceList->currentRow() == -1)
        {
            return;
        }

        // get the names
        QTableWidgetItem* curItem = mCurrentList->currentItem();
        QTableWidgetItem* sourceItem = mSourceList->currentItem();
        if (!curItem || !sourceItem)
        {
            return;
        }

        curItem = mCurrentList->item(curItem->row(), 2);
        sourceItem = mSourceList->item(sourceItem->row(), 2);

        const AZStd::string currentNodeName = curItem->text().toUtf8().data();
        const AZStd::string sourceNodeName  = mSourceList->currentItem()->text().toUtf8().data();
        if (sourceNodeName.empty() || currentNodeName.empty())
        {
            return;
        }

        // get the currently selected actor
        EMotionFX::Actor* currentActor = GetSelectedActor();
        MCORE_ASSERT(currentActor);

        EMotionFX::Node* currentNode = currentActor->GetSkeleton()->FindNodeByName(currentNodeName.c_str());
        EMotionFX::Node* sourceNode  = currentActor->GetSkeleton()->FindNodeByName(sourceNodeName.c_str());
        MCORE_ASSERT(currentNode && sourceNode);

        PerformMapping(currentNode->GetNodeIndex(), sourceNode->GetNodeIndex());
    }


    // perform the mapping
    void MirrorSetupWindow::PerformMapping(uint32 currentNodeIndex, uint32 sourceNodeIndex)
    {
        EMotionFX::Actor* currentActor = GetSelectedActor();

        // update the map
        const uint32 oldSourceIndex = mMap[currentNodeIndex];
        mMap[currentNodeIndex] = sourceNodeIndex;

        // update the current table
        const QString curName = currentActor->GetSkeleton()->GetNode(currentNodeIndex)->GetName();
        const QList<QTableWidgetItem*> currentListItems = mCurrentList->findItems(curName, Qt::MatchExactly);
        for (int32 i = 0; i < currentListItems.count(); ++i)
        {
            const uint32 rowIndex = currentListItems[i]->row();
            //if (rowIndex != mCurrentList->currentRow())
            //  continue;

            QTableWidgetItem* mappedItem = mCurrentList->item(rowIndex, 0);
            if (!mappedItem)
            {
                mappedItem = new QTableWidgetItem();
                mCurrentList->setItem(rowIndex, 0, mappedItem);
            }

            if (sourceNodeIndex == MCORE_INVALIDINDEX32)
            {
                mappedItem->setIcon(QIcon());
            }
            else
            {
                mappedItem->setIcon(*mMappedIcon);
            }
        }

        // update source table
        if (sourceNodeIndex != MCORE_INVALIDINDEX32)
        {
            const bool stillUsed = AZStd::find(mMap.begin(), mMap.end(), sourceNodeIndex) != mMap.end();
            const QString sourceName = currentActor->GetSkeleton()->GetNode(sourceNodeIndex)->GetName();
            const QList<QTableWidgetItem*> sourceListItems = mSourceList->findItems(sourceName, Qt::MatchExactly);
            for (int32 i = 0; i < sourceListItems.count(); ++i)
            {
                const uint32 rowIndex = sourceListItems[i]->row();
                //if (rowIndex != mSourceList->currentRow())
                //  continue;

                QTableWidgetItem* mappedItem = mSourceList->item(rowIndex, 0);
                if (!mappedItem)
                {
                    mappedItem = new QTableWidgetItem();
                    mSourceList->setItem(rowIndex, 0, mappedItem);
                }

                if (stillUsed == false)
                {
                    mappedItem->setIcon(QIcon());
                }
                else
                {
                    mappedItem->setIcon(*mMappedIcon);
                }
            }
        }
        else // we're clearing it
        {
            if (oldSourceIndex != MCORE_INVALIDINDEX32)
            {
                const bool stillUsed = AZStd::find(mMap.begin(), mMap.end(), sourceNodeIndex) != mMap.end();
                const QString sourceName = currentActor->GetSkeleton()->GetNode(oldSourceIndex)->GetName();
                const QList<QTableWidgetItem*> sourceListItems = mSourceList->findItems(sourceName, Qt::MatchExactly);
                for (int32 i = 0; i < sourceListItems.count(); ++i)
                {
                    const uint32 rowIndex = sourceListItems[i]->row();
                    //if (rowIndex != mSourceList->currentRow())
                    //  continue;

                    QTableWidgetItem* mappedItem = mSourceList->item(rowIndex, 0);
                    if (!mappedItem)
                    {
                        mappedItem = new QTableWidgetItem();
                        mSourceList->setItem(rowIndex, 0, mappedItem);
                    }

                    if (stillUsed == false)
                    {
                        mappedItem->setIcon(QIcon());
                    }
                    else
                    {
                        mappedItem->setIcon(*mMappedIcon);
                    }
                }
            }
        }

        // update the mapping table
        QTableWidgetItem* item = mMappingTable->item(currentNodeIndex, 1);
        if (!item && sourceNodeIndex != MCORE_INVALIDINDEX32)
        {
            item = new QTableWidgetItem();
            mMappingTable->setItem(currentNodeIndex, 1, item);
        }

        if (sourceNodeIndex == MCORE_INVALIDINDEX32)
        {
            if (item)
            {
                item->setText("");
            }
        }
        else
        {
            item->setText(currentActor->GetSkeleton()->GetNode(sourceNodeIndex)->GetName());
        }

        // update toolbar icons
        UpdateActorMotionSources();
        UpdateToolBar();
    }


    // press a key
    void MirrorSetupWindow::keyPressEvent(QKeyEvent* event)
    {
        event->ignore();
    }


    // releasing a key
    void MirrorSetupWindow::keyReleaseEvent(QKeyEvent* event)
    {
        // if we press delete, remove the mapping
        if (event->key() == Qt::Key_Delete)
        {
            RemoveCurrentSelectedMapping();
            event->accept();
            return;
        }

        event->ignore();
    }


    // remove the currently selected mapping
    void MirrorSetupWindow::RemoveCurrentSelectedMapping()
    {
        QList<QTableWidgetItem*> items = mCurrentList->selectedItems();
        if (items.count() > 0)
        {
            QTableWidgetItem* item = mCurrentList->item(items[0]->row(), 0);
            if (item)
            {
                OnCurrentListDoubleClicked(item);
            }
        }
    }


    // load and apply a mapping
    void MirrorSetupWindow::OnLoadMapping()
    {
        // make sure we have both a current actor and source actor
        EMotionFX::Actor* currentActor = GetSelectedActor();
        if (!currentActor)
        {
            AZ_Warning("EMotionFX", false, "There is no current actor set, a mapping cannot be loaded, please select an actor first!");
            QMessageBox::critical(this, "Cannot Save!", "You need to select a current actor before you can load and apply this node map!", QMessageBox::Ok);
            return;
        }

        // get the filename to save as
        const AZStd::string filename = GetMainWindow()->GetFileManager()->LoadNodeMapFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        // load the node map file from disk
        MCore::LogInfo("Loading node map from file '%s'", filename.c_str());
        EMotionFX::NodeMap* nodeMap = EMotionFX::GetImporter().LoadNodeMap(filename.c_str());
        if (!nodeMap)
        {
            AZ_Warning("EMotionFX", false, "Failed to load the node map!");
            QMessageBox::warning(this, "Failed Loading", "Loading of the node map file failed.", QMessageBox::Ok);
        }
        else
        {
            MCore::LogInfo("Loading of node map is successful, applying now...");
        }

        // check if the node map has a source actor
        if (nodeMap->GetSourceActor())
        {
            AZ_Warning("EMotionFX", false, "EMStudio::MirrorSetupWindow::OnLoadMapping() - There is no source actor to use, please manually load one first.");
            QMessageBox::warning(this, "No Source Actor", "Loading of the source actor inside the node map failed (or didn't contain one) and there is currently none set. Please manually load a source actor first and try again.", QMessageBox::Ok);
            return;
        }

        // now update our mapping data
        const uint32 numNodes = currentActor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            mMap[i] = MCORE_INVALIDINDEX32;
        }

        // now apply the map we loaded to the data we have here
        const uint32 numEntries = nodeMap->GetNumEntries();
        for (uint32 i = 0; i < numEntries; ++i)
        {
            // find the current node
            EMotionFX::Node* currentNode = currentActor->GetSkeleton()->FindNodeByName(nodeMap->GetFirstName(i));
            if (!currentNode)
            {
                continue;
            }

            // find the source node
            EMotionFX::Node* sourceNode = currentActor->GetSkeleton()->FindNodeByName(nodeMap->GetSecondName(i));
            if (!sourceNode)
            {
                continue;
            }

            // create the mapping
            mMap[currentNode->GetNodeIndex()] = sourceNode->GetNodeIndex();
        }

        // apply the current map as command
        ApplyCurrentMapAsCommand();

        // reinit the interface parts
        Reinit(false);

        // get rid of the nodeMap object as we don't need it anymore
        nodeMap->Destroy();
    }


    // save the mapping
    void MirrorSetupWindow::OnSaveMapping()
    {
        // get the currently selected actor
        EMotionFX::Actor* currentActor = GetSelectedActor();
        if (!currentActor)
        {
            AZ_Warning("EMotionFX", false, "There is no current actor set, there is nothing to save!");
            QMessageBox::warning(this, "Nothing To Save!", "You need to select a current actor before you can save a map!", QMessageBox::Ok);
            return;
        }

        // check if we got something to save at all
        if (CheckIfIsMapEmpty())
        {
            AZ_Warning("EMotionFX", false, "The node map is empty, there is nothing to save!");
            QMessageBox::warning(this, "Nothing To Save!", "The node map is empty, there is nothing to save!", QMessageBox::Ok);
            return;
        }

        // read the filename to save as
        const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveNodeMapFileDialog(this);
        if (filename.empty())
        {
            return;
        }

        MCore::LogInfo("Saving node map as '%s'", filename.c_str());

        // create an emfx node map object
        EMotionFX::NodeMap* map = EMotionFX::NodeMap::Create();
        const uint32 numNodes = currentActor->GetNumNodes();
        map->Reserve(numNodes);
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // skip unmapped entries
            if (mMap[i] == MCORE_INVALIDINDEX32)
            {
                continue;
            }

            // add the entry to the map if it doesn't yet exist
            if (map->GetHasEntry(currentActor->GetSkeleton()->GetNode(i)->GetName()) == false)
            {
                map->AddEntry(currentActor->GetSkeleton()->GetNode(i)->GetName(), currentActor->GetSkeleton()->GetNode(mMap[i])->GetName());
            }
        }

        // set the filename, in case we do something with it while saving later on
        map->SetFileName(filename.c_str());

        // save as little endian
        if (map->Save(filename.c_str(), MCore::Endian::ENDIAN_LITTLE) == false)
        {
            AZ_Warning("EMotionFX", false, "Failed to save node map file '%s', is it maybe in use or is the location read only?", filename.c_str());
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Node map <font color=red>failed</font> to save");
        }
        else
        {
            MCore::LogInfo("Saving of node map successfully completed");
            GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Node map <font color=green>successfully</font> saved");
        }

        map->Destroy();
    }


    // clear the mapping
    void MirrorSetupWindow::OnClearMapping()
    {
        if (QMessageBox::warning(this, "Clear Current Mapping?", "Are you sure you want to clear the current mapping?\nAll mapping information will be lost.", QMessageBox::Cancel | QMessageBox::Yes) != QMessageBox::Yes)
        {
            return;
        }

        EMotionFX::Actor* currentActor = GetSelectedActor();

        // apply mirror changes
        const AZStd::string command = AZStd::string::format("AdjustActor -actorID %d -mirrorSetup \"\"", currentActor->GetID());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // reinitialize, which also clears the map
        Reinit();
    }


    // check if the current map is empty
    bool MirrorSetupWindow::CheckIfIsMapEmpty() const
    {
        EMotionFX::Actor* currentActor = GetSelectedActor();

        if (!currentActor)
        {
            return true;
        }

        const uint32 numNodes = currentActor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (mMap[i] != MCORE_INVALIDINDEX32)
            {
                return false;
            }
        }

        return true;
    }


    // update the toolbar icons
    void MirrorSetupWindow::UpdateToolBar()
    {
        EMotionFX::Actor* currentActor = GetSelectedActor();

        // check which buttons have to be enabled/disabled
        const bool canOpen  = (currentActor);
        const bool canSave  = (CheckIfIsMapEmpty() == false && currentActor);
        const bool canClear = (CheckIfIsMapEmpty() == false);
        const bool canGuess = (currentActor);

        // enable or disable them
        mButtonOpen->setEnabled(canOpen);
        mButtonSave->setEnabled(canSave);
        mButtonClear->setEnabled(canClear);
        mButtonGuess->setEnabled(canGuess);
    }


    // perform best guess mapping
    void MirrorSetupWindow::OnBestGuess()
    {
        // get the current and source actor
        EMotionFX::Actor* currentActor = GetSelectedActor();
        if (!currentActor)
        {
            return;
        }

        if (mLeftEdit->text().size() == 0 || mRightEdit->text().size() == 0)
        {
            QMessageBox::information(this, "Empty Left And Right Strings", "Please enter both a left and right sub-string.\nThis can be something like 'Left' and 'Right'.\nThis would map nodes like 'Left Arm' to 'Right Arm' nodes.", QMessageBox::Ok);
            return;
        }

        // show a warning that we will overwrite the table entries
        //if (QMessageBox::warning(this, "Overwrite Mapping?", "Are you sure you want to possibly overwrite items in the mapping?\nAll or some existing mapping information might be lost.", QMessageBox::Cancel|QMessageBox::Yes) != QMessageBox::Yes)
        //return;

        //
        //  currentActor->MatchNodeMotionSources( FromQtString(mLeftEdit->text()), FromQtString(mRightEdit->text()) );

        // update the table and map
        uint32 numGuessed = 0;
        const uint32 numNodes = currentActor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // skip already setup mappings
            if (mMap[i] != MCORE_INVALIDINDEX32)
            {
                continue;
            }

            const uint16 matchIndex = currentActor->FindBestMatchForNode(currentActor->GetSkeleton()->GetNode(i)->GetName(), FromQtString(mLeftEdit->text()).c_str(), FromQtString(mRightEdit->text()).c_str());
            if (matchIndex != MCORE_INVALIDINDEX16)
            {
                mMap[i] = matchIndex;
                numGuessed++;
            }
        }

        // update the actor
        UpdateActorMotionSources();

        /*
            // try a geometrical mapping
            EMotionFX::Pose pose;
            pose.InitFromLocalBindSpaceTransforms( currentActor );

            // for all nodes in the current actor
            uint32 numGuessed = 0;
            const uint32 numNodes = currentActor->GetNumNodes();
            for (uint32 i=0; i<numNodes; ++i)
            {
                // skip already setup mappings
                if (mMap[i] != MCORE_INVALIDINDEX32)
                    continue;

                const uint16 matchIndex = currentActor->FindBestMirrorMatchForNode( static_cast<uint16>(i), pose );
                if (matchIndex != MCORE_INVALIDINDEX16)
                {
                    mMap[i] = matchIndex;
                    numGuessed++;
                }
            }
        */
        Reinit(false);

        // show some results
        QMessageBox::information(this, "Mirror Mapping Results", AZStd::string::format("We modified mappings for %d nodes.", numGuessed).c_str());
    }


    //
    void MirrorSetupWindow::UpdateActorMotionSources()
    {
        // get the current and source actor
        EMotionFX::Actor* currentActor = GetSelectedActor();
        if (!currentActor)
        {
            return;
        }
        /*
            const uint32 numNodes = currentActor->GetNumNodes();
            for (uint32 i=0; i<numNodes; ++i)
            {
                uint16 motionSource = i;

                // get the motion source from the map
                if (mMap[i] != MCORE_INVALIDINDEX32)
                    motionSource = static_cast<uint16>( mMap[i] );
            }
        */
        // apply the current map as command
        ApplyCurrentMapAsCommand();
    }


    // init the mapping table from a given actor
    void MirrorSetupWindow::InitMappingTableFromMotionSources(EMotionFX::Actor* actor)
    {
        if (!actor)
        {
            return;
        }

        const uint32 numNodes = actor->GetNumNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            if (actor->GetHasMirrorInfo())
            {
                uint16 motionSource = actor->GetNodeMirrorInfo(i).mSourceNode;
                if (motionSource != i)
                {
                    mMap[i] = motionSource;
                }
                else
                {
                    mMap[i] = MCORE_INVALIDINDEX32;
                }
            }
            else
            {
                mMap[i] = MCORE_INVALIDINDEX32;
            }
        }
    }


    // apply the current map as command
    void MirrorSetupWindow::ApplyCurrentMapAsCommand()
    {
        // get the current and source actor
        EMotionFX::Actor* currentActor = GetSelectedActor();
        if (!currentActor)
        {
            return;
        }

        // apply mirror changes
        AZStd::string commandString = AZStd::string::format("AdjustActor -actorID %d -mirrorSetup \"", currentActor->GetID());
        for (uint32 i = 0; i < currentActor->GetNumNodes(); ++i)
        {
            uint32 sourceNode = mMap[i];
            if (sourceNode != MCORE_INVALIDINDEX32 && sourceNode != i)
            {
                commandString += currentActor->GetSkeleton()->GetNode(i)->GetName();
                commandString += ",";
                commandString += currentActor->GetSkeleton()->GetNode(sourceNode)->GetName();
                commandString += ";";
            }
        }
        commandString += "\"";

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        FillMappingTable(currentActor, currentActor);
        UpdateToolBar();
    }

    EMotionFX::Actor* MirrorSetupWindow::GetSelectedActor() const
    {
        CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        EMotionFX::Actor* currentActor = selection.GetSingleActor();
        if (currentActor)
        {
            return currentActor;
        }

        EMotionFX::ActorInstance* currentActorInstance = selection.GetSingleActorInstance();
        if (currentActorInstance)
        {
            return currentActorInstance->GetActor();
        }

        return nullptr;
    }

}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/moc_MirrorSetupWindow.cpp>
