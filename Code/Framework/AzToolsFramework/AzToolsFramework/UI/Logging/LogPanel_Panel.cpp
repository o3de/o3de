/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "LogPanel_Panel.h"

#include <AzCore/std/delegate/delegate.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/XML/rapidxml.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <QTimer>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // 4251: 'QPainter::d_ptr': class 'QScopedPointer<QPainterPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QPainter'
                                                               // 4800: 'QFlags<QPainter::RenderHint>::Int': forcing value to bool 'true' or 'false' (performance warning)
#include <QDateTime>
#include <QPainter>
AZ_POP_DISABLE_WARNING
#include <QPushButton>
#include <QAbstractItemModel>
#include <QTextDocument>
AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                                    // 4800 'QTextEngine *const ': forcing value to bool 'true' or 'false' (performance warning)
#include <QAbstractTextDocumentLayout>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QLabel>
#include <QtWidgets/QApplication>

#include <AzQtComponents/Components/Widgets/TabWidget.h>

#include "NewLogTabDialog.h"
#include "LogControl.h"
#include "LoggingCommon.h"
#include <AzCore/Casting/numeric_cast.h>

namespace AzToolsFramework
{
    namespace LogPanel
    {
        // some tweakables
        static int s_defaultRingBufferSize = 2000; // how many messages in a traceprintf log tab to keep before older ones will be expired (by default)

        struct BaseLogPanel::Impl
        {
            AzQtComponents::TabWidget* pTabWidget;
            AZ::u32 storageID;
            AZStd::unordered_map<QObject*, TabSettings> settingsForTabs;
        };

        BaseLogPanel::BaseLogPanel(QWidget* pParent)
            : QWidget(pParent)
            , m_impl(new BaseLogPanel::Impl)
        {
            m_impl->storageID = 0;
            this->setLayout(aznew LogPanelLayout(nullptr));

            m_impl->pTabWidget = new AzQtComponents::TabWidget(this);
            m_impl->pTabWidget->setObjectName(QString::fromUtf8("tabWidget"));
            m_impl->pTabWidget->setGeometry(QRect(9, 9, 16, 16));
            m_impl->pTabWidget->setTabsClosable(true);
            m_impl->pTabWidget->setMovable(true);
            layout()->addWidget(m_impl->pTabWidget);

            // 1) new empty widget with horizontal layout
            QWidget* layoutWidget = new QWidget(this);
            layoutWidget->setLayout(new QHBoxLayout(layoutWidget));
            QMargins q(0, 0, 0, 0);
            layoutWidget->layout()->setContentsMargins(q);

            // 2) add buttons for "Copy all", "Reset" and "Add" actions

            QPushButton* pCopyAllButton = new QPushButton(QIcon(QStringLiteral(":/stylesheet/img/logging/copy.svg")), tr("Copy all"), this);
            layoutWidget->layout()->addWidget(pCopyAllButton);

            QPushButton* pResetButton = new QPushButton(QIcon(QStringLiteral(":/stylesheet/img/logging/reset.svg")), tr("Reset"), this);
            layoutWidget->layout()->addWidget(pResetButton);

            QPushButton* pContextButton = new QPushButton(QIcon(QStringLiteral(":/stylesheet/img/logging/add-filter.svg")), tr("Add..."), this);
            layoutWidget->layout()->addWidget(pContextButton);

            layout()->addWidget(layoutWidget);

            // ensure button size matches tab bar size
            QWidget temp;
            m_impl->pTabWidget->addTab(&temp, "");
            layoutWidget->setFixedHeight(m_impl->pTabWidget->tabBar()->sizeHint().height());
            m_impl->pTabWidget->removeTab(0);

            layout()->setContentsMargins(0, 0, 0, 0);

            connect(m_impl->pTabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(onTabClosed(int)));
            connect(pContextButton, SIGNAL(clicked(bool)), this, SLOT(onAddClicked(bool)));
            connect(pResetButton, SIGNAL(clicked(bool)), this, SLOT(onResetClicked(bool)));
            connect(pCopyAllButton, SIGNAL(clicked(bool)), this, SLOT(onCopyAllClicked()));

            pParent->layout()->addWidget(this);
        }

        void BaseLogPanel::SetStorageID(AZ::u32 id)
        {
            m_impl->storageID = id;
        }

