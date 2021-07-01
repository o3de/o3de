/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <vector>

namespace News
{
    //! A simple unique id generator.
    class UidGenerator
    {
    public:
        UidGenerator();
        int GenerateUid();
        int AddUid(int uid);
        void RemoveUid(int uid);
        void Clear();

    private:
        std::vector<int> m_uids;
    };
}
