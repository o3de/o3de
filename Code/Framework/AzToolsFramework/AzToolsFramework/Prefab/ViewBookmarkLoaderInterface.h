/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Interface/Interface.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Name/Name.h>

struct ViewBookmark
{
    AZ_CLASS_ALLOCATOR(ViewBookmark, AZ::SystemAllocator, 0);
    AZ_RTTI(ViewBookmark, "{522A38D9-6FFF-4B96-BECF-B4D0F7ABCD25}");

    ViewBookmark()
        : m_position(AZ::Vector3::CreateZero())
        , m_rotation(AZ::Vector3::CreateZero())
    {
    }

    static void Reflect(AZ::ReflectContext* context)
    {
        //ViewBookmarkVector3::Reflect(context);
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            //serializeContext->RegisterGenericType<ViewBookmarkVector3>();
            serializeContext->Class<ViewBookmark>()
                ->Version(0)
                ->Field("Position", &ViewBookmark::m_position)
                ->Field("Rotation", &ViewBookmark::m_rotation);
        }
    }

    AZ::Vector3 m_position;
    AZ::Vector3 m_rotation;
};

/*!
 * ViewBookmarkIntereface
 * Interface for saving/loading View Bookmarks.
 */
class ViewBookmarkLoaderInterface
{
public:
    AZ_RTTI(ViewBookmarkLoaderInterface, "{71E7E178-4107-4975-A6E6-1C4B005C981A}");

    virtual void SaveBookmarkSettingsFile() = 0;
    virtual bool SaveCustomBookmark(ViewBookmark config) = 0;
};
