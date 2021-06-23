/*
 * Copyright (c) Contributors to the Open 3D Engine Project
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
        static const AZ::u32 EditorMainWindow = AZ_CRC("EditorMainWindow", 0x82a58b05);
    }

}
