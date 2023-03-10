/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QApplication>
#include <QPainter>
#include <QStyledItemDelegate>
AZ_POP_DISABLE_WARNING

#include <AzCore/Memory/SystemAllocator.h>

namespace GraphCanvas
{
    class IconDecoratedNameDelegate
        : public QStyledItemDelegate
    {
    public:
        AZ_CLASS_ALLOCATOR(IconDecoratedNameDelegate, AZ::SystemAllocator);
        
        IconDecoratedNameDelegate(QWidget* parent);

        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    private:
        // Magic offset to try to align the icon with where Qt puts the icon. There's some padding it applies
        // and there's no way to really query it as far as I can tell.
        int m_paddingOffset = 14;
    };
}
