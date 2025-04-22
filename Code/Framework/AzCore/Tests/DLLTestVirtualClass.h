/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_UNITTEST_DLLTESTVIRTUALCLASS_H
#define AZCORE_UNITTEST_DLLTESTVIRTUALCLASS_H

namespace UnitTest
{
    class DLLTestVirtualClass
    {
    public:
        DLLTestVirtualClass()
            : m_data(1) {}
        virtual ~DLLTestVirtualClass() {}

        int m_data;
    };
}

#endif // AZCORE_UNITTEST_DLLTESTVIRTUALCLASS_H
