/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomPatch.h>
#include <AzCore/DOM/DomPrefixTree.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

namespace AZ::DocumentPropertyEditor
{
    //! RoutingAdapter manages chaining DocumentAdapters together so that their contents
    //! hierarchically merge, and their corresponding patches get applied cleanly.
    class RoutingAdapter : public DocumentAdapter
    {
    public:
        RoutingAdapter() = default;

    protected:
        //! Clears all routes.
        //! Typically, this should be called at the start of GetContents, so that child routes can be reinitalized.
        void ClearRoutes();
        //! Adds a route at the given path to the specified adapter.
        //! Reset and Changed signals will be bubbled up from this child adapter to this router.
        //! This implies that this adapter's contents at route match the child contents of adapter.
        void AddRoute(const Dom::Path& route, DocumentAdapterPtr adapter);
        //! Removes a route at a given path, and all of its subpaths.
        void RemoveRoute(const Dom::Path& route);
        //! If set, forwards all routing requests from this router to another router.
        //! This allows a single, parent RoutingAdapter to handle all requests.
        void SetRouter(RoutingAdapter* router, const Dom::Path& route) override;

    private:
        //! Takes a path provided from a nested adapter and maps it back to the absolute path inside this router.
        Dom::Path MapPathToRoute(const Dom::Path& path, const Dom::Path& route);
        //! Maps all paths in a patch to absolute paths in this router.
        Dom::Patch MapPatchToRoute(const Dom::Patch& patch, const Dom::Path& route);

        struct RouteEntry
        {
            Dom::Path m_path;
            DocumentAdapterPtr m_adapter;
            Dom::Value m_contents;
            ResetEvent::Handler m_onAdapterReset;
            ChangedEvent::Handler m_onAdapterChanged;
        };

        Dom::DomPrefixTree<AZStd::unique_ptr<RouteEntry>> m_routes;
        RoutingAdapter* m_rootRouter = nullptr;
        Dom::Path m_relativeRoutingPath;
    };
} // namespace AZ::DocumentPropertyEditor
