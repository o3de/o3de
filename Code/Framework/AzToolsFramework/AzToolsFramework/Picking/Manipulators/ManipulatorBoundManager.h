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

#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Picking/ContextBoundAPI.h>

namespace AzToolsFramework
{
    namespace Picking
    {
        class BoundShapeInterface;

        /**
         * Handle creating, destroying and storing all active manipulator
         * bounds for performing raycasts/picking against.
         */
        class ManipulatorBoundManager
        {
        public:
            AZ_CLASS_ALLOCATOR(ManipulatorBoundManager, AZ::SystemAllocator, 0);

            ManipulatorBoundManager() = default;
            ManipulatorBoundManager(const ManipulatorBoundManager&) = delete;
            ManipulatorBoundManager& operator=(const ManipulatorBoundManager&) = delete;
            ~ManipulatorBoundManager() = default;

            RegisteredBoundId UpdateOrRegisterBound(
                const BoundRequestShapeBase& shapeData, RegisteredBoundId id);
            void UnregisterBound(RegisteredBoundId boundId);
            void SetBoundValidity(RegisteredBoundId boundId, bool valid);
            void RaySelect(RaySelectInfo &rayInfo);

        private:
            AZStd::shared_ptr<BoundShapeInterface> CreateShape(
                const BoundRequestShapeBase& ptrShape, RegisteredBoundId id);
            void DeleteShape(const BoundShapeInterface* boundShape);

            AZStd::unordered_map<RegisteredBoundId, AZStd::shared_ptr<BoundShapeInterface>> m_boundIdToShapeMap;
            AZStd::vector<AZStd::shared_ptr<BoundShapeInterface>> m_bounds; ///< All current manipulator bounds.

            RegisteredBoundId m_nextBoundId = RegisteredBoundId(1); ///< Next bound id to use when a bound is registered.
        };
    } // namespace Picking
} // namespace AzToolsFramework