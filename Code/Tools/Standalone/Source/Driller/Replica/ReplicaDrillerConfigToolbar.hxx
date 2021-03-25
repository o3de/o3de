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

#ifndef DRILLER_REPLICA_REPLICADRILLERCONFIGTOOLBAR_H
#define DRILLER_REPLICA_REPLICADRILLERCONFIGTOOLBAR_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#pragma once

#include <QtWidgets/QWidget>
#endif

namespace Ui
{
    class ReplicaDrillerConfigToolbar;
}

namespace Driller
{
    class ReplicaDrillerConfigToolbar
        : public QWidget
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(ReplicaDrillerConfigToolbar, AZ::SystemAllocator,0);

        explicit ReplicaDrillerConfigToolbar(QWidget* parent = nullptr);
        ~ReplicaDrillerConfigToolbar();
        
        void enableTreeCommands(bool enabled);

    public:
        signals:
            void hideSelected();
            void showSelected();

            void hideAll();
            void showAll();

            void collapseAll();
            void expandAll();            

    private:

        Ui::ReplicaDrillerConfigToolbar* m_gui;
    };
}

#endif