/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_plugin = plugin;

        // set the window title
        setWindowTitle("Mirror Setup");

        // set the minimum size
        setMinimumWidth(800);
        setMinimumHeight(600);

        // load some icons
        const QDir dataDir{ QString(MysticQt::GetDataDir().c_str()) };
        m_boneIcon   = new QIcon(dataDir.filePath("Images/Icons/Bone.svg"));
        m_nodeIcon   = new QIcon(dataDir.filePath("Images/Icons/Node.svg"));
        m_meshIcon   = new QIcon(dataDir.filePath("Images/Icons/Mesh.svg"));
        m_mappedIcon = new QIcon(dataDir.filePath("Images/Icons/Confirm.svg"));

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
        m_buttonOpen = new QPushButton();
        EMStudioManager::MakeTransparentButton(m_buttonOpen,    "Images/Icons/Open.svg",       "Load and apply a mapping template.");
        connect(m_buttonOpen, &QPushButton::clicked, this, &MirrorSetupWindow::OnLoadMapping);
        m_buttonSave = new QPushButton();
        EMStudioManager::MakeTransparentButton(m_buttonSave,    "Images/Menu/FileSave.svg",    "Save the currently setup mapping as template.");
        connect(m_buttonSave, &QPushButton::clicked, this, &MirrorSetupWindow::OnSaveMapping);
        m_buttonClear = new QPushButton();
        EMStudioManager::MakeTransparentButton(m_buttonClear,   "Images/Icons/Clear.svg",      "Clear the currently setup mapping entirely.");
        connect(m_buttonClear, &QPushButton::clicked, this, &MirrorSetupWindow::OnClearMapping);

        m_buttonGuess = new QPushButton();
        EMStudioManager::MakeTransparentButton(m_buttonGuess,   "Images/Icons/Character.svg",  "Perform name based mapping.");
        connect(m_buttonGuess, &QPushButton::clicked, this, &MirrorSetupWindow::OnBestGuess);

        toolBarLayout->addWidget(m_buttonOpen, 0, Qt::AlignLeft);
        toolBarLayout->addWidget(m_buttonSave, 0, Qt::AlignLeft);
        toolBarLayout->addWidget(m_buttonClear, 0, Qt::AlignLeft);

        toolBarLayout->addSpacerItem(new QSpacerItem(100, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

        QHBoxLayout* leftRightLayout = new QHBoxLayout();
        leftRightLayout->addWidget(new QLabel("Left:"), 0, Qt::AlignRight);
        m_leftEdit = new QLineEdit("Bip01 L");
        m_leftEdit->setMaximumWidth(75);
        leftRightLayout->addWidget(m_leftEdit, 0, Qt::AlignRight);
        m_rightEdit = new QLineEdit("Bip01 R");
        m_rightEdit->setMaximumWidth(75);
        leftRightLayout->addWidget(new QLabel("Right:"), 0, Qt::AlignRight);
        leftRightLayout->addWidget(m_rightEdit, 0, Qt::AlignRight);
        leftRightLayout->addWidget(m_buttonGuess, 0, Qt::AlignRight);
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

        m_currentList = new QTableWidget();
        m_currentList->setAlternatingRowColors(true);
        m_currentList->setGridStyle(Qt::SolidLine);
        m_currentList->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_currentList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_currentList->setCornerButtonEnabled(false);
        m_currentList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_currentList->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_currentList->setColumnCount(3);
        m_currentList->setColumnWidth(0, 20);
        m_currentList->setColumnWidth(1, 20);
        m_currentList->setSortingEnabled(true);
        QHeaderView* verticalHeader = m_currentList->verticalHeader();
        verticalHeader->setVisible(false);
        QTableWidgetItem* headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_currentList->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_currentList->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_currentList->setHorizontalHeaderItem(2, headerItem);
        m_currentList->horizontalHeader()->setStretchLastSection(true);
        m_currentList->horizontalHeader()->setSortIndicatorShown(false);
        m_currentList->horizontalHeader()->setSectionsClickable(false);
        connect(m_currentList, &QTableWidget::itemSelectionChanged, this, &MirrorSetupWindow::OnCurrentListSelectionChanged);
        connect(m_currentList, &QTableWidget::itemDoubleClicked, this, &MirrorSetupWindow::OnCurrentListDoubleClicked);
        leftListLayout->addWidget(m_currentList);

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

        m_sourceList = new QTableWidget();
        m_sourceList->setAlternatingRowColors(true);
        m_sourceList->setGridStyle(Qt::SolidLine);
        m_sourceList->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_sourceList->setSelectionMode(QAbstractItemView::SingleSelection);
        m_sourceList->setCornerButtonEnabled(false);
        m_sourceList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_sourceList->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_sourceList->setColumnCount(3);
        m_sourceList->setColumnWidth(0, 20);
        m_sourceList->setColumnWidth(1, 20);
        m_sourceList->setSortingEnabled(true);
        verticalHeader = m_sourceList->verticalHeader();
        verticalHeader->setVisible(false);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_sourceList->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_sourceList->setHorizontalHeaderItem(1, headerItem);
        headerItem = new QTableWidgetItem("Name");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_sourceList->setHorizontalHeaderItem(2, headerItem);
        m_sourceList->horizontalHeader()->setStretchLastSection(true);
        m_sourceList->horizontalHeader()->setSortIndicatorShown(false);
        m_sourceList->horizontalHeader()->setSectionsClickable(false);
        connect(m_sourceList, &QTableWidget::itemSelectionChanged, this, &MirrorSetupWindow::OnSourceListSelectionChanged);
        rightListLayout->addWidget(m_sourceList);

        // create the mapping table
        QVBoxLayout* lowerLayout = new QVBoxLayout();
        lowerLayout->setMargin(0);
        lowerLayout->setSpacing(3);
        lowerWidget->setLayout(lowerLayout);

        QHBoxLayout* mappingLayout = new QHBoxLayout();
        mappingLayout->setMargin(0);
        lowerLayout->addLayout(mappingLayout);
        mappingLayout->addWidget(new QLabel("Mapping:"), 0, Qt::AlignLeft | Qt::AlignVCenter);
        spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
        mappingLayout->addWidget(spacerWidget);

        m_mappingTable = new QTableWidget();
        lowerLayout->addWidget(m_mappingTable);
        m_mappingTable->setAlternatingRowColors(true);
        m_mappingTable->setGridStyle(Qt::SolidLine);
        m_mappingTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_mappingTable->setSelectionMode(QAbstractItemView::SingleSelection);
        m_mappingTable->setCornerButtonEnabled(false);
        m_mappingTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_mappingTable->setContextMenuPolicy(Qt::DefaultContextMenu);
        m_mappingTable->setContentsMargins(3, 1, 3, 1);
        m_mappingTable->setColumnCount(2);
        m_mappingTable->setColumnWidth(0, m_mappingTable->width() / 2);
        m_mappingTable->setColumnWidth(1, m_mappingTable->width() / 2);
        verticalHeader = m_mappingTable->verticalHeader();
        verticalHeader->setVisible(false);

        // add the table headers
        headerItem = new QTableWidgetItem("Node");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_mappingTable->setHorizontalHeaderItem(0, headerItem);
        headerItem = new QTableWidgetItem("Mapped to");
        headerItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        m_mappingTable->setHorizontalHeaderItem(1, headerItem);
        m_mappingTable->horizontalHeader()->setStretchLastSection(true);
        m_mappingTable->horizontalHeader()->setSortIndicatorShown(false);
        m_mappingTable->horizontalHeader()->setSectionsClickable(false);
        connect(m_mappingTable, &QTableWidget::itemDoubleClicked, this, &MirrorSetupWindow::OnMappingTableDoubleClicked);
        connect(m_mappingTable, &QTableWidget::itemSelectionChanged, this, &MirrorSetupWindow::OnMappingTableSelectionChanged);
    }


    // destructor
    MirrorSetupWindow::~MirrorSetupWindow()
    {
        delete m_boneIcon;
        delete m_nodeIcon;
        delete m_meshIcon;
        delete m_mappedIcon;
    }


    // double clicked on an item in the table
    void MirrorSetupWindow::OnMappingTableDoubleClicked(QTableWidgetItem* item)
    {
        MCORE_UNUSED(item);
        // TODO: open a node hierarchy widget, where we can select a node from the m_sourceActor
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
        const AZStd::string nodeName = m_currentList->item(rowIndex, 2)->text().toUtf8().data();

        // find its index in the current actor, and remove its mapping
        EMotionFX::Node* node = currentActor->GetSkeleton()->FindNodeByName(nodeName.c_str());
        MCORE_ASSERT(node);

        // remove the mapping for this node
        PerformMapping(node->GetNodeIndex(), InvalidIndex);
    }


    // current list selection changed
    void MirrorSetupWindow::OnCurrentListSelectionChanged()
    {
        QList<QTableWidgetItem*> items = m_currentList->selectedItems();
        if (items.count() > 0)
        {
            //const uint32 currentListRow = items[0]->row();
            QTableWidgetItem* nameItem = m_currentList->item(items[0]->row(), 2);
            QList<QTableWidgetItem*> mappingTableItems = m_mappingTable->findItems(nameItem->text(), Qt::MatchExactly);

            for (int32 i = 0; i < mappingTableItems.count(); ++i)
            {
                if (mappingTableItems[i]->column() != 0)
                {
                    continue;
                }

                const uint32 rowIndex = mappingTableItems[i]->row();

                m_mappingTable->selectRow(rowIndex);
                m_mappingTable->setCurrentItem(mappingTableItems[i]);
            }
        }
    }


    // source list selection changed
    void MirrorSetupWindow::OnSourceListSelectionChanged()
    {
        QList<QTableWidgetItem*> items = m_sourceList->selectedItems();
        if (items.count() > 0)
        {
            //const uint32 currentListRow = items[0]->row();
            QTableWidgetItem* nameItem = m_sourceList->item(items[0]->row(), 2);
            QList<QTableWidgetItem*> mappingTableItems = m_mappingTable->findItems(nameItem->text(), Qt::MatchExactly);

            for (int32 i = 0; i < mappingTableItems.count(); ++i)
            {
                if (mappingTableItems[i]->column() != 1)
                {
                    continue;
                }

                const uint32 rowIndex = mappingTableItems[i]->row();

                m_mappingTable->selectRow(rowIndex);
                m_mappingTable->setCurrentItem(mappingTableItems[i]);
            }
        }
    }


    // selection changed in the table
    void MirrorSetupWindow::OnMappingTableSelectionChanged()
    {
        // select both items in the list widgets as well
        QList<QTableWidgetItem*> items = m_mappingTable->selectedItems();
        if (items.count() > 0)
        {
            const uint32 rowIndex = items[0]->row();

            QTableWidgetItem* item = m_mappingTable->item(rowIndex, 0);
            if (item)
            {
                QList<QTableWidgetItem*> listItems = m_currentList->findItems(item->text(), Qt::MatchExactly);
                if (listItems.count() > 0)
                {
                    m_currentList->selectRow(listItems[0]->row());
                    m_currentList->setCurrentItem(listItems[0]);
                }
            }

            item = m_mappingTable->item(rowIndex, 1);
            if (item)
            {
                QList<QTableWidgetItem*> listItems = m_sourceList->findItems(item->text(), Qt::MatchExactly);
                if (listItems.count() > 0)
                {
                    m_sourceList->selectRow(listItems[0]->row());
                    m_sourceList->setCurrentItem(listItems[0]);
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
            currentActor->ExtractBoneList(0, &m_currentBoneList);
        }

        // clear the node map
        if (reInitMap)
        {
            m_map.clear();
            if (currentActor)
            {
                const size_t numNodes = aznumeric_caster(currentActor->GetNumNodes());
                m_map.resize(numNodes);
                AZStd::fill(m_map.begin(), m_map.end(), InvalidIndex);
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
            m_currentList->setRowCount(0);
            return;
        }

        // fill the left list widget
        QString currentName;
        const size_t numNodes = actor->GetNumNodes();

        // count the number of rows
        int numRows = 0;
        for (size_t i = 0; i < numNodes; ++i)
        {
            currentName = actor->GetSkeleton()->GetNode(i)->GetName();
            if (currentName.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                numRows++;
            }
        }
        m_currentList->setRowCount(numRows);

        // fill the rows
        int rowIndex = 0;
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            currentName = node->GetName();
            if (currentName.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                // mark if there is a mapping or not
                const bool mapped = (m_map[node->GetNodeIndex()] != InvalidIndex);
                QTableWidgetItem* mappedItem = new QTableWidgetItem();
                mappedItem->setIcon(mapped ? *m_mappedIcon : QIcon());
                m_currentList->setItem(rowIndex, 0, mappedItem);

                // pick the right icon for the type column
                QTableWidgetItem* typeItem = new QTableWidgetItem();
                if (actor->GetMesh(0, node->GetNodeIndex()))
                {
                    typeItem->setIcon(*m_meshIcon);
                }
                else if (AZStd::find(begin(m_currentBoneList), end(m_currentBoneList), node->GetNodeIndex()) != end(m_currentBoneList))
                {
                    typeItem->setIcon(*m_boneIcon);
                }
                else
                {
                    typeItem->setIcon(*m_nodeIcon);
                }

                m_currentList->setItem(rowIndex, 1, typeItem);

                // set the name
                QTableWidgetItem* currentTableItem = new QTableWidgetItem(currentName);
                m_currentList->setItem(rowIndex, 2, currentTableItem);

                // set the row height and add one index
                m_currentList->setRowHeight(rowIndex, 21);
                ++rowIndex;
            }
        }
    }


    // fill the source list widget
    void MirrorSetupWindow::FillSourceListWidget(EMotionFX::Actor* actor, const QString& filterString)
    {
        if (!actor)
        {
            m_sourceList->setRowCount(0);
            return;
        }

        // fill the left list widget
        QString name;
        const size_t numNodes = actor->GetNumNodes();

        // count the number of rows
        int numRows = 0;
        for (size_t i = 0; i < numNodes; ++i)
        {
            name = actor->GetSkeleton()->GetNode(i)->GetName();
            if (name.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                numRows++;
            }
        }
        m_sourceList->setRowCount(numRows);

        // fill the rows
        int rowIndex = 0;
        for (size_t i = 0; i < numNodes; ++i)
        {
            EMotionFX::Node* node = actor->GetSkeleton()->GetNode(i);
            name = node->GetName();
            if (name.contains(filterString, Qt::CaseInsensitive) || filterString.isEmpty())
            {
                // mark if there is a mapping or not
                const bool mapped = AZStd::find(m_map.begin(), m_map.end(), i) != m_map.end();
                QTableWidgetItem* mappedItem = new QTableWidgetItem();
                mappedItem->setIcon(mapped ? *m_mappedIcon : QIcon());
                m_sourceList->setItem(rowIndex, 0, mappedItem);

                // pick the right icon for the type column
                QTableWidgetItem* typeItem = new QTableWidgetItem();
                if (actor->GetMesh(0, node->GetNodeIndex()))
                {
                    typeItem->setIcon(*m_meshIcon);
                }
                else if (AZStd::find(m_sourceBoneList.begin(), m_sourceBoneList.end(), node->GetNodeIndex()) != m_sourceBoneList.end())
                {
                    typeItem->setIcon(*m_boneIcon);
                }
                else
                {
                    typeItem->setIcon(*m_nodeIcon);
                }

                m_sourceList->setItem(rowIndex, 1, typeItem);

                // set the name
                QTableWidgetItem* currentTableItem = new QTableWidgetItem(name);
                m_sourceList->setItem(rowIndex, 2, currentTableItem);

                // set the row height and add one index
                m_sourceList->setRowHeight(rowIndex, 21);
                ++rowIndex;
            }
        }
    }


    // fill the mapping table
    void MirrorSetupWindow::FillMappingTable(EMotionFX::Actor* currentActor, EMotionFX::Actor* sourceActor)
    {
        if (!currentActor)
        {
            m_mappingTable->setRowCount(0);
            return;
        }

        // fill the table
        QString currentName;
        QString sourceName;
        const int numNodes = aznumeric_caster(currentActor->GetNumNodes());
        m_mappingTable->setRowCount(numNodes);
        for (int i = 0; i < numNodes; ++i)
        {
            currentName = currentActor->GetSkeleton()->GetNode(i)->GetName();

            QTableWidgetItem* currentTableItem = new QTableWidgetItem(currentName);
            m_mappingTable->setItem(i, 0, currentTableItem);
            m_mappingTable->setRowHeight(i, 21);

            if (m_map[i] != InvalidIndex)
            {
                sourceName = sourceActor->GetSkeleton()->GetNode(m_map[i])->GetName();
                currentTableItem = new QTableWidgetItem(sourceName);
                m_mappingTable->setItem(i, 1, currentTableItem);
            }
            else
            {
                m_mappingTable->setItem(i, 1, new QTableWidgetItem());
            }
        }
    }


    // pressing the link button
    void MirrorSetupWindow::OnLinkPressed()
    {
        if (m_currentList->currentRow() == -1 || m_sourceList->currentRow() == -1)
        {
            return;
        }

        // get the names
        QTableWidgetItem* curItem = m_currentList->currentItem();
        QTableWidgetItem* sourceItem = m_sourceList->currentItem();
        if (!curItem || !sourceItem)
        {
            return;
        }

        curItem = m_currentList->item(curItem->row(), 2);
        sourceItem = m_sourceList->item(sourceItem->row(), 2);

        const AZStd::string currentNodeName = curItem->text().toUtf8().data();
        const AZStd::string sourceNodeName  = m_sourceList->currentItem()->text().toUtf8().data();
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
    void MirrorSetupWindow::PerformMapping(size_t currentNodeIndex, size_t sourceNodeIndex)
    {
        EMotionFX::Actor* currentActor = GetSelectedActor();

        // update the map
        const size_t oldSourceIndex = m_map[currentNodeIndex];
        m_map[currentNodeIndex] = sourceNodeIndex;

        // update the current table
        const QString curName = currentActor->GetSkeleton()->GetNode(currentNodeIndex)->GetName();
        const QList<QTableWidgetItem*> currentListItems = m_currentList->findItems(curName, Qt::MatchExactly);
        for (int32 i = 0; i < currentListItems.count(); ++i)
        {
            const int rowIndex = currentListItems[i]->row();

            QTableWidgetItem* mappedItem = m_currentList->item(rowIndex, 0);
            if (!mappedItem)
            {
                mappedItem = new QTableWidgetItem();
                m_currentList->setItem(rowIndex, 0, mappedItem);
            }

            if (sourceNodeIndex == InvalidIndex)
            {
                mappedItem->setIcon(QIcon());
            }
            else
            {
                mappedItem->setIcon(*m_mappedIcon);
            }
        }

        // update source table
        if (sourceNodeIndex != InvalidIndex)
        {
            const bool stillUsed = AZStd::find(m_map.begin(), m_map.end(), sourceNodeIndex) != m_map.end();
            const QString sourceName = currentActor->GetSkeleton()->GetNode(sourceNodeIndex)->GetName();
            const QList<QTableWidgetItem*> sourceListItems = m_sourceList->findItems(sourceName, Qt::MatchExactly);
            for (int32 i = 0; i < sourceListItems.count(); ++i)
            {
                const int rowIndex = sourceListItems[i]->row();

                QTableWidgetItem* mappedItem = m_sourceList->item(rowIndex, 0);
                if (!mappedItem)
                {
                    mappedItem = new QTableWidgetItem();
                    m_sourceList->setItem(rowIndex, 0, mappedItem);
                }

                if (stillUsed == false)
                {
                    mappedItem->setIcon(QIcon());
                }
                else
                {
                    mappedItem->setIcon(*m_mappedIcon);
                }
            }
        }
        else // we're clearing it
        {
            if (oldSourceIndex != InvalidIndex)
            {
                const bool stillUsed = AZStd::find(m_map.begin(), m_map.end(), sourceNodeIndex) != m_map.end();
                const QString sourceName = currentActor->GetSkeleton()->GetNode(oldSourceIndex)->GetName();
                const QList<QTableWidgetItem*> sourceListItems = m_sourceList->findItems(sourceName, Qt::MatchExactly);
                for (int32 i = 0; i < sourceListItems.count(); ++i)
                {
                    const int rowIndex = sourceListItems[i]->row();

                    QTableWidgetItem* mappedItem = m_sourceList->item(rowIndex, 0);
                    if (!mappedItem)
                    {
                        mappedItem = new QTableWidgetItem();
                        m_sourceList->setItem(rowIndex, 0, mappedItem);
                    }

                    if (stillUsed == false)
                    {
                        mappedItem->setIcon(QIcon());
                    }
                    else
                    {
                        mappedItem->setIcon(*m_mappedIcon);
                    }
                }
            }
        }

        // update the mapping table
        QTableWidgetItem* item = m_mappingTable->item(aznumeric_caster(currentNodeIndex), 1);
        if (!item && sourceNodeIndex != InvalidIndex)
        {
            item = new QTableWidgetItem();
            m_mappingTable->setItem(aznumeric_caster(currentNodeIndex), 1, item);
        }

        if (sourceNodeIndex == InvalidIndex)
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
        QList<QTableWidgetItem*> items = m_currentList->selectedItems();
        if (items.count() > 0)
        {
            QTableWidgetItem* item = m_currentList->item(items[0]->row(), 0);
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
        const size_t numNodes = currentActor->GetNumNodes();
        AZStd::fill(m_map.begin(), AZStd::next(m_map.begin(), numNodes), InvalidIndex);

        // now apply the map we loaded to the data we have here
        const size_t numEntries = nodeMap->GetNumEntries();
        for (size_t i = 0; i < numEntries; ++i)
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
            m_map[currentNode->GetNodeIndex()] = sourceNode->GetNodeIndex();
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
        const size_t numNodes = currentActor->GetNumNodes();
        map->Reserve(numNodes);
        for (size_t i = 0; i < numNodes; ++i)
        {
            // skip unmapped entries
            if (m_map[i] == InvalidIndex)
            {
                continue;
            }

            // add the entry to the map if it doesn't yet exist
            if (map->GetHasEntry(currentActor->GetSkeleton()->GetNode(i)->GetName()) == false)
            {
                map->AddEntry(currentActor->GetSkeleton()->GetNode(i)->GetName(), currentActor->GetSkeleton()->GetNode(m_map[i])->GetName());
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

        const size_t numNodes = currentActor->GetNumNodes();
        return AZStd::all_of(m_map.begin(), AZStd::next(m_map.begin(), numNodes), [](const size_t nodeIndex)
        {
            return nodeIndex != InvalidIndex;
        });
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
        m_buttonOpen->setEnabled(canOpen);
        m_buttonSave->setEnabled(canSave);
        m_buttonClear->setEnabled(canClear);
        m_buttonGuess->setEnabled(canGuess);
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

        if (m_leftEdit->text().size() == 0 || m_rightEdit->text().size() == 0)
        {
            QMessageBox::information(this, "Empty Left And Right Strings", "Please enter both a left and right sub-string.\nThis can be something like 'Left' and 'Right'.\nThis would map nodes like 'Left Arm' to 'Right Arm' nodes.", QMessageBox::Ok);
            return;
        }

        // update the table and map
        uint32 numGuessed = 0;
        const size_t numNodes = currentActor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            // skip already setup mappings
            if (m_map[i] != InvalidIndex)
            {
                continue;
            }

            const uint16 matchIndex = currentActor->FindBestMatchForNode(currentActor->GetSkeleton()->GetNode(i)->GetName(), FromQtString(m_leftEdit->text()).c_str(), FromQtString(m_rightEdit->text()).c_str());
            if (matchIndex != MCORE_INVALIDINDEX16)
            {
                m_map[i] = matchIndex;
                numGuessed++;
            }
        }

        // update the actor
        UpdateActorMotionSources();

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

        const size_t numNodes = actor->GetNumNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            if (actor->GetHasMirrorInfo())
            {
                uint16 motionSource = actor->GetNodeMirrorInfo(i).m_sourceNode;
                if (motionSource != i)
                {
                    m_map[i] = motionSource;
                }
                else
                {
                    m_map[i] = InvalidIndex;
                }
            }
            else
            {
                m_map[i] = InvalidIndex;
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
        for (size_t i = 0; i < currentActor->GetNumNodes(); ++i)
        {
            size_t sourceNode = m_map[i];
            if (sourceNode != InvalidIndex && sourceNode != i)
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
