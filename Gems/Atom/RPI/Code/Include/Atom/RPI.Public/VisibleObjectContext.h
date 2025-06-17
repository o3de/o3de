/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/ThreadLocalContext.h>
#include <Atom/RPI.Public/Configuration.h>

namespace AZ
{
    namespace RPI
    {
        // A struct representing an objet to be culled by the RPI culling system,
        // with the visible objects written to a visiblity list instead of being rendered
        // directly by the RPI
        struct VisibleObjectProperties
        {
            //! A pointer to the custom data for this object
            const void* m_userData = nullptr;
            //! A depth value this object which can be used for sorting draw calls
            float m_depth = 0.0f;
        };
        using VisibleObjectList = AZStd::vector<VisibleObjectProperties>;
        using VisibleObjectListView = AZStd::span<const VisibleObjectProperties>;

        /**
         * This class is a context for filling and accessing visible object lists. It is designed to be thread-safe
         * and low-contention. The API is partitioned into two phases: append and consume.
         *
         * In the append phase, visible object entries (or void* pointers to user data) are added to the context.
         * This is thread-safe and low contention. 
         *
         * Call FinalizeLists to transition to the consume phase. This combines the per-thread data into a single list.
         *
         * Finally, in the consume phase, the context is immutable and lists are accessible via GetList.
         */
        class ATOM_RPI_PUBLIC_API VisibleObjectContext final
        {
        public:
            VisibleObjectContext() = default;

            /// Copies and moves are disabled to enforce thread safety.
            AZ_DISABLE_COPY_MOVE(VisibleObjectContext);

            void Shutdown();

            /// Adds visible objects to the thread local visible object lists.
            /// The depth value here is the depth of the object from the perspective of the view.
            void AddVisibleObject(const void* userData, float depth = 0.0f);

            /// Coalesces the thread local visible object lists in preparation for access via GetList. This should
            /// be called from a single thread as a sync point between the append / consume phases.
            void FinalizeLists();

            /// Returns the visible object list for the view.
            VisibleObjectListView GetList() const;

        private:
            // Thread local storage of visible objects during the append phase
            RHI::ThreadLocalContext<VisibleObjectList> m_visibleObjectListContext;
            // Combined results from the thread local list to be used during the consume phase
            VisibleObjectList m_finalizedVisibleObjectList;
        };
    }
}
