/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/std/tuple.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AzPhysics
{
    using SceneIndex = AZ::s8;

    //! A handle to a Scene within the physics simulation.
    //! A SceneHandle is a tuple of a Crc of the scenes name and the index in the Scene list.
    using SceneHandle = AZStd::tuple<AZ::Crc32, SceneIndex>;
    

    //! Helper for retrieving the values from the SceneHandle tuple.
    //! Example usage
    //! @code{ .cpp }
    //! SceneHandle someHandle;
    //! AZ::Crc32 handleCrc = AZStd::get<SceneHandleValues::Crc>(someHandle);
    //! SceneIndex index = AZStd::get<SceneHandleValues::Index>(someHandle);
    //! @endcode
    enum SceneHandleValues
    {
        Crc = 0,
        Index
    };

    static constexpr SceneHandle InvalidSceneHandle = { AZ::Crc32(), -1 };

    //! Ease of use type for referencing a List of SceneHandle objects.
    using SceneHandleList = AZStd::vector<SceneHandle>;
}
