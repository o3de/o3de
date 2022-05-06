/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/RoutingAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    void RoutingAdapter::ClearRoutes()
    {
        RemoveRoute(Dom::Path());
    }

    void RoutingAdapter::AddRoute(const Dom::Path& route, DocumentAdapterPtr adapter)
    {
        if (m_rootRouter != nullptr)
        {
            m_rootRouter->AddRoute(m_relativeRoutingPath / route, adapter);
            return;
        }

        AZStd::unique_ptr<RouteEntry> entry = AZStd::make_unique<RouteEntry>();
        entry->m_contents = adapter->GetContents();
        entry->m_onAdapterChanged = ChangedEvent::Handler(
            [this, entryPtr = entry.get()](const Dom::Patch& patch)
            {
                // On patch, map it to the actual path in this adapter and notify the view
                NotifyContentsChanged(MapPatchToRoute(patch, entryPtr->m_path));
            });
        entry->m_onAdapterReset = ResetEvent::Handler(
            [this, entryPtr = entry.get()]()
            {
                // On reset, create a replace operation that updates the entire contents of this route
                Dom::Patch patch;
                patch.PushBack(Dom::PatchOperation::ReplaceOperation(entryPtr->m_path, entryPtr->m_adapter->GetContents()));
                NotifyContentsChanged(patch);
            });
        adapter->ConnectChangedHandler(entry->m_onAdapterChanged);
        adapter->ConnectResetHandler(entry->m_onAdapterReset);
        entry->m_adapter = AZStd::move(adapter);
        m_routes.SetValue(route, AZStd::move(entry));
    }

    void RoutingAdapter::RemoveRoute(const Dom::Path& route)
    {
        if (m_rootRouter != nullptr)
        {
            m_rootRouter->RemoveRoute(m_relativeRoutingPath);
            return;
        }

        m_routes.EraseValue(route, true);
    }

    void RoutingAdapter::SetRouter(RoutingAdapter* router, const Dom::Path& route)
    {
        m_rootRouter = router;
        m_relativeRoutingPath = route;
    }

    Dom::Path RoutingAdapter::MapPathToRoute(const Dom::Path& path, const Dom::Path& route)
    {
        Dom::Path result = route;
        AZ_Assert(
            path.size() >= 1 && path[0].IsIndex() && path[0].GetIndex() == 0, "All adapter paths should start with /0 for the Adapter tag");
        // Skip the root /0 for any adapter paths, as they'll have been subsumed by this route
        for (size_t i = 1; i < path.size(); ++i)
        {
            result.Push(path[i]);
        }
        return result;
    }

    Dom::Patch RoutingAdapter::MapPatchToRoute(const Dom::Patch& patch, const Dom::Path& route)
    {
        Dom::Patch routedPatch = patch;
        for (Dom::PatchOperation& entry : routedPatch)
        {
            entry.SetDestinationPath(MapPathToRoute(entry.GetDestinationPath(), route));
            const Dom::PatchOperation::Type entryType = entry.GetType();
            if (entryType == Dom::PatchOperation::Type::Move || entryType == Dom::PatchOperation::Type::Copy)
            {
                entry.SetSourcePath(MapPathToRoute(entry.GetSourcePath(), route));
            }
        }
        return routedPatch;
    }
} // namespace AZ::DocumentPropertyEditor
