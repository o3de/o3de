/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace WhiteBox
{
    struct WhiteBoxMesh;
    struct WhiteBoxMeshHandle;

    //! Converts a WhiteBoxMeshHandle into a WhiteBoxMesh pointer.
    WhiteBoxMesh* WhiteBoxMeshFromHandle(const WhiteBoxMeshHandle& whiteBoxMeshHandle);

    //! Exposes all of the White Box methods to Behavior Context for use in scripting.
    void Reflect(AZ::ReflectContext* context);
} // namespace WhiteBox
