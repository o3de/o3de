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
#include "AzCore/Name/Name.h"

//struct ViewBookmarkPosition
//{
//    AZ_CLASS_ALLOCATOR(ViewBookmarkPosition, AZ::SystemAllocator, 0);
//    AZ_RTTI(ViewBookmarkPosition, "{4BA6ABA5-F1F5-4DC3-B5B0-F7B314BCC448}");
//
//    static void Reflect(AZ::ReflectContext* context)
//    {
//        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
//        {
//            serializeContext->Class<ViewBookmarkPosition>()
//                ->Version(0)
//                ->Field("x", &ViewBookmarkPosition::m_xPos)
//                ->Field("y", &ViewBookmarkPosition::m_yPos)
//                ->Field("z", &ViewBookmarkPosition::m_zPos);
//
//        }
//    }
//
//    float m_xPos;
//    float m_yPos;
//    float m_zPos;
//};

struct ViewBookmark
{
    AZ_CLASS_ALLOCATOR(ViewBookmark, AZ::SystemAllocator, 0);
    AZ_RTTI(ViewBookmark, "{522A38D9-6FFF-4B96-BECF-B4D0F7ABCD25}");

    //static void Reflect(AZ::ReflectContext* context)
    //{
    //    if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
    //    {
    //        serializeContext->Class<ViewBookmark>()
    //            ->Version(0)
    //            ->Field("x", &ViewBookmark::m_xPos)
    //            ->Field("y", &ViewBookmark::m_yPos)
    //            ->Field("z", &ViewBookmark::m_zPos);
    //    }
    //}

    float m_xPos;
    float m_yPos;
    float m_zPos;
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
