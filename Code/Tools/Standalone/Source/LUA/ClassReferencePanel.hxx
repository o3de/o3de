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

