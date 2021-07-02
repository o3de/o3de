/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef CONFIGURE_ANNOTATIONS_WINDOW_H
#define CONFIGURE_ANNOTATIONS_WINDOW_H

#pragma once

#if !defined(Q_MOC_RUN)
#include <QDialog>
#include <QAbstractTableModel>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#endif

class QSortFilterProxyModel;

namespace Ui
{
    class configureAnnotationsDialog;
}

namespace Driller
{
    class AnnotationsProvider;
    class ConfigureAnnotationsModel;

    class ConfigureAnnotationsWindow : public QDialog
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(ConfigureAnnotationsWindow, AZ::SystemAllocator, 0);

        ConfigureAnnotationsWindow(QWidget *pParent = NULL);
        virtual ~ConfigureAnnotationsWindow();

        void Initialize(AnnotationsProvider* ptrProvider);

    public slots:
        void OnFilterChanged(const QString&);
        
    protected:
        Ui::configureAnnotationsDialog*    m_ptrLoadedUI;
        QSortFilterProxyModel* m_proxyModel;
        ConfigureAnnotationsModel* m_ptrModel;
        AnnotationsProvider* m_ptrProvider;
        virtual void closeEvent ( QCloseEvent * e );
    };
    
    
    class ConfigureAnnotationsModel : public QAbstractTableModel
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(ConfigureAnnotationsModel, AZ::SystemAllocator, 0);
        
        ////////////////////////////////////////////////////////////////////////////////////////////////
        // QAbstractTableModel
        int rowCount(const QModelIndex& index = QModelIndex()) const override;
        int columnCount(const QModelIndex& index = QModelIndex()) const override;
        Qt::ItemFlags flags(const QModelIndex &index) const override;
        
        QVariant data(const QModelIndex& index, int role) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) override;
        QVariant headerData ( int section, Qt::Orientation orientation, int role ) const override;
        ////////////////////////////////////////////////////////////////////////////////////////////////

        ConfigureAnnotationsModel(AnnotationsProvider* ptrProvider, QObject *pParent = NULL);
        
        virtual ~ConfigureAnnotationsModel();
        
    private:
        AnnotationsProvider* m_ptrProvider;
        AZStd::vector<QString> m_cache;
        AZStd::vector<QPixmap> m_cachedColorIcons;

        QPixmap CreatePixmapForColor(QColor color);
    
    private slots:
        void Recache();
    };


}

#endif //CONFIGURE_ANNOTATIONS_WINDOW_H