        int BaseLogPanel::GetTabWidgetCount()
        {
            return m_impl->pTabWidget->count();
        }

        QWidget* BaseLogPanel::GetTabWidgetAtIndex(int index)
        {
            return m_impl->pTabWidget->widget(index);
        }

        void BaseLogPanel::Reflect(AZ::ReflectContext* reflection)
        {
            SavedState::Reflect(reflection);
        }

        QSize BaseLogPanel::minimumSize() const
        {
            return layout()->minimumSize();
        }

        QSize BaseLogPanel::sizeHint() const
        {
            return minimumSize();
        }

        BaseLogPanel::~BaseLogPanel()
        {
        }

        void BaseLogPanel::onAddClicked(bool /*checked*/)
        {
            // user clicked the "Add..." button

            NewLogTabDialog newDialog(this); 
            if (newDialog.exec() == QDialog::Accepted)
            {
                // add a new tab with those settings.
                TabSettings settings(newDialog.m_tabName.toUtf8().data(),
                    newDialog.m_windowName.toUtf8().data(),
                    newDialog.m_textFilter.toUtf8().data(),
                    newDialog.m_checkNormal,
                    newDialog.m_checkWarning,
                    newDialog.m_checkError,
                    newDialog.m_checkDebug);

                AddLogTab(settings);
            }
        }

        void BaseLogPanel::onResetClicked(bool /*checked*/)
        {
            // user clicked the "Reset" button

            while (m_impl->pTabWidget->widget(0))
            {
                onTabClosed(0);
            }

            Q_EMIT TabsReset();
        }

        void BaseLogPanel::onCopyAllClicked()
        {
            if (m_impl->pTabWidget)
            {
                QWidget* currentTab = m_impl->pTabWidget->currentWidget();
                if (currentTab)
                {
                    QMetaObject::invokeMethod(currentTab, "CopyAll", Qt::QueuedConnection);
                }
            }
        }

        void BaseLogPanel::AddLogTab(const TabSettings& settings)
        {
            QWidget* newTab = CreateTab(settings);

            if (newTab)
            {
                int newTabIndex = m_impl->pTabWidget->addTab(newTab, QString::fromUtf8(settings.m_tabName.c_str()));
                m_impl->pTabWidget->setCurrentIndex(newTabIndex);
                m_impl->settingsForTabs.insert(AZStd::make_pair(qobject_cast<QObject*>(newTab), settings));
                
                connect(newTab, SIGNAL(onLinkActivated(const QString&)), this, SIGNAL(onLinkActivated(const QString&)));
            }
        }

        bool BaseLogPanel::LoadState()
        {
            AZStd::intrusive_ptr<SavedState> savedState;

            if (m_impl->storageID != 0)
            {
                savedState = AZ::UserSettings::Find<SavedState>(m_impl->storageID, AZ::UserSettings::CT_LOCAL);
            }

            if (savedState)
            {
                if (savedState->m_tabSettings.empty())
                {
                    return false;
                }

                while (m_impl->pTabWidget->count())
                {
                    QWidget* pWidget = m_impl->pTabWidget->widget(0);
                    m_impl->pTabWidget->removeTab(0);
                    delete pWidget;
                }

                for (const TabSettings& settings : savedState->m_tabSettings)
                {
                    AddLogTab(settings);
                }
            }
            else
            {
                return false;
            }

            return true;
        }

        void BaseLogPanel::SaveState()
        {
            if (m_impl->storageID == 0)
            {
                AZ_TracePrintf("Debug", "A log window not storing its state because it has not been assigned a storage ID.");
                return;
            }

            AZStd::intrusive_ptr<SavedState> myState = AZ::UserSettings::CreateFind<SavedState>(m_impl->storageID, AZ::UserSettings::CT_LOCAL);
            myState->m_tabSettings.clear(); // because it might find an existing state!

            for (auto pair : m_impl->settingsForTabs)
            {
                myState->m_tabSettings.push_back(pair.second);
            }
        }

        void BaseLogPanel::onTabClosed(int whichTab)
        {
            // a tab was closed.
            QWidget* pWidget = m_impl->pTabWidget->widget(whichTab);
            m_impl->pTabWidget->removeTab(whichTab);
            delete pWidget;
        }

