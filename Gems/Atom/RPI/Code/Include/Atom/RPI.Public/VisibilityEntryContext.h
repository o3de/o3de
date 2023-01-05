/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/ThreadLocalContext.h>

namespace AZ
{
    namespace RPI
    {
        struct VisiblityEntryProperties
        {
            //! A pointer to the draw item
            const void* m_userData = nullptr;
            //! A sorting key of this draw item which is used for sorting draw items in DrawList
            // Check RHI::SortDrawList() function for detail
            RHI::DrawItemSortKey m_sortKey = 0;
            //! A depth value this draw item which is used for sorting draw items in DrawList
            //! Check RHI::SortDrawList() function for detail
            float m_depth = 0.0f;
        };
        using VisibilityList = AZStd::vector<VisiblityEntryProperties>;
        using VisibilityListView = AZStd::span<const VisiblityEntryProperties>;

        /**
         * This class is a context for filling and accessing visibility lists. It is designed to be thread-safe
         * and low-contention. The API is partitioned into two phases: append and consume.
         *
         * In the append phase, visibility entries (or void* pointers to user data) are added to the context.
         * This is thread-safe and low contention. 
         *
         * Call FinalizeLists to transition to the consume phase. This performs sorting and coalescing
         * of visibility entries.
         *
         * Finally, in the consume phase, the context is immutable and lists are accessible via GetList.
         */
        class VisibilityEntryContext final
        {
        public:
            VisibilityEntryContext() = default;

            /// Copies and moves are disabled to enforce thread safety.
            AZ_DISABLE_COPY_MOVE(VisibilityEntryContext);

            bool IsInitialized() const;

            /// Must be called prior to adding draw items. Defines the set of draw list tags to filter into.
            void Init();

            void Shutdown();

            /// Filters the draw items in the draw packet into draw lists. Only draw lists specified at init time are appended.
            /// The depth value here is the depth of the object from the perspective of the view.
            void AddVisibilityEntry(const void* userData, float depth = 0.0f);

            /// Coalesces the draw lists in preparation for access via GetList. This should
            /// be called from a single thread as a sync point between the append / consume phases.
            void FinalizeLists();

            /// Returns the draw list associated with the provided tag.
            VisibilityListView GetList() const;

        private:
            RHI::ThreadLocalContext<VisibilityList> m_visibilityListContext;
            VisibilityList m_finalizedVisibilityList;
        };
    }
}
