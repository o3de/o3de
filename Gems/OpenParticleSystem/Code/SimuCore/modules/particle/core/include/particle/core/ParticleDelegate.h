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
    class ParticleDelegate<void(uint8_t*, Args...)> {
    public:
        using FuncType = void(uint8_t*, Args&&...);

        ParticleDelegate() noexcept = default;
        ~ParticleDelegate() noexcept = default;

        template<auto Func, typename Data>
        void Connect() noexcept
        {
            size = sizeof(Data);
            func = [](uint8_t* rawData, Args&&... args) {
                Func(reinterpret_cast<Data*>(rawData), std::forward<Args>(args)...);
            };
        }

        void operator()(uint8_t* data, Args&&... args) const
        {
            if (func != nullptr) {
                func(data, std::forward<Args>(args)...);
            }
        }
        uint32_t size = 0;
        FuncType* func = nullptr;
    };

    template<typename T>
    struct DataArgs {
    };
}
