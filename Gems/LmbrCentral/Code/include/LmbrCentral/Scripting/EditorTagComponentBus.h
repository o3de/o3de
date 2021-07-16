/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <LmbrCentral/Scripting/TagComponentBus.h>

namespace LmbrCentral
{
    /**
    * In the editor, the Tag Component consists of strings, and thus if you want to manipulate tags
    * at editor time instead of runtime, you need to use this bus instead (which sets/gets/adds/removes strings)
    * note that the Editor Tag Component will still send the appropriate Tag Added / Tag Removed / other similar messages
    * using the underlying CRC engine system, so you can use that to query tags still.
    */

    using EditorTags = AZStd::vector<AZStd::string>;

    class EditorTagComponentRequests : public AZ::ComponentBus
    {
    public:
        virtual ~EditorTagComponentRequests() = default;

        /// Returns true if the entity has the tag
        virtual bool HasTag(const char* tag) = 0;

        /// Adds the tag to the entity if it didn't already have it
        virtual void AddTag(const char* tag) = 0;

        /// Add a list of tags to the entity if it didn't already have them
        virtual void AddTags(const EditorTags& tags)  { for (const AZStd::string& tag : tags) AddTag(tag.c_str()); }

        /// Removes a tag from the entity if it had it
        virtual void RemoveTag(const  char* tag) = 0;

        /// Removes a list of tags from the entity if it had them
        virtual void RemoveTags(const EditorTags& tags) { for (const AZStd::string& tag : tags) RemoveTag(tag.c_str()); }

        /// Gets the list of tags on the entity
        virtual const EditorTags& GetTags() = 0;
    };
    using EditorTagComponentRequestBus = AZ::EBus<EditorTagComponentRequests>;

} // namespace LmbrCentral