        RingBufferLogDataModel::RingBufferLogDataModel(QObject* pParent)
            : QAbstractTableModel(pParent)
            , m_lines(s_defaultRingBufferSize)
        {
            m_startLineAdd = -1;
            m_numLinesAdded = 0;
            m_numLinesRemoved = 0;
        }

        RingBufferLogDataModel::~RingBufferLogDataModel()
        {
        }

        // implementation of the data() method.  Given an index (which contains a row and column), and a role for the data, such as color or display or what,
        // qt wants us to return an appropriate scrap of display data:
        QVariant RingBufferLogDataModel::data(const QModelIndex& index, int role) const
        {
            if ((!index.isValid()) || (index.parent() != QModelIndex()))
            {
                return QVariant();
            }

            if (role == Qt::TextAlignmentRole)
            {
                if (aznumeric_cast<DataRoles>(index.column()) == DataRoles::Window)  // the window column should be center align
                {
                    return Qt::AlignCenter;
                }
            }

            return m_lines[index.row()].data(index.column(), role);
        }

        // the Qt renderer and ui input layer wants to know what behavior the current cell has.
        // we need to let it know what cells have what flags - the only one we care about is
        // Qt::ItemIsEditable.  We should apply that to any cell you want the user to be able to double click on.
        Qt::ItemFlags RingBufferLogDataModel::flags(const QModelIndex& index) const
        {
            if (!index.isValid())
            {
                return Qt::ItemIsEnabled;
            }

            if (index.column() == 3)  // the 3rd column (message) is "editable" so that you can double click it and it turns into a copyable thing.
            {
                return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
            }


            return QAbstractItemModel::flags(index);
        }

        void RingBufferLogDataModel::AppendLine(Logging::LogLine& source)
        {
            if (m_startLineAdd == -1)
            {
                m_startLineAdd = (int)m_lines.size();
                m_numLinesAdded = 0;
            }

            if (m_lines.size() == m_lines.capacity())
            {
                m_numLinesRemoved++; // this line will cause a line to be removed.
            }
            m_lines.push_back();
            m_lines.back() = AZStd::move(source);
            ++m_numLinesAdded;
        }

        void RingBufferLogDataModel::Clear()
        {
            if (m_lines.empty())
            {
                return;
            }

            beginRemoveRows(QModelIndex(), 0, (int)m_lines.size() - 1);
            m_lines.clear();
            endRemoveRows();
        }

        int RingBufferLogDataModel::rowCount(const QModelIndex& index) const
        {
            if (index.parent() == QModelIndex())
            {
                return (int)m_lines.size();
            }

            return 0;
        }
        int RingBufferLogDataModel::columnCount(const QModelIndex& index) const
        {
            if (index.parent() == QModelIndex())
            {
                return 4; // icon + date + source + logged text;
            }

            return 0;
        }

        const Logging::LogLine& RingBufferLogDataModel::GetLineFromIndex(const QModelIndex& index)
        {
            return m_lines[index.row()];
        }

        void RingBufferLogDataModel::CommitAdd()
        {
            // we have finished adding lines in this clump.

            if (m_numLinesRemoved > m_startLineAdd)
            {
                m_numLinesRemoved = m_startLineAdd;
            }

            if (m_numLinesAdded > (int)m_lines.capacity())
            {
                m_numLinesAdded = (int)m_lines.capacity();
            }

            if (m_numLinesRemoved)
            {
                beginRemoveRows(QModelIndex(), 0, m_numLinesRemoved - 1);
                endRemoveRows();
            }

            if (m_numLinesAdded)
            {
                beginInsertRows(QModelIndex(), m_startLineAdd - m_numLinesRemoved, m_startLineAdd - m_numLinesRemoved + (m_numLinesAdded - 1));
                endInsertRows();
            }


            // remember though that we need to remove rows when the size remains the same:

            m_startLineAdd = -1;
            m_numLinesAdded = 0;
            m_numLinesRemoved = 0;
        }

        ListLogDataModel::ListLogDataModel(QObject* pParent)
            : QAbstractTableModel(pParent)
        {
            m_lines.reserve(50);
        }

        ListLogDataModel::~ListLogDataModel()
        {
        }


