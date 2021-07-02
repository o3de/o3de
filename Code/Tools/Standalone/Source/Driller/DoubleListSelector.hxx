/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef AZTOOLSFRAMEWORK_UI_UICORE_DOUBLELISTSELECTOR_H
#define AZTOOLSFRAMEWORK_UI_UICORE_DOUBLELISTSELECTOR_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

#include <QtWidgets/QWidget>
#endif

namespace Ui
{
    class DoubleListSelector;
}

namespace Driller
{
    class DoubleListSelector
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(DoubleListSelector, AZ::SystemAllocator,0);

        explicit DoubleListSelector(QWidget* parent = nullptr);
        ~DoubleListSelector();

        void setItemList(const QStringList& items, bool maintainActiveList = true);
        void setActiveItems(const QStringList& items);

        const QStringList& getActiveItems() const;

        void setActiveTitle(const QString& title);
        void setInactiveTitle(const QString& title);

    public slots:
        void activateSelected();
        void deactivateSelected();

    signals:
        void ActiveItemsChanged();

    private:

        Ui::DoubleListSelector* m_gui;        
    };
}

#endif
