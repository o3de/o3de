/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DrawPacket.h>
#include <Atom/RHI/DrawList.h>
#include <Atom/RHI/ThreadLocalContext.h>

namespace AZ::RHI
{
    //! This class is a context for filling and accessing draw lists. It is designed to be thread-safe
    //! and low-contention. To use it, initialize with the a bit-mask of draw list tags. This mask acts
    //! as a filter. The API is partitioned into two phases: append and consume.
    //!
    //! In the append phase, draw packets (or singular draw items) are added to the context. These are
    //! filtered into the table of draw lists. This is thread-safe and low contention. 
    //!
    //! Call FinalizeLists to transition to the consume phase. This performs sorting and coalescing
    //! of draw lists.
    //!
    //! Finally, in the consume phase, the context is immutable and lists are accessible via GetList.
    class DrawListContext final
    {
    public:
        DrawListContext() = default;

        /// Copies and moves are disabled to enforce thread safety.
        AZ_DISABLE_COPY_MOVE(DrawListContext);

        bool IsInitialized() const;

        /// Must be called prior to adding draw items. Defines the set of draw list tags to filter into.
        void Init(DrawListMask drawListMask);

        void Shutdown();

        /// Filters the draw items in the draw packet into draw lists. Only draw lists specified at init time are appended.
        /// The depth value here is the depth of the object from the perspective of the view.
        void AddDrawPacket(const DrawPacket* drawPacket, float depth = 0.0f);

        /// Adds an individual draw item to the draw list associated with the provided tag. This will
        /// no-op if the tag is not present in the internal draw list mask.
        void AddDrawItem(DrawListTag drawListTag, DrawItemProperties drawItemProperties);

        /// Coalesces the draw lists in preparation for access via GetList. This should
        /// be called from a single thread as a sync point between the append / consume phases.
        void FinalizeLists();

        /// Returns the draw list associated with the provided tag.
        DrawListView GetList(DrawListTag drawListTag) const;

        /// Returns the collection of merged draw lists. This is only so that the View can sort the
        /// merged draw lists and isn't intended for use outside that case.
        DrawListsByTag& GetMergedDrawListsByTag();

    private:
        ThreadLocalContext<DrawListsByTag> m_threadListsByTag;
        DrawListsByTag m_mergedListsByTag;
        DrawListMask m_drawListMask = 0;
    };
}
