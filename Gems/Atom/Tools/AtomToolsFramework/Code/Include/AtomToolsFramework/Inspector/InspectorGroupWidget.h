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
#include <AzCore/Memory/SystemAllocator.h>
#include <QWidget>
#endif

namespace AtomToolsFramework
{
    class InspectorGroupWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(InspectorGroupWidget, AZ::SystemAllocator, 0);

        InspectorGroupWidget(QWidget* parent = nullptr);

        //! Apply non-destructive UI changes like repainting or updating fields
        virtual void Refresh();

        //! Apply destructive UI changes like adding or removing child widgets
        virtual void Rebuild();
    };
} // namespace AtomToolsFramework