        // implementation of the data() method.  Given an index (which contains a row and column), and a role for the data, such as color or display or what,
        // qt wants us to return an appropriate scrap of display data:
        QVariant ListLogDataModel::data(const QModelIndex& index, int role) const
        {
            if ((!index.isValid()) || (index.parent() != QModelIndex()))
            {
                return QVariant();
            }

            return m_lines[index.row()].data(index.column(), role);
        }

        // the Qt renderer and ui input layer wants to know what behavior the current cell has.
        // we need to let it know what cells have what flags - the only one we care about is
        // Qt::ItemIsEditable.  We should apply that to any cell you want the user to be able to double click on.
        Qt::ItemFlags ListLogDataModel::flags(const QModelIndex& index) const
        {
            if (!index.isValid())
            {
                return Qt::ItemIsEnabled;
            }

            if (index.column() == 3)  // the 3rd column (message) is "editable" so that you can double click it and it turns into a copyable thing.
            {
                return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
            }

            return QAbstractItemModel::flags(index);
        }

        void ListLogDataModel::AppendLine(Logging::LogLine& source)
        {
            if (!m_alreadyAddingLines)
            {
                m_alreadyAddingLines = true;
                m_linesAdded = 0;
            }

            m_lines.push_back();
            m_lines.back() = AZStd::move(source);
            ++m_linesAdded;
        }

        void ListLogDataModel::Clear()
        {
            beginRemoveRows(QModelIndex(), 0, (int)m_lines.size() - 1);
            m_alreadyAddingLines = false;
            m_linesAdded = 0;
            m_lines.clear();
            endRemoveRows();
        }

        int ListLogDataModel::rowCount(const QModelIndex& index) const
        {
            if (index.parent() == QModelIndex())
            {
                return (int)m_lines.size();
            }

            return 0;
        }
        int ListLogDataModel::columnCount(const QModelIndex& index) const
        {
            if (index.parent() == QModelIndex())
            {
                return 4; // icon + date + source + logged text;
            }

            return 0;
        }

        void ListLogDataModel::CommitAdd()
        {
            // we have finished adding lines in this clump.
            if (m_linesAdded > 0)
            {
                int numLines = azlossy_caster(m_lines.size());
                beginInsertRows(QModelIndex(), numLines - m_linesAdded, numLines - 1);
                endInsertRows();
            }

            // remember though that we need to remove rows when the size remains the same:
            m_linesAdded = 0;
            m_alreadyAddingLines = false;
        }

        FilteredLogDataModel::FilteredLogDataModel(QObject* parent)
            : QSortFilterProxyModel(parent)
        {
        }

        void FilteredLogDataModel::SetTabSettings(const TabSettings& source)
        {
            m_tabSettings = source;
            invalidateFilter();
        }

        bool FilteredLogDataModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
        {
            if (sourceParent.isValid())
            {
                return false;
            }

            QAbstractItemModel* source = sourceModel();

            if (!source)
            {
                return false;
            }

            const Logging::LogLine* sourceLine = source->data(source->index(sourceRow, 0), ExtraRoles::LogLineRole).value<const Logging::LogLine*>();

            if (!sourceLine)
            {
                return false;
            }

            // apply filters.
            bool showErrors = ((m_tabSettings.m_filterFlags & (0x01 << TabSettings::FILTER_ERROR)) != 0);
            if (!showErrors) // do not display errors
            {
                if (sourceLine->GetLogType() == Logging::LogLine::TYPE_ERROR)
                {
                    return false;
                }
            }

            bool showWarnings = ((m_tabSettings.m_filterFlags & (0x01 << TabSettings::FILTER_WARNING)) != 0);
            if (!showWarnings)
            {
                if (sourceLine->GetLogType() == Logging::LogLine::TYPE_WARNING)
                {
                    return false;
                }
            }

            bool showMessages = ((m_tabSettings.m_filterFlags & (0x01 << TabSettings::FILTER_NORMAL)) != 0);
            if (!showMessages)
            {
                if (sourceLine->GetLogType() == Logging::LogLine::TYPE_MESSAGE)
                {
                    return false;
                }

                if (sourceLine->GetLogType() == Logging::LogLine::TYPE_CONTEXT)
                {
                    return false;
                }
            }

            bool showDebug = ((m_tabSettings.m_filterFlags & (0x01 << TabSettings::FILTER_DEBUG)) != 0);
            if (!showDebug)
            {
                if (sourceLine->GetLogType() == Logging::LogLine::TYPE_DEBUG)
                {
                    return false;
                }
            }

            if (!m_tabSettings.m_window.empty())
            {
                // filter to a specific window.
                if (m_tabSettings.m_window != "All")
                {
                    if (azstricmp(sourceLine->GetLogWindow().c_str(), m_tabSettings.m_window.c_str()) != 0)
                    {
                        return false;
                    }
                }
            }

            if (!m_tabSettings.m_textFilter.empty())
            {
                // filter to contains specific text:
                if (AzFramework::StringFunc::Find(sourceLine->GetLogMessage().c_str(), m_tabSettings.m_textFilter.c_str(), false, false) == AZStd::string::npos)
                {
                    // we did not find it
                    return false;
                }
            }

            return true;
        }

