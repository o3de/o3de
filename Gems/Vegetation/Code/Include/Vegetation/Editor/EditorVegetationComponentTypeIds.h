/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace Vegetation
{
    // Vegetation Areas
    static const char* EditorAreaBlenderComponentTypeId = "{374A5C69-A252-4C4B-AE10-A673EF7AFE82}";
    static const char* EditorBlockerComponentTypeId = "{9E765835-9CEB-4AEC-A913-787D3D21451D}";
    static const char* EditorMeshBlockerComponentTypeId = "{130F5DFF-EF6F-4B37-8717-194876DE12DB}";
    static const char* EditorSpawnerComponentTypeId = "{DD96FD51-A86B-48BC-A6AB-89183B538269}";

    // Vegetation Area Filters
    static const char* EditorDistanceBetweenFilterComponentTypeId = "{78DE1245-7023-40D6-B365-CC45EB4CE622}";
    static const char* EditorDistributionFilterComponentTypeId = "{8EDD1DA2-B597-4BCE-9285-C68886504EC7}";
    static const char* EditorShapeIntersectionFilterComponentTypeId = "{8BCE1190-6681-4C27-834A-AFFC8FBBDCD1}";
    static const char* EditorSurfaceAltitudeFilterComponentTypeId = "{CD722D14-9C3B-4F89-B695-65B584279EB3}";
    static const char* EditorSurfaceMaskDepthFilterComponentTypeId = "{A5441713-89DF-49C1-BA4E-3429FF23B43F}";
    static const char* EditorSurfaceMaskFilterComponentTypeId = "{D2F223B4-60BE-4AC5-A1AA-260B91119918}";
    static const char* EditorSurfaceSlopeFilterComponentTypeId = "{5130DA4B-6586-4249-9B86-6496EB2B1A78}";

    // Vegetation Area Modifiers
    static const char* EditorPositionModifierComponentTypeId = "{E1A2D544-B54A-437F-A40D-1FA5C5999D1C}";
    static const char* EditorRotationModifierComponentTypeId = "{6E4B91BC-DAD7-4630-A78C-261D96EEA979}";
    static const char* EditorScaleModifierComponentTypeId = "{D2391F8A-BB54-463E-9691-9290A802C6DE}";
    static const char* EditorSlopeAlignmentModifierComponentTypeId = "{B0C62968-562B-4A8C-9969-E2AAB5379F66}";

    // Vegetation Area Selectors
    static const char* EditorDescriptorWeightSelectorComponentTypeId = "{0FB90550-149B-4E05-B22C-2753F6526E97}";
}
