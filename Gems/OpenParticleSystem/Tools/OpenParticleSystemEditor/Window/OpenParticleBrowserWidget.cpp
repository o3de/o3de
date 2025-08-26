/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Material/MaterialSourceData.h>
#include <Atom/RPI.Edit/Material/MaterialTypeSourceData.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AzQtComponents/Utilities/DesktopUtilities.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzToolsFramework/AssetBrowser/Views/AssetBrowserTreeView.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Window/OpenParticleBrowserWidget.h>
#include <Window/ui_OpenParticleBrowserWidget.h>
#include <Window/OpenParticleSystemEditorWindowRequests.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QAction>
#include <QByteArray>
#include <QCursor>
#include <QDesktopServices>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QGraphicsOpacityEffect>
#include <QListWidget>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QLineEdit>
#include <QDir>
#include <QToolBar>
#include "WidgetItem.h"
AZ_POP_DISABLE_WARNING

namespace OpenParticleSystemEditor
{
    constexpr AZStd::string_view IMAGE_PARTICLE = ":/AssetEditor/default_document.svg";
    constexpr AZStd::string_view IMAGE_FOLDER = ":/stylesheet/img/UI20/Folder.svg";
    constexpr AZStd::string_view IMAGE_SEARCH_PATH = ":/stylesheet/img/search.svg";
    constexpr AZStd::string_view IMAGE_FILTER = ":/stylesheet/img/UI20/more.svg";
    constexpr AZStd::string_view PARTICLE = "Particle";
    constexpr AZStd::string_view PARTICLES = "Particles";
    constexpr AZStd::string_view PARTICLE_EXTENSION = "particle";
    constexpr AZStd::string_view ICON_IMPORT_NORMAL = ":/SpinBox/arrowRightMaxReached.svg";
    constexpr AZStd::string_view ICON_IMPORT_HOVER = ":/stylesheet/img/UI20/SpinBox/decrease-rightarrow.svg";
    constexpr AZStd::string_view ICON_SAVEALL_NORMAL = ":/Gallery/Save.svg";
    constexpr AZStd::string_view ICON_SAVEALL_HOVER = ":/Gallery/Save.svg";
    constexpr AZStd::string_view ICON_EXPAND_NORMAL = ":/stylesheet/img/UI20/toolbar/Scale.svg";
    constexpr AZStd::string_view ICON_EXPAND_HOVER = ":/stylesheet/img/UI20/toolbar/Scale.svg";
    constexpr AZStd::string_view ICON_HIDE_NORMAL = ":/stylesheet/img/UI20/toolbar/Object_list.svg";
    constexpr AZStd::string_view ICON_HIDE_HOVER = ":/stylesheet/img/UI20/toolbar/Object_list.svg";
    constexpr AZStd::string_view ICON_FILTER_DIR = ":/stylesheet/img/UI20/Grid-large.svg";
    constexpr AZStd::string_view ICON_FILTER_PARTICLE = ":/Gallery/Grid-small.svg";
    [[maybe_unused]] constexpr AZStd::string_view ICON_IMPORT_12PX = ":/stylesheet/img/UI20/TreeView/open.svg";
    constexpr AZStd::string_view ICON_SAVEALL_12PX = ":/Gallery/Save.svg";

    NodeBrowserTreeDelegate::NodeBrowserTreeDelegate(QWidget* parent)
        : EntryDelegate(parent)
    {
    }

