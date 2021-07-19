/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <ACETypes.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    class IAudioConnection
    {
    public:
        IAudioConnection(CID id)
            : m_id(id)
        {
        }

        virtual ~IAudioConnection() {}

        CID GetID() const
        {
            return m_id;
        }

        virtual bool HasProperties()
        {
            return false;
        }

    private:
        CID m_id;
    };

} // namespace AudioControls
