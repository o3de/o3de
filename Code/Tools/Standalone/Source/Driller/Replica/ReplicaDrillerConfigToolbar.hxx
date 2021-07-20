/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
