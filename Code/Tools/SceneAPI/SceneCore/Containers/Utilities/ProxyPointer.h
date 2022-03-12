#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