        void LogPanelLayout::addItem(QLayoutItem* pChild)
        {
            m_children.push_back(pChild);
        }
        QLayoutItem* LogPanelLayout::itemAt(int index) const
        {
            if (index >= (int)m_children.size())
            {
                return nullptr;
            }

            return m_children[index];
        }

        QLayoutItem* LogPanelLayout::takeAt(int index)
        {
            QLayoutItem* pItem = nullptr;

            if (index >= (int)m_children.size())
            {
                return nullptr;
            }

            pItem = m_children[index];
            m_children.erase(m_children.begin() + index);
            return pItem;
        }

        LogPanelLayout::LogPanelLayout(QWidget* /*pParent*/)
        {
        }

        LogPanelLayout::~LogPanelLayout()
        {
        }

        int LogPanelLayout::count() const
        {
            return (int)m_children.size();
        }

        void LogPanelLayout::setGeometry(const QRect& r)
        {
            int left, top, right, bottom;
            getContentsMargins(&left, &top, &right, &bottom);
            QRect effectiveRect = r.adjusted(+left, +top, -right, -bottom);
            for (int pos = 0; pos < (int)m_children.size() - 1; ++pos)
            {
                QLayoutItem* pItem = m_children[pos];
                if (pItem->widget())
                {
                    pItem->widget()->setGeometry(effectiveRect);
                }
                else
                {
                    pItem->setGeometry(effectiveRect);
                }
            }

            if (m_children.size())
            {
                // if we have any elements, the last element is top right aligned:
                QLayoutItem* pItem = m_children[m_children.size() - 1];
                QSize lastItemSize = pItem->minimumSize();
                QRect topRightCorner(effectiveRect.topRight() - QPoint(lastItemSize.width(), 0), lastItemSize);
                pItem->setGeometry(topRightCorner);
            }
        }

        Qt::Orientations LogPanelLayout::expandingDirections() const
        {
            return Qt::Orientations();
        }

        QSize LogPanelLayout::sizeHint() const
        {
            return minimumSize();
        }

        QSize LogPanelLayout::minimumSize() const
        {
            QSize size;

            int left, top, right, bottom;
            getContentsMargins(&left, &top, &right, &bottom);

            for (int pos = 0; pos < (int)m_children.size(); ++pos)
            {
                QLayoutItem* item = m_children[pos];
                size = size.expandedTo(item->minimumSize());
            }

            size += (QSize(left + right, top + bottom));

            return size;
        }

        LogPanelItemDelegate::LogPanelItemDelegate(QWidget* pParent, int messageColumn)
            : QStyledItemDelegate(pParent)
        {
            m_messageColumn = messageColumn;
            pOwnerWidget = pParent;
            m_painterLabel = new QLabel(pParent);
            m_painterLabel->setTextFormat(Qt::RichText);
            m_painterLabel->setAutoFillBackground(false);
            m_painterLabel->setContentsMargins(4, 0, 4, 0);
            m_painterLabel->setMargin(0);
            m_painterLabel->setIndent(0);
            m_painterLabel->hide();
        }

        LogPanelItemDelegate::~LogPanelItemDelegate()
        {
        }

