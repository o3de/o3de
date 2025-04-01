/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>

#include <GraphCanvas/Types/Endpoint.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/ContextMenuAction.h>

namespace GraphCanvas
{
    class EndpointSelectionAction
        : public QAction
    {
    public:
        AZ_CLASS_ALLOCATOR(EndpointSelectionAction, AZ::SystemAllocator);

        EndpointSelectionAction(const Endpoint& endpoint);
        ~EndpointSelectionAction() = default;

        const Endpoint& GetEndpoint() const;

    private:
        Endpoint m_endpoint;
    };
}
