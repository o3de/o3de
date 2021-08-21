/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ScriptCanvas
{
    class SerializationListener
    {
    public:
        AZ_RTTI(SerializationListener, "{CA4EE281-30B3-4928-BCD8-9305CE75E463}");
        virtual ~SerializationListener() {}

        virtual void OnSerialize() {}

        virtual void OnDeserialize() {}
    };

    using SerializationListeners = AZStd::vector<SerializationListener*>;
}
