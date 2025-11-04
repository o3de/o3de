/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QApplication>
#include <QPainter>
#include <QStyledItemDelegate>
AZ_POP_DISABLE_WARNING

#include <AzCore/Memory/SystemAllocator.h>
#endif

namespace GraphCanvas
{
    // General Delegate for allowing a ComboBox to be embedded into a ViewModel
    class GenericComboBoxDelegate
        : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(GenericComboBoxDelegate, AZ::SystemAllocator);

        // Some large Qt field. Should be larger then 256 which is the Qt::UserRole
        // Don't want to make it too close to avoid Running into other custom user controls.
        static const int ComboBoxDelegateRole = 0xdd31a0a6; //AZ_CRC_CE("GenericComboBoxDelegate");

        explicit GenericComboBoxDelegate(QObject* parent);
        
        void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    
    private Q_SLOTS:
        void dismissComboBox();
    };
}
