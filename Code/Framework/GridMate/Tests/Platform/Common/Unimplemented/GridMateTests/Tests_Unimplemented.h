/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace UnitTest
{
    class GridMateTestFixture_Unimplemented
    {
    protected:
        void Construct() {}
        void Destruct() {}
    };

    using GridMateTestFixture_Platform = GridMateTestFixture_Unimplemented;
}

