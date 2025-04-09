/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/std/string/string.h>

namespace GraphModelIntegration
{
    namespace Attributes
    {
        const static AZ::Crc32 TitlePaletteOverride = AZ_CRC_CE("TitlePaletteOverride");
    }

    class Helpers
    {
    public:
        //! Helper method to retrieve the TitlePaletteOverride attribute (if exists) set on
        //! a given AZ type class, that will also check any base class that it is derived from
        static AZStd::string GetTitlePaletteOverride(void* nodePtr, const AZ::TypeId& typeId);

    private:
        //! Callback method needed for IRttiHelper::EnumHierarchy that gets invoked at every level
        //! allowing us to build a list of each TypeId it encounters
        static void RttiEnumHierarchyHelper(const AZ::TypeId& typeId, void* userData);
    };
}
