/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace UnitTest
{
    // This fixture provides a functional serialize context and allocators.
    class SerializeContextFixture : public LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_serializeContext = aznew AZ::SerializeContext(true, true);
        }

        void TearDown() override
        {
            delete m_serializeContext;
            m_serializeContext = nullptr;

            LeakDetectionFixture::TearDown();
        }

    protected:
        AZ::SerializeContext* m_serializeContext = nullptr;
    };

    /*
     * Scoped RAII class automatically invokes the supplied reflection functions and reflects them to the supplied SerializeContext
     * On Destruction the serialize context is set to remove reflection and the reflection functions are invoked to to unreflect
     * them from the SerializeContext
     */
    class ScopedSerializeContextReflector
    {
    public:
        using ReflectCallable = AZStd::function<void(AZ::SerializeContext*)>;

        ScopedSerializeContextReflector(AZ::SerializeContext& serializeContext, std::initializer_list<ReflectCallable> reflectFunctions)
            : m_serializeContext(serializeContext)
            , m_reflectFunctions(reflectFunctions)
        {
            bool isCurrentlyRemovingReflection = m_serializeContext.IsRemovingReflection();
            if (isCurrentlyRemovingReflection)
            {
                m_serializeContext.DisableRemoveReflection();
            }
            for (ReflectCallable& reflectFunction : m_reflectFunctions)
            {
                if (reflectFunction)
                {
                    reflectFunction(&m_serializeContext);
                }
            }
            if (isCurrentlyRemovingReflection)
            {
                m_serializeContext.EnableRemoveReflection();
            }
        }

        ~ScopedSerializeContextReflector()
        {
            // Unreflects reflected functions in reverse order
            bool isCurrentlyRemovingReflection = m_serializeContext.IsRemovingReflection();
            if (!isCurrentlyRemovingReflection)
            {
                m_serializeContext.EnableRemoveReflection();
            }
            for (auto reflectFunctionIter = m_reflectFunctions.rbegin(); reflectFunctionIter != m_reflectFunctions.rend(); ++reflectFunctionIter)
            {
                ReflectCallable& reflectFunction = *reflectFunctionIter;
                if (reflectFunction)
                {
                    reflectFunction(&m_serializeContext);
                }
            }
            if (!isCurrentlyRemovingReflection)
            {
                m_serializeContext.DisableRemoveReflection();
            }
        }

    private:
        AZ::SerializeContext& m_serializeContext;
        AZStd::vector<ReflectCallable> m_reflectFunctions;
    };
}
