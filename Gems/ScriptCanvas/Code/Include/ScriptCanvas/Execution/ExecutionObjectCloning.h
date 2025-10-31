/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>

/**
* The EditContext and other facilities give script users, especially ScriptCanvas users the ability to create and initialize objects
* that cannot be easily created with a sequence of C++ code. Some Editor facilities give users access to private variables in objects
* that cannot be modified or initialized directly via their public C++ interface.
*
* This ExecutionCloneSource objects exists to facilitate fast construction and initialization of the such objects when needed to execute
* compiled ScriptCanvas graphs properly.
*/

namespace ScriptCanvas
{
    namespace Execution
    {
        class CloneSource final
        {
        public:
            AZ_TYPE_INFO(CloneSource, "{C2E49697-AC80-4024-A7F8-99AFD4858EDA}");
            AZ_CLASS_ALLOCATOR(CloneSource, AZ::SystemAllocator);

            CloneSource(const AZ::BehaviorClass& bcClass, void* source);

            struct Result
            {
                void* object;
                AZ::TypeId typeId;
            };

            Result Clone() const;

        private:
            void* m_source = nullptr;
            const AZ::BehaviorClass* m_class = nullptr;
        };
    }
}
