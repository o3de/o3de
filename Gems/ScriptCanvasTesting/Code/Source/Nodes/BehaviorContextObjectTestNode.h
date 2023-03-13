/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Attributes.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvasTestingNodes
{
    //! This object is used to test the use of BehaviorContext classes
    class BehaviorContextObjectTest
    {
    public:
        AZ_RTTI(BehaviorContextObjectTest, "{FF687BD7-42AA-44C4-B3AB-79353E8C6CCF}");
        AZ_CLASS_ALLOCATOR(BehaviorContextObjectTest, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        BehaviorContextObjectTest()
        {
        }

        virtual ~BehaviorContextObjectTest() {}

        void SetString(AZStd::string string)
        {
            m_string = string;
        }

        AZStd::string GetString() const
        {
            return m_string;
        }

    private:
        AZStd::string m_name;
        AZStd::string m_string;

    };
}
