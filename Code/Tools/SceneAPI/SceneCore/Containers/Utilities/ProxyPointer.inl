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

namespace AZ
{
    namespace SceneAPI
    {
        namespace Containers
        {
            template<typename Type>
            ProxyPointer<Type>::ProxyPointer(const Type& value)
                : m_value(value)
            {
            }

            template<typename Type>
            Type& ProxyPointer<Type>::operator*()
            {
                return m_value;
            }

            template<typename Type>
            Type* ProxyPointer<Type>::operator->()
            {
                return &m_value;
            }

            template<typename Type>
            ProxyPointer<Type&>::ProxyPointer(Type& value)
                : m_value(value)
            {
            }

            template<typename Type>
            Type& ProxyPointer<Type&>::operator*()
            {
                return m_value;
            }

            template<typename Type>
            Type* ProxyPointer<Type&>::operator->()
            {
                return &m_value;
            }
        } // Containers
    } // SceneAPI
} // AZ
