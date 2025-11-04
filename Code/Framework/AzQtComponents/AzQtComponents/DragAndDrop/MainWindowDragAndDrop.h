/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/Buses/DragAndDrop.h>

namespace AzQtComponents
{
    namespace DragAndDropContexts 
    {
        // this will be the context ID (used to connect to the drag and drop bus) if you care about events 
        // that cover the entire main window.
        static const AZ::u32 EditorMainWindow = AZ_CRC_CE("EditorMainWindow");
    }

}
