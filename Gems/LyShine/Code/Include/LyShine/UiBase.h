/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <platform.h>
#include <algorithm>
#include <Cry_Math.h>
#include <Cry_Color.h>
#include <ISystem.h>
#include <CryCommon/LegacyAllocator.h>

#include <AzCore/std/string/string.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

// This is a workaround for AZCore including WinUser.h which defines DrawText to be DrawTextA
#ifdef DrawText
    #undef DrawText
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Common types used across the LyShine UI system
namespace LyShine
{
    //! used for UI element names and canvas names, should be the same as the AZ::Entity name type
    typedef AZStd::string NameType;

    //! Used for pathnames for UI canvas and prefab files
    typedef AZStd::string PathnameType;

    //! USed for our 32-bit id's for canvases and elements. Only used so FlowGraph can reference these things
    //! These should go away once FlowGraph can used AZ::EntityIds
    typedef unsigned int CanvasId;
    typedef unsigned int ElementId;

    //! Used for UI action names
    typedef AZStd::string ActionName;

    //! Used for test strings in  UI text elements
    typedef AZStd::string StringType;    // not yet decided if we should use wchar_t or UTF8

    //! Used for passing lists of entities
    typedef AZStd::vector<AZ::Entity*, AZ::StdLegacyAllocator> EntityArray;

    enum class BlendMode
    {
        Normal,     //!< interpolate from dest color to source color according to source alpha
        Add,        //!< source color * source alpha is added to dest color (Linear Dodge)
        Screen,     //!< as close as we can get to Photoshop Screen
        Darken,     //!< like Photoshop darken
        Lighten,    //!< like Photoshop lighten
    };

    // used to map old EntityId's to new EntityId's when generating new ids for a paste or prefab
    typedef AZStd::unordered_map<AZ::EntityId, AZ::EntityId> EntityIdMap;
};


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(ColorF, "{63782551-A309-463B-A301-3A360800DF1E}");
    AZ_TYPE_INFO_SPECIALIZE(ColorB, "{6F0CC2C0-0CC6-4DBF-9297-B043F270E6A4}");
    AZ_TYPE_INFO_SPECIALIZE(Vec4, "{CAC9510C-8C00-41D4-BC4D-2C6A8136EB30}");
} // namespace AZ

