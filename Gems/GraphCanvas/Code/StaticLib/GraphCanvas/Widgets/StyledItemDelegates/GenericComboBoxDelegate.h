/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        AZ_CLASS_ALLOCATOR(GenericComboBoxDelegate, AZ::SystemAllocator, 0);

        // Some large Qt field. Should be larger then 256 which is the Qt::UserRole
        // Don't want to make it too close to avoid Running into other custom user controls.
        static const int ComboBoxDelegateRole = 0xdd31a0a6; //AZ_CRC("GenericComboBoxDelegate", 0xdd31a0a6);

        explicit GenericComboBoxDelegate(QObject* parent);
        
        void setEditorData(QWidget* editor, const QModelIndex& index) const override;
        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    
    private Q_SLOTS:
        void dismissComboBox();
    };
}
