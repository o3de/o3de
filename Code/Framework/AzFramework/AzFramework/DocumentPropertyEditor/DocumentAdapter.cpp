/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    void DocumentAdapter::ConnectResetHandler(ResetEvent::Handler& handler)
    {
        handler.Connect(m_resetEvent);
    }

    void DocumentAdapter::ConnectChangedHandler(ChangedEvent::Handler& handler)
    {
        handler.Connect(m_changedEvent);
    }

    void DocumentAdapter::SetRouter(RoutingAdapter* /*router*/, const Dom::Path& /*route*/)
    {
        // By default, setting a router is a no-op, this only matters for nested routing adapters
    }

    void DocumentAdapter::NotifyResetDocument()
    {
        m_resetEvent.Signal();
    }

    void DocumentAdapter::NotifyContentsChanged(const AZ::Dom::Patch& patch)
    {
        m_changedEvent.Signal(patch);
    }
} // namespace AZ::DocumentPropertyEditor
