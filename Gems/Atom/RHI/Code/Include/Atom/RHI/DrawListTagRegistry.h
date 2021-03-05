/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <Atom/RHI/DrawList.h>
#include <AzCore/Name/Name.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/parallel/shared_mutex.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * Allocates and registers draw list tags by name, allowing the user to acquire and find tags from names.
         * The class is designed to map user-friendly tag names defined through content or higher level code to
         * low-level tags, which are simple handles.
         *
         * Some notes about usage and design:
         *   - DrawListTag values represent indexes into a bitmask, which allows for fast comparison when filtering
         *     draw items into draw lists (see View::HasDrawListTag()).
         *   - Tags are reference counted, which means multiple calls to 'Acquire' with the same name will increment 
         *     the internal reference count on the tag. This allows shared ownership between systems, if necessary.
         *   - FindTag is provided to search for a tag reference without taking ownership.
         *   - Names are case sensitive.
         */
        class DrawListTagRegistry final
            : public AZStd::intrusive_base
        {
        public:
            AZ_CLASS_ALLOCATOR(DrawListTagRegistry, AZ::SystemAllocator, 0);
            AZ_DISABLE_COPY_MOVE(DrawListTagRegistry);

            static Ptr<DrawListTagRegistry> Create();

            /**
             * Resets the registry back to an empty state. All references are released.
             */
            void Reset();

            /**
             * Acquires a draw list tag from the provided name (case sensitive). If the tag already existed, it is ref-counted.
             * Returns a valid tag on success; returns a null tag if the registry is at full capacity. You must
             * call ReleaseTag() if successful.
             */
            DrawListTag AcquireTag(const Name& drawListName);

            /**
             * Releases a reference to a tag. Tags are ref-counted, so it's necessary to maintain ownership of the
             * tag and release when its no longer needed.
             */
            void ReleaseTag(DrawListTag drawListTag);

            /**
             * Finds the tag associated with the provided name (case sensitive). If a tag exists with that name, the tag
             * is returned. The reference count is NOT incremented on success; ownership is not passed to the user. If
             * the tag does not exist, a null tag is returned.
             */
            DrawListTag FindTag(const Name& drawListName) const;

            /**
             * Returns the name of the given DrawListTag, or empty string if the tag is not registered.
             */
            Name GetName(DrawListTag tag) const;

            /**
             * Returns the number of allocated tags in the registry.
             */
            size_t GetAllocatedTagCount() const;

        private:
            DrawListTagRegistry() = default;

            struct Entry
            {
                Name m_name;
                size_t m_refCount = 0;
            };

            mutable AZStd::shared_mutex m_mutex;
            AZStd::array<Entry, Limits::Pipeline::DrawListTagCountMax> m_entriesByTag;
            size_t m_allocatedTagCount = 0;
        };
    }
}
