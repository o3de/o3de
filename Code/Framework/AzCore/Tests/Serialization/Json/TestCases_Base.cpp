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

#include <AzCore/Serialization/SerializeContext.h>
#include <Tests/Serialization/Json/TestCases_Base.h>


namespace JsonSerializationTests
{
    // BaseClass

    void BaseClass::add_ref()
    {
        ++m_refCount;
    }

    void BaseClass::release()
    {
        if (--m_refCount == 0)
        {
            delete this;
        }
    }

    bool BaseClass::Equals(const BaseClass& rhs, bool) const
    {
        return m_baseVar == rhs.m_baseVar;
    }

    void BaseClass::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context)
    {
        context->Class<BaseClass>()->Field("base_var", &BaseClass::m_baseVar);
    }


    // BaseClass2

    void BaseClass2::add_ref()
    {
        ++m_refCount;
    }

    void BaseClass2::release()
    {
        if (--m_refCount == 0)
        {
            delete this;
        }
    }

    bool BaseClass2::Equals(const BaseClass2& rhs, bool) const
    {
        return
            m_base2Var1 == rhs.m_base2Var1 &&
            m_base2Var2 == rhs.m_base2Var2 &&
            m_base2Var3 == rhs.m_base2Var3;
    }

    void BaseClass2::Reflect(AZStd::unique_ptr<AZ::SerializeContext>& context)
    {
        context->Class<BaseClass2>()
            ->Field("base2_var1", &BaseClass2::m_base2Var1)
            ->Field("base2_var2", &BaseClass2::m_base2Var2)
            ->Field("base2_var3", &BaseClass2::m_base2Var3);
    }
} // namespace JsonSerializationTests
