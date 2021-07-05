/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QMenu>
#endif

namespace AzToolsFramework
{
    class InstanceDataNode;
}

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        class PropertyGridContextMenu
            : public QMenu
        {
            Q_OBJECT

        public:

            PropertyGridContextMenu(AzToolsFramework::InstanceDataNode* node);
        };
    }
}
