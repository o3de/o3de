/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <cstdint>
#include <string_view>

namespace SimuCore::ParticleCore {
    template<typename... Args>
    class ParticleDelegate;

    template<typename... Args>
    class ParticleDelegate<void(AZ::u8*, Args...)> {
    public:
        using FuncType = void(AZ::u8*, Args&&...);

        ParticleDelegate() noexcept = default;
        ~ParticleDelegate() noexcept = default;

        template<auto Func, typename Data>
        void Connect() noexcept
        {
            size = sizeof(Data);
            func = [](AZ::u8* rawData, Args&&... args) {
                Func(reinterpret_cast<Data*>(rawData), std::forward<Args>(args)...);
            };
        }

        void operator()(AZ::u8* data, Args&&... args) const
        {
            if (func != nullptr) {
                func(data, std::forward<Args>(args)...);
            }
        }
        AZ::u32 size = 0;
        FuncType* func = nullptr;
    };

    template<typename T>
    struct DataArgs {
    };
}
