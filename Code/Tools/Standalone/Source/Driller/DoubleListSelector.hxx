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
