/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>

namespace RagdollTestData
{
    //! The number of nodes in the test ragdoll configuration.
    static const size_t NumNodes = 22;

    //! Node positions for a T pose for the test ragdoll configuration.
    static const AZ::Vector3 NodePositions[] =
    {
        AZ::Vector3(0.0f, 0.0f, 1.014454f),
        AZ::Vector3(-0.152039f, -0.056946f, 1.368118f),
        AZ::Vector3(0.0f, 0.0f, 1.055454f),
        AZ::Vector3(0.0f, 0.0f, 1.117004f),
        AZ::Vector3(0.0f, 0.0f, 1.193852f),
        AZ::Vector3(0.0f, 0.0f, 1.279282f),
        AZ::Vector3(-0.049042f, -0.055115f, 1.373142f),
        AZ::Vector3(-0.492157f, 0.017487f, 1.047424f),
        AZ::Vector3(-0.347839f, -0.093597f, 1.183502f),
        AZ::Vector3(0.0f, -0.023651f, 1.385757f),
        AZ::Vector3(0.0f, 0.003479f, 1.52404f),
        AZ::Vector3(0.0f, -0.01181f, 1.446209f),
        AZ::Vector3(0.049042f, -0.055115f, 1.373138f),
        AZ::Vector3(0.492157f, 0.017487f, 1.047421f),
        AZ::Vector3(0.152039f, -0.056946f, 1.368118f),
        AZ::Vector3(0.347839f, -0.093597f, 1.183498f),
        AZ::Vector3(-0.09021f, 0.00061f, 0.970799f),
        AZ::Vector3(-0.11261f, -0.022736f, 0.09676f),
        AZ::Vector3(-0.10321f, -0.000488f, 0.463436f),
        AZ::Vector3(0.09021f, 0.00061f, 0.970799f),
        AZ::Vector3(0.11261f, -0.022736f, 0.09676f),
        AZ::Vector3(0.10321f, -0.000488f, 0.463436f)
    };

    //! Node orientations for a T pose for the test ragdoll configuration.
    static const AZ::Quaternion NodeOrientations[] =
    {
        AZ::Quaternion(0.0f, -0.706934214f, 0.0f, 0.706934214f),
        AZ::Quaternion(0.0628239065f, -0.927271962f, 0.0249484479f, -0.368235558f),
        AZ::Quaternion(0.0f, -0.707106829f, 0.0f, 0.707106829f),
        AZ::Quaternion(0.0f, -0.707106829f, 0.0f, 0.707106829f),
        AZ::Quaternion(0.0f, -0.707106829f, 0.0f, 0.707106829f),
        AZ::Quaternion(0.0f, -0.707106829f, 0.0f, 0.707106829f),
        AZ::Quaternion(0.0105714500f, -0.999943256f, -1.44154765e-05f, 0.00136360526f),
        AZ::Quaternion(-0.234650344f, -0.899288297f, -0.0931836665f, -0.357122779f),
        AZ::Quaternion(-0.234650344f, -0.899288297f, -0.0931836665f, -0.357122779f),
        AZ::Quaternion(-0.0684024617f, -0.703790545f, 0.0684024617f, 0.703790545f),
        AZ::Quaternion(-0.0684024617f, -0.703790545f, 0.0684024617f, 0.703790545f),
        AZ::Quaternion(-0.0684024617f, -0.703790545f, 0.0684024617f, 0.703790545f),
        AZ::Quaternion(-0.00136360526f, -1.44154765e-05f, 0.999943256f, 0.0105714500f),
        AZ::Quaternion(0.357122779f, -0.0931836665f, 0.899288297f, -0.234650359f),
        AZ::Quaternion(0.370845109f, 0.0276379380f, 0.925627708f, 0.0701668411f),
        AZ::Quaternion(0.357122779f, -0.0931836665f, 0.899288297f, -0.234650359f),
        AZ::Quaternion(0.000775639550f, -0.716108859f, 0.000756012159f, -0.697987854f),
        AZ::Quaternion(-0.346383005f, -0.613959551f, -0.348518163f, -0.617743969f),
        AZ::Quaternion(0.0217111148f, -0.715780079f, 0.0211617611f, -0.697667360f),
        AZ::Quaternion(0.697988033f, 0.000756011985f, 0.716108680f, 0.000775639724f),
        AZ::Quaternion(0.617747664f, -0.348512053f, 0.613956034f, -0.346388757f),
        AZ::Quaternion(0.697667360f, 0.0211687498f, 0.715780079f, 0.0217043031f)
    };

    //! Parent indices for each node in the test ragdoll configuration.
    static const size_t ParentIndices[] = { SIZE_MAX, 6, 0, 2, 3, 4, 5, 8, 1, 5, 11, 9, 5, 15, 12, 14, 0, 18, 16, 0, 21, 19 };
} // namespace RagdollTestData
