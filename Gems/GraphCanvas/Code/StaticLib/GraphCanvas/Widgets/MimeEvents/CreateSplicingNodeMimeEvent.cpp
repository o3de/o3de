/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <GraphCanvas/Widgets/MimeEvents/CreateSplicingNodeMimeEvent.h>

namespace GraphCanvas
{
    ////////////////////////////////
    // CreateSplicingNodeMimeEvent
    ////////////////////////////////

    void CreateSplicingNodeMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

        if (serializeContext)
        {
            serializeContext->Class<CreateSplicingNodeMimeEvent, GraphCanvasMimeEvent>()
                ->Version(0)
                ;
        }
    }
}
