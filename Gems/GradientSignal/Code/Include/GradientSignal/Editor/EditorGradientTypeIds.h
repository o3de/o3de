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

#include <AzCore/RTTI/TypeInfo.h>

namespace GradientSignal
{
    // Gradients
    static const AZ::TypeId EditorConstantGradientComponentTypeId = "{A878736F-39B0-482B-89B8-FD0F083E4D88}";
    static const AZ::TypeId EditorImageGradientComponentTypeId = "{4E20A1C2-CEFF-4E9F-BEFF-C18257DFEC38}";
    static const AZ::TypeId EditorPerlinGradientComponentTypeId = "{99C0B5DF-9BB4-44EE-9BDF-1141ECE26D19}";
    static const AZ::TypeId EditorRandomGradientComponentTypeId = "{B590FB2E-33ED-4F7F-AA12-1FE76278D880}";
    static const AZ::TypeId EditorShapeAreaFalloffGradientComponentTypeId = "{473AAC7F-990C-4761-866A-6F451C92DB3B}";
    static const AZ::TypeId EditorSurfaceAltitudeGradientComponentTypeId = "{41A7009A-53E4-44B3-B23D-7D02266A3D9D}";
    static const AZ::TypeId EditorSurfaceMaskGradientComponentTypeId = "{B5395FBC-FC19-4259-8A57-00C92FE1A2A2}";
    static const AZ::TypeId EditorSurfaceSlopeGradientComponentTypeId = "{915D3430-BB7D-47F2-ABE0-3B562636F7A9}";

    // Gradient Modifiers
    static const AZ::TypeId EditorDitherGradientComponentTypeId = "{D20E867C-692A-4056-B5AC-B372CFBAC524}";
    static const AZ::TypeId EditorMixedGradientComponentTypeId = "{90642402-6C5F-4C4D-B1F0-8C0F242A2716}";
    static const AZ::TypeId EditorInvertGradientComponentTypeId = "{BC63EA1C-E06B-4771-BF5D-4AE2B54AE494}";
    static const AZ::TypeId EditorLevelsGradientComponentTypeId = "{227E7DB6-E1FE-438E-B837-2D4604DA1DEC}";
    static const AZ::TypeId EditorPosterizeGradientComponentTypeId = "{CC21E213-19B6-46D6-B774-40F20B0950D7}";
    static const AZ::TypeId EditorSmoothStepGradientComponentTypeId = "{3CDEA65E-519A-468E-884E-3F047FD7791F}";
    static const AZ::TypeId EditorThresholdGradientComponentTypeId = "{1AB9BC5A-8EE3-4527-87AD-7E34AC438879}";
}
