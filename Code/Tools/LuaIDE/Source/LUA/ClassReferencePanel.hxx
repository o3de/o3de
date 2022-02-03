/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>

#include <QtCore/QObject>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeView>
#endif

#pragma once

namespace LUAEditor
{
    class Context;
}

class DHClassReferenceWidget : public QTreeView
{
    Q_OBJECT;
public:
    // class allocator inteintionally removed so that Qt gui designer can make us
    //AZ_CLASS_ALLOCATOR(DHClassReferenceWidget,AZ::SystemAllocator,0);

    DHClassReferenceWidget( QWidget * parent = 0 );
    virtual ~DHClassReferenceWidget();

    public slots:
        void OnDoubleClicked( const QModelIndex & );
};