    void NodeBrowserTreeDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        if (index.column() == aznumeric_cast<int>(AssetBrowserEntry::Column::Name))
        {
            painter->save();

#if defined(QT_VERSION) && defined(QT_VERSION_CHECK) && (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
            QStyleOptionViewItemV4 options = option;
#else
            QStyleOptionViewItem options = option;
#endif
            initStyleOption(&options, index);

            // paint the original node item
            EntryDelegate::paint(painter, option, index);

            const int textMargin = options.widget->style()->pixelMetric(QStyle::PM_FocusFrameHMargin, 0, options.widget) + 1;
            QRect textRect = options.widget->style()->subElementRect(QStyle::SE_ItemViewItemText, &options);
            textRect = textRect.adjusted(textMargin, 0, -textMargin, 0);

            QModelIndex sourceIndex = static_cast<const AssetBrowserFilterModel*>(index.model())->mapToSource(index);
            AssetBrowserEntry* pEutry = static_cast<AssetBrowserEntry*>(sourceIndex.internalPointer());

            QString test = index.model()->data(index).toString();
            int regexIndex = m_filterRegex.indexIn(test);

            if (pEutry && regexIndex >= 0)
            {
                // pos, len
                const AZStd::pair<int, int> highlight(regexIndex, m_filterRegex.matchedLength());
                QString preSelectedText = options.text.left(highlight.first);
                int preSelectedTextLength = options.fontMetrics.horizontalAdvance(preSelectedText);
                QString selectedText = options.text.mid(highlight.first, highlight.second);
                int selectedTextLength = options.fontMetrics.horizontalAdvance(selectedText);

                int leftSpot = textRect.left() + preSelectedTextLength;

                // Only need to do the draw if we actually are doing to be highlighting the text.
                if (leftSpot < textRect.right())
                {
                    int visibleLength = AZStd::GetMin(selectedTextLength, textRect.right() - leftSpot);
                    int rectLeft = textRect.left() + preSelectedTextLength + m_iconSize + m_paddingOffset;
                    QRect highlightRect(rectLeft, textRect.top(), visibleLength, textRect.height());

                    // paint the highlight rect
                    painter->fillRect(highlightRect, options.palette.highlight());
                    painter->setPen(QPen(Qt::white));
                    painter->drawText(highlightRect, Qt::AlignCenter, selectedText);
                }
            }

            painter->restore();
        }
        else
        {
            EntryDelegate::paint(painter, option, index);
        }
    }

    void NodeBrowserTreeDelegate::SetStringFilter(const QString& filter)
    {
        m_filterString = QRegExp::escape(filter.simplified().replace(" ", ""));

        QString regExIgnoreWhitespace(m_filterString[0]);
        for (int i = 1; i < m_filterString.size(); ++i)
        {
            regExIgnoreWhitespace.append("\\s*");
            regExIgnoreWhitespace.append(m_filterString[i]);
        }

        m_filterRegex = QRegExp(regExIgnoreWhitespace, Qt::CaseInsensitive);
    }

    ParticleBrowserWidget::ParticleBrowserWidget(QWidget* parent)
        : QWidget(parent)
        , m_ui(new Ui::ParticleBrowserWidget)
    {
        using namespace AzToolsFramework::AssetBrowser;

        m_ui->setupUi(this);
        InitUI();
        InitIcon();

        // Get the asset browser model
        AssetBrowserModel* assetBrowserModel = nullptr;
        AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);
        AZ_Assert(assetBrowserModel, "Failed to get file browser model");

        AssetSelectionModel selectionModel = AssetSelectionModel::EverythingSelection();
        EntryTypeFilter* foldersFilter = new EntryTypeFilter();
        foldersFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);
        selectionModel.SetSelectionFilter(FilterConstType(foldersFilter));
        selectionModel.SetDisplayFilter(selectionModel.GetSelectionFilter());
        m_ui->m_searchWidget->GetFilter()->AddFilter(selectionModel.GetDisplayFilter());
        QString name = selectionModel.GetDisplayFilter()->GetName();

        AssetGroupFilter* groupFilter = new AssetGroupFilter();
        groupFilter->SetAssetGroup(PARTICLE.data());
        m_ui->m_searchWidget->GetTypesFilter()->AddFilter(FilterConstType(groupFilter));

        // Hook up the data set to the tree view
        m_filterModel = aznew AssetBrowserFilterModel(this);
        m_filterModel->setSourceModel(assetBrowserModel);
        m_filterModel->SetFilter(CreateFilter());

        m_ui->m_assetBrowserTreeViewWidget->setModel(m_filterModel);
        m_ui->m_assetBrowserTreeViewWidget->SetShowSourceControlIcons(false);

        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SetName("AssetBrowserTreeView_" + name);

        m_treeDelegate = aznew NodeBrowserTreeDelegate(this);
        connect(m_ui->m_searchWidget, &AzToolsFramework::AssetBrowser::SearchWidget::TextFilterChanged, m_treeDelegate, &NodeBrowserTreeDelegate::SetStringFilter);
        m_ui->m_assetBrowserTreeViewWidget->setItemDelegate(m_treeDelegate);
        connect(m_treeDelegate, &EntryDelegate::RenameEntry,  m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::AfterRename);

        disconnect(m_ui->m_assetBrowserTreeViewWidget, &QTreeView::customContextMenuRequested, m_ui->m_assetBrowserTreeViewWidget, 0);
        connect(m_ui->m_assetBrowserTreeViewWidget, &QTreeView::customContextMenuRequested, m_ui->m_assetBrowserTreeViewWidget, [](const QPoint&){});

        ConnectSignal();
        AddNewMenu();
        LoadAllParticles();
    }

    void ParticleBrowserWidget::ConnectSignal()
    {
        connect(m_ui->m_searchWidget->GetFilter().data(), &AssetBrowserEntryFilter::updatedSignal, m_filterModel, &AssetBrowserFilterModel::filterUpdatedSlot);
        connect(m_ui->m_assetBrowserTreeViewWidget, &AssetBrowserTreeView::selectionChangedSignal, this, &ParticleBrowserWidget::OnSelectedChanged);
        connect(m_ui->hideTreeBtn, &QToolButton::pressed, this, &ParticleBrowserWidget::HideTreeView);
        connect(m_ui->showTreeTbtn, &QToolButton::pressed, this, &ParticleBrowserWidget::HideTreeView);
        connect(m_ui->m_searchPathWidget, &AzToolsFramework::AssetBrowser::SearchWidget::TextFilterChanged, this, &ParticleBrowserWidget::OnTextPathChanged);
        connect(m_ui->listWidget, &QListWidget::itemDoubleClicked, this, &ParticleBrowserWidget::OnItemDoubleClicked);
        connect(m_ui->listWidget, &QListWidget::customContextMenuRequested, this, &ParticleBrowserWidget::AddNewSlot);
        connect(m_ui->filterParticleBtn, &QToolButton::pressed, this, &ParticleBrowserWidget::ShowParticlePath);
        connect(m_ui->saveAllBtn, &QToolButton::pressed, this, []()
        {
            EBUS_EVENT(OpenParticleSystemEditorWindowRequestsBus, SaveDocument);
        });
        connect(m_filterModel, &AssetBrowserFilterModel::filterChanged, this, [this]()
        {
            const bool hasFilter = !m_ui->m_searchWidget->GetFilterString().isEmpty();
            constexpr bool selectFirstFilteredIndex = true;
            if (m_ui->m_assetBrowserTreeViewWidget->IsTreeViewSavingReady())
            {
                m_ui->m_assetBrowserTreeViewWidget->UpdateAfterFilter(hasFilter, selectFirstFilteredIndex);
            }
            if (hasFilter)
            {
                m_ui->m_assetBrowserTreeViewWidget->expandAll();
            }
            else
            {
                m_ui->m_assetBrowserTreeViewWidget->collapseAll();
                InitParticlesList();
            }
        });
        connect(m_ui->listWidget, &QListWidget::itemSelectionChanged, this, [this]()
        {
            QList<QListWidgetItem*> listSelectedItems = m_ui->listWidget->selectedItems();
            m_ui->label->setText(QString::fromUtf8("%1 items(%2 selected)").arg(m_ui->listWidget->count()).arg(listSelectedItems.size()));
            for (int i = 0; i < m_ui->listWidget->count(); i++)
            {
                QListWidgetItem* item = m_ui->listWidget->item(i);
                WidgetItem* widgetItem = (WidgetItem*)(m_ui->listWidget->itemWidget(item));
                if (item->isSelected())
                {
                    widgetItem->OnSelected();
                }
                else
                {
                    widgetItem->OnRelease();
                    widgetItem->m_lineEdit->setStyleSheet("background:rgb(68,68,68);color:white");
                }
            }
        });
        connect(m_ui->listWidget, &QListWidget::itemClicked, this, [this](QListWidgetItem* item)
        {
            WidgetItem* widgetItem = (WidgetItem*)(m_ui->listWidget->itemWidget(item));
            widgetItem->m_lineEdit->setEnabled(true);
            widgetItem->SetFocusFlag(true);
            widgetItem->m_lineEdit->setStyleSheet("background:transparent;color:white");
        });
    }

    ParticleBrowserWidget::~ParticleBrowserWidget()
    {
        // Maintains the tree expansion state between runs
        m_ui->m_assetBrowserTreeViewWidget->SaveState();
        AZ::TickBus::Handler::BusDisconnect();
    }

    void ParticleBrowserWidget::InitUI()
    {
        m_ui->m_searchWidget->Setup(true, true);
        m_ui->m_searchWidget->SetTypeFilterVisible(false);
        m_ui->m_searchWidget->SetFilteredParentVisible(false);
        m_ui->m_searchWidget->setMinimumSize(QSize(100, 0));
        m_ui->m_splitter->setSizes(QList<int>() << 300 << 100);
        m_ui->m_splitter->setStretchFactor(1, 1);
        m_ui->splitter->setStretchFactor(1, 1);
        m_ui->splitterView->setStretchFactor(1, 1);
        m_ui->splitter_4->setStretchFactor(1, 1);
        QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect;
        opacityEffect->setOpacity(0);
        m_ui->splitterView->handle(1)->setGraphicsEffect(opacityEffect);
        m_ui->hideTreeBtn->setFixedWidth(30);
        m_ui->hideTreeBtn->setIconSize(QSize(22, 22));
        m_ui->filterParticleBtn->setFixedWidth(30);
        m_ui->filterParticleBtn->setIcon(QIcon(IMAGE_FILTER.data()));
        m_ui->filterParticleBtn->setIconSize(QSize(22, 22));
        m_ui->m_searchPathWidget->setMinimumSize(QSize(100, 0));
        m_ui->listWidget->setIconSize(QSize(65, 65));
        m_ui->listWidget->setResizeMode(QListWidget::Adjust);
        m_ui->listWidget->setMovement(QListWidget::Static);
        m_ui->listWidget->setSpacing(20);
        m_ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_ui->showTreeTbtn->setFixedWidth(30);
        m_ui->showTreeTbtn->setIcon(QIcon(IMAGE_SEARCH_PATH.data()));
        m_ui->showTreeTbtn->setIconSize(QSize(22, 22));
        m_ui->horizontalLayout->setContentsMargins(0, 0, 0, 0);
        m_ui->horizontalLayout_3->setContentsMargins(0, 2, 0, 0);
        m_ui->showTreeTbtn->hide();
        m_ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        PortalItemDelegate* delegate = new PortalItemDelegate(this);
        m_ui->listWidget->setItemDelegate(delegate);
        m_ui->saveAllBtn->installEventFilter(this);
        m_ui->importBtn->installEventFilter(this);
        m_ui->hideTreeBtn->installEventFilter(this);
        m_ui->showTreeTbtn->installEventFilter(this);
        m_ui->filterParticleBtn->installEventFilter(this);
    }

    void ParticleBrowserWidget::InitIcon()
    {
        m_ui->importBtn->setIconSize(QSize(56, 18));
        m_ui->importBtn->setIcon(QIcon(ICON_IMPORT_NORMAL.data()));
        m_ui->saveAllBtn->setIconSize(QSize(64, 18));
        m_ui->saveAllBtn->setIcon(QIcon(ICON_SAVEALL_NORMAL.data()));
        m_ui->hideTreeBtn->setIcon(QIcon(ICON_EXPAND_NORMAL.data()));

        m_ui->hideTreeBtn->setToolTip(tr("Hide the folder panel"));
        m_ui->showTreeTbtn->setToolTip(tr("Show the folder panel"));
        m_ui->filterParticleBtn->setToolTip(tr("Filter particle files"));

        m_mapIcon = {
            { m_ui->importBtn, { ICON_IMPORT_NORMAL, ICON_IMPORT_HOVER} },
            { m_ui->saveAllBtn, { ICON_SAVEALL_NORMAL, ICON_SAVEALL_HOVER } },
            { m_ui->hideTreeBtn, { ICON_EXPAND_NORMAL, ICON_EXPAND_HOVER } },
            { m_ui->showTreeTbtn, { ICON_HIDE_NORMAL, ICON_HIDE_HOVER } },
            { m_ui->filterParticleBtn, { ICON_FILTER_DIR, ICON_FILTER_PARTICLE } }
        };
        QToolBar* toolbar = new QToolBar;
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
#if defined(O3DE_REV_BUILD_RELEASE) && (O3DE_REV_BUILD_RELEASE)
        toolbar->addAction(QIcon(ICON_IMPORT_12PX.data()), tr("Import"));
#endif
        toolbar->addAction(QIcon(ICON_SAVEALL_12PX.data()), tr("Save All"), this, []()
        {
            EBUS_EVENT(OpenParticleSystemEditorWindowRequestsBus, SaveDocument);
        });
        m_ui->importBtn->hide();
        m_ui->saveAllBtn->hide();
        m_ui->horizontalLayout_2->insertSpacing(1, 10);
        m_ui->horizontalLayout_2->insertWidget(2, toolbar, 0, Qt::AlignLeft);
        toolbar->setStyleSheet("QToolBar QToolButton:hover{color:rgb(30,112,235);}");
    }

    bool ParticleBrowserWidget::eventFilter(QObject* obj, QEvent* event)
    {
        auto iter = m_mapIcon.begin();
        for (; iter!= m_mapIcon.end(); ++iter)
        {
            if (iter->first == obj)
            {
                break;
            }
        }
        if (iter != m_mapIcon.end())
        {
            QToolButton* toolButton = static_cast<QToolButton*>(obj);
            if (event->type() == QEvent::Enter)
            {
                if (!iter->second[MOUSE_ENTER].empty())
                {
                    toolButton->setIcon(QIcon(iter->second[MOUSE_ENTER].c_str()));
                }
            }
            else if (event->type() == QEvent::Leave)
            {
                if (!iter->second[MOUSE_LEAVE].empty())
                {
                    toolButton->setIcon(QIcon(iter->second[MOUSE_LEAVE].c_str()));
                }
            }
        }
        return QWidget::eventFilter(obj, event);
    }

    void ParticleBrowserWidget::OnSelectedChanged()
    {
        const auto& selectedAssets = m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets();
        m_ui->listWidget->clear();
        if (!selectedAssets.empty())
        {
            const AssetBrowserEntry* curEntry = selectedAssets.front();
            if (curEntry == nullptr)
            {
                return;
            }
            int size = curEntry->GetChildCount();
            for (int i = 0; i < size; i++)
            {
                const AssetBrowserEntry* pEntry = curEntry->GetChild(i);
                auto type = pEntry->GetEntryType(); // == AssetBrowserEntry::AssetEntryType::Source;
                QString displayName = QString::fromUtf8(pEntry->GetName().c_str());
                QString fullPath = QString::fromUtf8(pEntry->GetFullPath().c_str());
                QString relativePath = QString::fromUtf8(pEntry->GetRelativePath().c_str());
                QString iconPath;
                QString fileName;
                if (type == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    iconPath = IMAGE_FOLDER.data();
                }
                else
                {
                    if (pEntry->GetDisplayName().endsWith(PARTICLE_EXTENSION.data()))
                    {
                        iconPath = IMAGE_PARTICLE.data();
                        fileName = displayName;
                        displayName = displayName.left(displayName.lastIndexOf("."));
                    }
                    else
                    {
                        continue;  // other file undisplay
                    }
                }
                AddListItem(displayName, fileName, fullPath, iconPath, relativePath);
            }
        }
    }

    void ParticleBrowserWidget::AddListItem(const QString& displayName, const QString& fileName, const QString& fullPath, const QString &iconPath,
                                            const QString& relativePath)
    {
        QListWidgetItem* pItem = new QListWidgetItem(m_ui->listWidget);
        pItem->setSizeHint(QSize(66, 95));
        pItem->setToolTip(fileName.isEmpty() ? displayName : fileName);
        pItem->setData(Qt::UserRole, fullPath);
        pItem->setData(Qt::AccessibleTextRole, relativePath);
        pItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
        WidgetItem* widgetItem = new WidgetItem();
        widgetItem->Init(displayName, iconPath, pItem);
        pItem->setText(displayName);
        connect(widgetItem, &WidgetItem::editingFinished, this, &ParticleBrowserWidget::OnItemRename);
        m_ui->listWidget->addItem(pItem);
        m_ui->listWidget->setItemWidget(pItem, widgetItem);
    }

    void ParticleBrowserWidget::ShowParticlePath()
    {
        static bool showOnly = true;
        if (showOnly)
        {
            showOnly = false;
            m_ui->m_searchWidget->ClearTypeFilter();
            m_ui->m_searchWidget->SetFilterState("", PARTICLE.data(), true);
            m_ui->m_searchWidget->GetTypesFilter()->RemoveAllFilters();
            AssetGroupFilter* groupFilter = new AssetGroupFilter();
            groupFilter->SetAssetGroup(PARTICLE.data());
            m_ui->m_searchWidget->GetTypesFilter()->AddFilter(FilterConstType(groupFilter));
            m_ui->filterParticleBtn->setIcon(QIcon(ICON_FILTER_PARTICLE.data()));
            m_mapIcon[m_ui->filterParticleBtn] = { ICON_FILTER_PARTICLE, ICON_FILTER_DIR };
        }
        else
        {
            showOnly = true;
            m_ui->m_searchWidget->ClearTypeFilter();
            m_ui->m_searchWidget->SetFilterState("", PARTICLE.data(), false);
            m_ui->m_searchWidget->GetTypesFilter()->RemoveAllFilters();
            m_ui->filterParticleBtn->setIcon(QIcon(ICON_FILTER_DIR.data()));
            m_mapIcon[m_ui->filterParticleBtn] = { ICON_FILTER_DIR, ICON_FILTER_PARTICLE };
        }
    }

    QString ParticleBrowserWidget::GetCurrentPath()
    {
        QModelIndex curIndex = m_ui->m_assetBrowserTreeViewWidget->currentIndex();
        const AssetBrowserEntry* rowEntry = m_ui->m_assetBrowserTreeViewWidget->GetEntryFromIndex<AssetBrowserEntry>(curIndex);
        if (rowEntry == nullptr)
        {
            return "";
        }
        QString fullPath = rowEntry->GetFullPath().c_str();
        return fullPath;
    }

    void ParticleBrowserWidget::CreateFolderWindow(const QString& currentPath)
    {
        QDialog* dialog = new QDialog(this);
        dialog->setWindowTitle(tr("Create Folder"));
        QLabel* labelFolder = new QLabel(tr("Enter Folder Name"), this);
        QLineEdit* lineEdit = new QLineEdit(dialog);
        lineEdit->setMinimumWidth(280);
        QVBoxLayout* vLayout = new QVBoxLayout(dialog);
        vLayout->addWidget(labelFolder);
        vLayout->addWidget(lineEdit);
        QHBoxLayout* hLayout = new QHBoxLayout();
        QSpacerItem* spacerFront = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        hLayout->addItem(spacerFront);
        QPushButton* buttonOk = new QPushButton(tr("OK"), dialog);
        QPushButton* buttonCancel = new QPushButton(tr("Cancel"), dialog);
        connect(buttonCancel, &QPushButton::pressed, [dialog](){ dialog->close(); });
        connect(buttonOk, &QPushButton::pressed, [=, this]()
        {
            if (lineEdit->text().trimmed().isEmpty() || CheckName(lineEdit->text().trimmed(), true))
            {
                return;
            }
            QDir* folder = new QDir;
            folder->cd(currentPath);
            bool exist = folder->exists(lineEdit->text().trimmed());
            if (exist)
            {
                QMessageBox::warning(this, tr("Warning"), tr("Folder already exists!"));
            }
            else
            {
                bool bSuccess = folder->mkdir(lineEdit->text());
                if (!bSuccess)
                {
                    QMessageBox::warning(this, tr("Warning"), tr("Failed to create the folder!"));
                }
            }
            dialog->accept();
        });
        hLayout->addWidget(buttonOk);
        hLayout->addWidget(buttonCancel);
        vLayout->addLayout(hLayout);
        QRect rect = this->geometry();
        dialog->move(rect.width() / 2, rect.height() / 2);
        if (dialog->exec() == QDialog::Accepted)
        {
            const int delayTime = 200;
            QTimer::singleShot(delayTime, this, [this]()
            {
                m_ui->m_assetBrowserTreeViewWidget->model()->layoutChanged();
                OnSelectedChanged();
            });
        }
    }

    void ParticleBrowserWidget::CreateParticleWindow(const QString& currentPath)
    {
        QDialog* dialog = new QDialog(this);
        dialog->setWindowTitle(tr("Create Particle System"));
        QLabel* labelPath = new QLabel(tr("Path"));
        QLineEdit* lineEdit = new QLineEdit(currentPath, dialog);
        lineEdit->setReadOnly(true);
        lineEdit->setFocusPolicy(Qt::NoFocus);
        QLabel* labelFileName = new QLabel(tr("Enter File Name"), dialog);
        QLineEdit* editFileName = new QLineEdit(dialog);
        AZStd::string currentpath = currentPath.toStdString().c_str();
        const auto fileInfo = QFileInfo(AtomToolsFramework::GetUniqueFilePath(currentpath + AZ_CORRECT_FILESYSTEM_SEPARATOR + "untitled.particle").c_str());
        editFileName->setText(fileInfo.fileName());
        editFileName->setMinimumWidth(390);
        QVBoxLayout* vLayout = new QVBoxLayout(dialog);
        vLayout->addWidget(labelPath);
        vLayout->addWidget(lineEdit);
        vLayout->addWidget(labelFileName);
        vLayout->addWidget(editFileName);
        QHBoxLayout* hLayout = new QHBoxLayout();
        QSpacerItem* spacerFront = new QSpacerItem(20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        hLayout->addItem(spacerFront);
        QPushButton* buttonOk = new QPushButton(tr("OK"), dialog);
        QPushButton* buttonCancel = new QPushButton(tr("Cancel"), dialog);
        hLayout->addWidget(buttonOk);
        hLayout->addWidget(buttonCancel);
        vLayout->addLayout(hLayout);
        connect(buttonCancel, &QPushButton::pressed, [dialog](){ dialog->close(); });
        connect(buttonOk, &QPushButton::pressed, [=, this]()
        {
            QFileInfo particleFile;
            particleFile.setFile(currentPath, editFileName->text());
            if (!CheckParticleFileName(particleFile, editFileName->text()))
            {
                return;
            }
            QString fullName = particleFile.absoluteFilePath();
            if (AzFramework::StringFunc::Path::IsExtension(fullName.toUtf8().data(), PARTICLE_EXTENSION.data()))
            {
                EBUS_EVENT(OpenParticleSystemEditorWindowRequestsBus, CreateParticleFile, fullName.toStdString().c_str());
            }
            dialog->accept();
        });
        QRect rect = this->geometry();
        dialog->setGeometry(rect.width() / 2, rect.height() / 2, 400, 155);
        if (dialog->exec() == QDialog::Accepted)
        {
            QString fullPathName = currentPath + QDir::separator();
            QString fileName = editFileName->text().trimmed();
            fullPathName += fileName;
            QString displayName = fileName.left(fileName.indexOf("."));
            AddListItem(displayName, fileName, fullPathName, IMAGE_PARTICLE.data());
            if (!m_particlesPath.contains(fullPathName))
            {
                m_particlesPath.push_back(fullPathName);
            }
        }
    }

    void ParticleBrowserWidget::CreateFolderSlot()
    {
        QString currentPath = GetCurrentPath();
        CreateFolderWindow(currentPath);
    }

    void ParticleBrowserWidget::CreateNewParticle()
    {
        QString currentPath = GetCurrentPath();
        CreateParticleWindow(currentPath);
    }

    bool ParticleBrowserWidget::CheckName(const QString& name, bool bIsFolder)
    {
        // Check if the filename has any invalid characters
        QRegularExpression validFileNameRegex(QString::fromUtf8(reinterpret_cast<const char*>(u8"[\u4e00-\u9fa5a-zA-Z0-9_-]*$")));
        QRegularExpressionMatch validFileNameMatch = validFileNameRegex.match(name);
        // If the filename had invalid characters, then show a warning message
        QRegularExpression fileNameWithSymbolsRegex("(?![_-])[\\p{P}\\p{S}]");
        QRegularExpressionMatch fileNameWithSymbolsMatch = fileNameWithSymbolsRegex.match(name);
        bool bFileNameMatch = validFileNameMatch.hasMatch();
        bool bFileNameWithSymbolsMatch = fileNameWithSymbolsMatch.hasMatch();
        bool bResult = (!bFileNameMatch || bFileNameWithSymbolsMatch);
        if (bIsFolder && bResult)
        {
            QMessageBox::warning(
                this, QObject::tr("Warning"),
                QObject::tr("Particle assets folder are restricted to alphanumeric characters, Chinese characters, hyphens (-), underscores (_), and dots (.)."));
        }
        return bResult;
    }

    bool ParticleBrowserWidget::CheckParticleFileName(const QFileInfo &fileInfo, const QString &text)
    {
        AZStd::string absoluteFilePath = fileInfo.absoluteFilePath().toStdString().c_str();
        AZStd::string completeBaseName = fileInfo.completeBaseName().toStdString().c_str();
        AZStd::string fileName = text.toStdString().c_str();
        if (!AzFramework::StringFunc::Path::Normalize(absoluteFilePath) ||
            !AzFramework::StringFunc::Path::IsValid(absoluteFilePath.c_str()) ||
            (completeBaseName.find_last_of(AZ_FILESYSTEM_EXTENSION_SEPARATOR) != AZStd::string::npos) ||
            (completeBaseName.contains(' ')) || (completeBaseName.empty()) ||
            (fileName.find(AZ_FILESYSTEM_DRIVE_SEPARATOR) != AZStd::string::npos) ||
            (fileName.find(AZ_CORRECT_FILESYSTEM_SEPARATOR) != AZStd::string::npos) ||
            (fileName.find(AZ_WRONG_FILESYSTEM_SEPARATOR) != AZStd::string::npos) || CheckName(completeBaseName.c_str()))
        {
            if (!AzFramework::StringFunc::Path::IsExtension(absoluteFilePath.c_str(), PARTICLE_EXTENSION.data()) &&
                !fileInfo.isDir())
            {
                QMessageBox::warning(this, QObject::tr("Warning"),
                    QObject::tr(
                        "Particle assets are restricted to alphanumeric characters, Chinese characters, hyphens (-), underscores (_), and dots (.)."));
                return false;
            }
        }

        if (fileInfo.exists())
        {
            QMessageBox::warning(this, QObject::tr("Warning"),
                QObject::tr("The file name is existed, Please override it"));
            return false;
        }
        return true;
    }

    void ParticleBrowserWidget::OnItemRename(QLineEdit* lineEdit, QListWidgetItem* item)
    {
        WidgetItem* widgetItem = (WidgetItem*)(m_ui->listWidget->itemWidget(item));
        widgetItem->SetFocusFlag(false);

        QString oldName = item->text();
        QString oldFullPath = item->data(Qt::UserRole).toString();
        QFileInfo oldFileInfo(oldFullPath);

        QString newName = lineEdit->text();
        QString newFullPath = oldFileInfo.absolutePath() + "/" + newName;
        if (!oldFileInfo.isDir())
        {
            newFullPath = newFullPath + "." + QString(PARTICLE_EXTENSION.data());
        }
        
        QFileInfo newFileInfo(newFullPath); 

        if (oldName.compare(newName) == 0 || !CheckParticleFileName(newFileInfo, newName))
        {
            lineEdit->setEnabled(false);
            lineEdit->setText(oldName);
            return;
        }

        QDir dir(oldFileInfo.absolutePath());
        if (!dir.rename(oldFullPath, newFullPath))
        {
            lineEdit->setText(oldName);
            lineEdit->setEnabled(false);
            QMessageBox::information(this, tr("Warning"), tr("Failed to rename the file/folder!"));
            return;
        }

        item->setText(newName);
        lineEdit->setEnabled(false);
        item->setData(Qt::UserRole, newFullPath);
        widgetItem->setToolTip(newFileInfo.fileName());
    }

    void ParticleBrowserWidget::OnItemDoubleClicked(QListWidgetItem* item)
    {
        // open particle file
        QString fullPath = item->data(Qt::UserRole).toString();
        AZ::IO::FileIOBase* fileIOBase = AZ::IO::FileIOBase::GetInstance();
        bool isDir = fileIOBase->IsDirectory(fullPath.toUtf8().data());

        if ((!isDir) && AzFramework::StringFunc::Path::IsExtension(fullPath.toUtf8().data(), PARTICLE_EXTENSION.data()))
        {
            EBUS_EVENT(OpenParticleSystemEditorWindowRequestsBus, OpenDocument, fullPath.toStdString().c_str());
        }
        else if (isDir)
        {
            AZStd::string relativePath  = item->data(Qt::AccessibleTextRole).toString().toUtf8().data();
            AzFramework::StringFunc::Replace(relativePath, '\\', '/');
            m_ui->m_assetBrowserTreeViewWidget->SelectFolder(relativePath);
        }
        else
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fullPath.toUtf8().data()));
        }
    }

    AzToolsFramework::AssetBrowser::FilterConstType ParticleBrowserWidget::CreateFilter() const
    {
        using namespace AzToolsFramework::AssetBrowser;

        QSharedPointer<EntryTypeFilter> sourceFilter(new EntryTypeFilter);
        sourceFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Source);

        QSharedPointer<EntryTypeFilter> folderFilter(new EntryTypeFilter);
        folderFilter->SetEntryType(AssetBrowserEntry::AssetEntryType::Folder);

        QSharedPointer<CompositeFilter> sourceOrFolderFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::OR));
        sourceOrFolderFilter->AddFilter(sourceFilter);
        sourceOrFolderFilter->AddFilter(folderFilter);

        QSharedPointer<CompositeFilter> finalFilter(new CompositeFilter(CompositeFilter::LogicOperatorType::AND));
        finalFilter->AddFilter(sourceOrFolderFilter);
        finalFilter->AddFilter(m_ui->m_searchWidget->GetFilter());

        return finalFilter;
    }

    void ParticleBrowserWidget::OnTextPathChanged(const QString& text)
    {
        if (text.trimmed().isEmpty())
        {
            OnSelectedChanged();
            return;
        }
        m_ui->listWidget->clear();
        bool searchName = (text.indexOf(".") == -1) ? true : false;
        for (int i = 0; i < m_particlesPath.size(); i++)
        {
            QString fullPath = m_particlesPath.at(i);
            QFileInfo fileInfo(fullPath);
            QString displayName = fileInfo.fileName();
            QString fileName = displayName.left(displayName.lastIndexOf("."));
            if (searchName)
            {
                if (fileName.contains(text.trimmed(), Qt::CaseInsensitive))
                {
                    AddListItem(fileName, displayName, fullPath, IMAGE_PARTICLE.data());
                }
            }
            else
            {
                QString searchKey = "." + QString(PARTICLE_EXTENSION.data());
                if (text.trimmed().compare(searchKey, Qt::CaseInsensitive) == 0 || text.trimmed().compare(displayName, Qt::CaseInsensitive) == 0)
                {
                    AddListItem(fileName, displayName, fullPath, IMAGE_PARTICLE.data());
                }
            }
        }
    }

    void ParticleBrowserWidget::HideTreeView()
    {
        if (!m_ui->splitter->isHidden())
        {
            m_ui->splitter->hide();
            m_ui->showTreeTbtn->show();
        }
        else
        {
            m_ui->splitter->show();
            m_ui->showTreeTbtn->hide();
        }
    }

    void ParticleBrowserWidget::InitParticlesList()
    {
        QSet<QString> expandedElements;
        auto treeView = m_ui->m_assetBrowserTreeViewWidget;
        if (!treeView->IsTreeViewSavingReady())
        {
            return;
        }
        treeView->WriteTreeViewStateTo(expandedElements);
        QString sAssetPath;
        for (auto& dir : expandedElements)
        {
            if (dir.endsWith("Assets"))
            {
                sAssetPath = dir;
                break;
            }
        }
        sAssetPath = "Gems/OpenParticleSystem/Assets";
        QStringList dirList = sAssetPath.split("/");
        QList<QModelIndex> modelList;
        modelList.push_back(treeView->rootIndex());
        for (int i = 0; i < dirList.size(); i++)
        {
            if (modelList.size() > i)
            {
                QModelIndex idxCurrent;
                if (FindModelIndex(modelList.at(i), dirList.at(i), idxCurrent))
                {
                    modelList.push_back(idxCurrent);
                    treeView->expand(idxCurrent);
                }
            }
        }
        QModelIndex idxCurrent;
        if (FindModelIndex(modelList.back(), PARTICLES.data(), idxCurrent))
        {
            treeView->setCurrentIndex(idxCurrent);
        }
    }

    bool ParticleBrowserWidget::FindModelIndex(const QModelIndex& idxParent, const QString& key, QModelIndex& idxCurrent)
    {
        int elements = m_ui->m_assetBrowserTreeViewWidget->model()->rowCount(idxParent);
        for (int idx = 0; idx < elements; ++idx)
        {
            auto rowIdx = m_ui->m_assetBrowserTreeViewWidget->model()->index(idx, 0, idxParent);
            auto rowEntry = m_ui->m_assetBrowserTreeViewWidget->GetEntryFromIndex<AssetBrowserEntry>(rowIdx);
            if (rowEntry == nullptr)
            {
                continue;
            }
            if (key.compare(rowEntry->GetName().c_str()) == 0)
            {
                idxCurrent = rowIdx;
                return true;
            }
        }
        return false;
    }

    void ParticleBrowserWidget::ExpandToKey(const QModelIndex& idxParent, const QString& key)
    {
        int elements = m_ui->m_assetBrowserTreeViewWidget->model()->rowCount(idxParent);
        for (int idx = 0; idx < elements; ++idx)
        {
            auto rowIdx = m_ui->m_assetBrowserTreeViewWidget->model()->index(idx, 0, idxParent);
            auto productEntry = m_ui->m_assetBrowserTreeViewWidget->GetEntryFromIndex<AssetBrowserEntry>(rowIdx);
            if (productEntry && productEntry->GetDisplayName().contains(key, Qt::CaseInsensitive))
            {
                QModelIndex index = idxParent;
                while (index.isValid())
                {
                    m_ui->m_assetBrowserTreeViewWidget->expand(index);
                    QModelIndex newIdx = index.parent(); // its not safe to iterate on itself!
                    index = newIdx;
                }
                continue;
            }
            ExpandToKey(rowIdx, key);
        }
    }

    void ParticleBrowserWidget::LoadAllParticles()
    {
        LoadParticleFromItem(m_ui->m_assetBrowserTreeViewWidget->rootIndex());
        m_ui->m_searchWidget->GetTypesFilter()->RemoveAllFilters();
    }

    void ParticleBrowserWidget::LoadParticleFromItem(const QModelIndex& idxParent)
    {
        auto treeView = m_ui->m_assetBrowserTreeViewWidget;
        int elements = treeView->model()->rowCount(idxParent);
        for (int idx = 0; idx < elements; ++idx)
        {
            auto rowIdx = treeView->model()->index(idx, 0, idxParent);
            auto rowEntry = m_ui->m_assetBrowserTreeViewWidget->GetEntryFromIndex<AssetBrowserEntry>(rowIdx);
            int size = rowEntry->GetChildCount();
            for (int i = 0; i < size; i++)
            {
                const AssetBrowserEntry* pEntry = rowEntry->GetChild(i);
                if (pEntry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Folder)
                {
                    continue;
                }
                else
                {
                    if (pEntry->GetDisplayName().endsWith(PARTICLE_EXTENSION.data()))
                    {
                        QString sParticlePath = pEntry->GetFullPath().c_str();
                        if (!m_particlesPath.contains(sParticlePath))
                        {
                            m_particlesPath.push_back(sParticlePath);
                        }
                    }
                }
            }
            LoadParticleFromItem(rowIdx);
        }
    }

    void ParticleBrowserWidget::CreateMenu(QMenu* menu)
    {
        QAction* actionFolder = new QAction(tr("Add New Folder"), menu);
        QAction* actionParticleSys = new QAction(tr("Add New Particle System"), menu);
        connect(actionFolder, &QAction::triggered, this, &ParticleBrowserWidget::CreateFolderSlot);
        connect(actionParticleSys, &QAction::triggered, this, &ParticleBrowserWidget::CreateNewParticle);
        menu->addAction(actionFolder);
        menu->addAction(actionParticleSys);
    }

    void ParticleBrowserWidget::AddNewMenu()
    {
        QMenu* menu = new QMenu(this);
        CreateMenu(menu);
        m_ui->m_addNewButton->setMenu(menu);
    }

    void ParticleBrowserWidget::AddNewSlot(const QPoint& pos)
    {
        QListWidgetItem* curItem = m_ui->listWidget->itemAt(pos);
        if (curItem != nullptr)
        {
            return;
        }
        QMenu *menu = new QMenu(this);
        CreateMenu(menu);
        menu->exec(QCursor::pos());
    }

    void ParticleBrowserWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        AZ_UNUSED(time);
        AZ_UNUSED(deltaTime);

        if (!m_pathToSelect.empty())
        {
            // Attempt to select the new path
            AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Broadcast(
                &AzToolsFramework::AssetBrowser::AssetBrowserViewRequestBus::Events::SelectFileAtPath, m_pathToSelect);

            // Iterate over the selected entries to verify if the selection was made
            for (const AssetBrowserEntry* entry : m_ui->m_assetBrowserTreeViewWidget->GetSelectedAssets())
            {
                if (entry)
                {
                    AZStd::string sourcePath = entry->GetFullPath();
                    AzFramework::StringFunc::Path::Normalize(sourcePath);
                    if (m_pathToSelect == sourcePath)
                    {
                        // Once the selection is confirmed, cancel the operation and disconnect
                        AZ::TickBus::Handler::BusDisconnect();
                        m_pathToSelect.clear();
                    }
                }
            }
        }
    }
} // namespace OpenParticleSystemEditor

#include <Window/moc_OpenParticleBrowserWidget.cpp>
