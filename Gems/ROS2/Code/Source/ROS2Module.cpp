/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <ROS2ModuleInterface.h>
#include <ROS2SystemComponent.h>

namespace ROS2
{
    class ROS2Module
        : public ROS2ModuleInterface
    {
    public:
        AZ_RTTI(ROS2Module, "{e23a1379-787c-481e-ad83-c0e04a3d06fe}", ROS2ModuleInterface);
        AZ_CLASS_ALLOCATOR(ROS2Module, AZ::SystemAllocator, 0);
    };
}// namespace ROS2

AZ_DECLARE_MODULE_CLASS(Gem_ROS2, ROS2::ROS2Module)