        QSize LogPanelItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            if (index.isValid())
            {
                if (index.column() == m_messageColumn)
                {
                    QString data = index.data(Qt::DisplayRole).toString();
                    bool rich = index.data(ExtraRoles::RichTextRole).toBool();
                    if (rich)
                    {
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
                        QStyleOptionViewItemV4 viewItem = option;
                        initStyleOption(&viewItem, index);
#else
                        const QStyleOptionViewItem& viewItem = option;
#endif

                        QTextDocument doc;
                        doc.setHtml(viewItem.text);
                        doc.setDocumentMargin(2);
                        doc.setDefaultFont(viewItem.font);
                        doc.setTextWidth(viewItem.rect.width());
                        return QSize(static_cast<int>(doc.idealWidth()), static_cast<int>(doc.size().height()));
                    }
                }
            }
            return QStyledItemDelegate::sizeHint(option, index);
        }

        void LogPanelItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            if (index.column() == m_messageColumn)
            {
                bool rich = index.data(ExtraRoles::RichTextRole).toBool();
                // if we contain links then make it rich...
                if (rich)
                {
#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
                    QStyleOptionViewItemV4 tempOption = option;
#else
                    QStyleOptionViewItem tempOption = option;
#endif
                    initStyleOption(&tempOption, index);

                    QStyle* style = tempOption.widget ? tempOption.widget->style() : QApplication::style();

                    QTextDocument doc;
                    doc.setHtml(tempOption.text);
                    doc.setDocumentMargin(2);
                    doc.setDefaultFont(tempOption.font);
                    doc.setTextWidth(tempOption.rect.width());

                    /// Painting item without text
                    tempOption.text.clear();
                    style->drawControl(QStyle::CE_ItemViewItem, &tempOption, painter);

                    QAbstractTextDocumentLayout::PaintContext ctx;

                    // Highlighting text if item is selected
                    if (tempOption.state & QStyle::State_Selected)
                    {
                        ctx.palette.setColor(QPalette::Text, tempOption.palette.color(QPalette::Active, QPalette::HighlightedText));
                    }
                    else
                    {
                        ctx.palette.setColor(QPalette::Text, tempOption.palette.color(QPalette::Active, QPalette::Text));
                    }

                    QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &tempOption);
                    painter->save();
                    painter->translate(textRect.topLeft());
                    painter->setClipRect(textRect.translated(-textRect.topLeft()));
                    doc.documentLayout()->draw(painter, ctx);
                    painter->restore();
                    return;
                }
            }

            QStyledItemDelegate::paint(painter, option, index);
        }

        void LogPanelItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
        {
            (void)editor;
            (void)index;
            // we don't actually allow editing.
        }

        void LogPanelItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
        {
            (void)model;
            (void)index;
            // we only read data from the second column.
            if (index.column() == m_messageColumn)
            {
                QLabel* label = static_cast<QLabel*>(editor);
                QString data = index.data(Qt::DisplayRole).toString();
                label->setText(data);
            }
            return;
        }

        void LogPanelItemDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            (void)index;
            editor->setGeometry(option.rect);
        }

        QWidget* LogPanelItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            if (index.column() == m_messageColumn)
            {
                QString data = index.data(Qt::DisplayRole).toString();
                bool isRich = index.data(ExtraRoles::RichTextRole).toBool();

#if (QT_VERSION < QT_VERSION_CHECK(5, 11, 0))
                QStyleOptionViewItemV4 options = option;
#else
                QStyleOptionViewItem options = option;
#endif
                initStyleOption(&options, index);

                QLabel* richLabel = new QLabel(parent);
                richLabel->setFont(options.font);
                // only for rich text do we do rich text:

                // if we contain links then make it rich...
                if (isRich)
                {
                    richLabel->setTextFormat(Qt::RichText);
                }

                richLabel->setText(data);

                richLabel->setGeometry(options.rect);
                richLabel->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
                richLabel->setPalette(options.palette);
                richLabel->setAutoFillBackground(true);
                richLabel->setContentsMargins(4, 0, 4, 0);
                QStyle* style = options.widget ? options.widget->style() : QApplication::style();
                richLabel->setStyle(style);

                if (isRich)
                {
                    richLabel->resize(sizeHint(option, index));
                }

                // if a link is clicked, go ahead and let us know!  Thx.
                connect(richLabel, SIGNAL(linkActivated(const QString&)), this, SIGNAL(onLinkActivated(const QString&)));
                return richLabel;
            }

            return nullptr;
        }

        bool LogPanelItemDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
        {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }

    } // namespace LogPanel
} // namespace AzToolsFramework

#include "UI/Logging/moc_LogPanel_Panel.cpp"
