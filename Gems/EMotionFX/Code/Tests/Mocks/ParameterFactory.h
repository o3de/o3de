/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    class ParameterFactory;

    namespace Internal
    {
        ParameterFactory* GetParameterFactory();
    } // namespace Internal

    class ParameterFactory
    {
    public:
        MOCK_CONST_METHOD1(CreateImpl, Parameter*(const AZ::TypeId& type));

        static Parameter* Create(const AZ::TypeId& type) { return Internal::GetParameterFactory()->CreateImpl(type); }
    };

    namespace Internal
    {
        ParameterFactory* GetParameterFactory()
        {
            static ParameterFactory factory;
            return &factory;
        }
    } // namespace Internal
} // namespace EMotionFX
