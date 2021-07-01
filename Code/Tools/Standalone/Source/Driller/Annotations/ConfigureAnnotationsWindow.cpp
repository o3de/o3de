/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandaloneTools_precompiled.h"
#include <QtCore/QAbstractTableModel>
#include "ConfigureAnnotationsWindow.hxx"
#include <Source/Driller/Annotations/ui_ConfigureAnnotationsDialog.h>
#include <Source/Driller/Annotations/Annotations.hxx>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzToolsFramework/UI/UICore/QWidgetSavedState.h>
#include <AzToolsFramework/UI/UICore/ColorPickerDelegate.hxx>

#include <QPainter>
#include <QSortFilterProxyModel>
#include <QCloseEvent>

namespace Driller
{
    //////////////////////////////
    // ConfigureAnnotationsModel
    //////////////////////////////
    int ConfigureAnnotationsModel::rowCount(const QModelIndex& index) const
    {
        if (index == QModelIndex())
        {
            return (int)m_cache.size();
        }
        return 0;
    }

    int ConfigureAnnotationsModel::columnCount(const QModelIndex& index) const
    {
        (void)index;
        return 1;
    }

    Qt::ItemFlags ConfigureAnnotationsModel::flags(const QModelIndex& index) const
    {
        if (index == QModelIndex())
        {
            return Qt::ItemFlags();
        }

        if (index.column() == 0)
        {
            return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
        }

        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    bool ConfigureAnnotationsModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role == Qt::CheckStateRole)
        {
            Qt::CheckState newState = (Qt::CheckState)value.toInt();
            if (index.column() == 0)
            {
                if (index.row() < (int)m_cache.size())
                {
                    Qt::CheckState oldEnabled = m_ptrProvider->IsChannelEnabled(AZ::Crc32(m_cache[index.row()].toUtf8().data())) ? Qt::Checked : Qt::Unchecked;

                    if (newState != oldEnabled)
                    {
                        m_ptrProvider->SetChannelEnabled(m_cache[index.row()].toUtf8().data(), newState == Qt::Checked ? true : false);
                        return true;
                    }
                }
            }
        }
        else if (role == AzToolsFramework::ColorPickerDelegate::COLOR_PICKER_ROLE)
        {
            QColor newColor = qvariant_cast<QColor>(value);
            AZ::u32 crcOfChannel = AZ::Crc32(m_cache[index.row()].toUtf8().data());
            m_ptrProvider->SetColorForChannel(crcOfChannel, newColor);
            m_cachedColorIcons[index.row()] = CreatePixmapForColor(newColor);
        }
        return false;
    }

    QVariant ConfigureAnnotationsModel::data(const QModelIndex& index, int role) const
    {
        if (index == QModelIndex())
        {
            return QVariant();
        }

        if (index.column() == 0)
        {
            switch (role)
            {
            case  Qt::CheckStateRole:
            {
                AZ::u32 crcvalue = AZ::Crc32(m_cache[index.row()].toUtf8().data());
                return QVariant(m_ptrProvider->IsChannelEnabled(crcvalue) ? QVariant(Qt::Checked) : QVariant(Qt::Unchecked));
            }
            case Qt::DecorationRole:
            {
                return m_cachedColorIcons[index.row()];
            }
            case Qt::DisplayRole:
            {
                return m_cache[index.row()];
            }
            case AzToolsFramework::ColorPickerDelegate::COLOR_PICKER_ROLE:
            {
                AZ::u32 crcvalue = AZ::Crc32(m_cache[index.row()].toUtf8().data());
                QColor channelColor = m_ptrProvider->GetColorForChannel(crcvalue);
                return QVariant(channelColor);
            }
            }
        }

        return QVariant();
    }

    QVariant ConfigureAnnotationsModel::headerData (int section, Qt::Orientation orientation, int role) const
    {
        (void)orientation;

        if (role == Qt::DisplayRole)
        {
            switch (section)
            {
            case 0:
                return QVariant(tr("Annotation Type"));
                break;
            }

            return QVariant();
        }
        else if (role == Qt::TextAlignmentRole)
        {
            if (section == 0)
            {
                return QVariant(Qt::AlignVCenter | Qt::AlignLeft);
            }
        }

        return QVariant();
    }

    QPixmap ConfigureAnnotationsModel::CreatePixmapForColor(QColor color)
    {
        QPixmap pixmap(16, 16);
        {
            QPainter painter(&pixmap);
            painter.fillRect(0, 0, 16, 16, Qt::black);
            painter.fillRect(1, 1, 15, 15, color);
        }
        return pixmap;
    }


    void ConfigureAnnotationsModel::Recache()
    {
        beginResetModel();
        m_cache.clear();
        m_cachedColorIcons.clear();
        AZStd::for_each(m_ptrProvider->GetAllKnownChannels().begin(), m_ptrProvider->GetAllKnownChannels().end(),
            [this](const AZStd::string& value)
            {
                m_cache.push_back(QString::fromUtf8(value.c_str()));
                QColor channelColor = m_ptrProvider->GetColorForChannel(AZ::Crc32(value.c_str()));

                m_cachedColorIcons.push_back(CreatePixmapForColor(channelColor));
            });
        endResetModel();
    }

    ConfigureAnnotationsModel::ConfigureAnnotationsModel(AnnotationsProvider* ptrProvider, QObject* pParent)
        : QAbstractTableModel(pParent)
    {
        m_ptrProvider = ptrProvider;
        connect(ptrProvider, &AnnotationsProvider::KnownAnnotationsChanged, this, &ConfigureAnnotationsModel::Recache);
        Recache();
    }

    ConfigureAnnotationsModel::~ConfigureAnnotationsModel()
    {
    }

    ///////////////////////////////
    // ConfigureAnnotationsWindow
    ///////////////////////////////

    ConfigureAnnotationsWindow::ConfigureAnnotationsWindow(QWidget* pParent /* = NULL */)
        : QDialog(pParent)
        , m_proxyModel(nullptr)
    {
        m_ptrLoadedUI = azcreate(Ui::configureAnnotationsDialog, ());
        m_ptrLoadedUI->setupUi(this);
    }

    ConfigureAnnotationsWindow::~ConfigureAnnotationsWindow()
    {
        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> pState = AZ::UserSettings::CreateFind<AzToolsFramework::QWidgetSavedState>(AZ_CRC("CONFIGURE ANNOTATIONS WINDOW", 0x581c6568), AZ::UserSettings::CT_GLOBAL);
        if (pState)
        {
            pState->CaptureGeometry(this);
        }

        azdestroy(m_ptrLoadedUI);
    }

    void ConfigureAnnotationsWindow::Initialize(AnnotationsProvider* ptrProvider)
    {
        m_ptrProvider = ptrProvider;
        m_ptrModel = aznew ConfigureAnnotationsModel(ptrProvider, this);

        m_proxyModel = new QSortFilterProxyModel(this);
        m_proxyModel->setDynamicSortFilter(false);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setSourceModel(m_ptrModel);

        m_ptrLoadedUI->statusTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_ptrLoadedUI->statusTable->setModel(m_proxyModel);
        m_ptrLoadedUI->statusTable->setItemDelegate(aznew AzToolsFramework::ColorPickerDelegate(this));
        m_ptrLoadedUI->statusTable->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);

        connect(m_ptrLoadedUI->searchField, SIGNAL(textChanged(const QString&)), this, SLOT(OnFilterChanged(const QString&)));

        AZStd::intrusive_ptr<AzToolsFramework::QWidgetSavedState> windowState = AZ::UserSettings::Find<AzToolsFramework::QWidgetSavedState>(AZ_CRC("CONFIGURE ANNOTATIONS WINDOW", 0x581c6568), AZ::UserSettings::CT_GLOBAL);

        if (windowState)
        {
            windowState->RestoreGeometry(this);
        }
    }

    void ConfigureAnnotationsWindow::OnFilterChanged(const QString& filter)
    {
        m_proxyModel->setFilterFixedString(filter);
    }

    void ConfigureAnnotationsWindow::closeEvent (QCloseEvent* e)
    {
        e->accept();
        deleteLater();
    }
}

#include <Source/Driller/Annotations/moc_ConfigureAnnotationsWindow.cpp>
