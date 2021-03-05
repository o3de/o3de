#pragma once

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
            // Utility class that stores (or references if possible) the given value
            //      but otherwise acts as a pointer. This can be useful for functions
            //      that require returning a pointer but don't store a local copy to
            //      return.

            template<typename Type>
            class ProxyPointer
            {
            public:
                ProxyPointer(const Type& value);
                Type& operator*();
                Type* operator->();

            private:
                Type m_value;
            };

            template<typename Type>
            class ProxyPointer<Type&>
            {
            public:
                ProxyPointer(Type& value);
                Type& operator*();
                Type* operator->();

            private:
                Type& m_value;
            };
        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/Utilities/ProxyPointer.inl>