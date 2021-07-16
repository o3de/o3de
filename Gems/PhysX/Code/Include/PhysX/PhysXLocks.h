/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>

namespace PhysX
{
    namespace Internal
    {
        template<bool readLock = false>
        struct PhysXLock final
        {
            PhysXLock(physx::PxScene& scene, const char* file = __FILE__, uint32_t line = __LINE__)
                : m_scene(&scene)
                , m_file(file)
                , m_line(line)
            {
                lock();
            }

            PhysXLock(physx::PxScene* scene, const char* file = __FILE__, uint32_t line = __LINE__)
                : m_file(file)
                , m_line(line)
            {
                if (scene != nullptr)
                {
                    m_scene = scene;
                    lock();
                }
            }

            ~PhysXLock()
            {
                if (m_scene != nullptr)
                {
                    unlock();
                }
            }

        private:
#ifdef PHYSX_ENABLE_MULTI_THREADING
            void lock()
            {
                if constexpr (readLock)
                {
                    m_scene->lockRead(m_file, static_cast<physx::PxU32>(m_line));
                }
                else
                {
                    m_scene->lockWrite(m_file, static_cast<physx::PxU32>(m_line));
                }
            }

            void unlock()
            {
                if constexpr (readLock)
                {
                    m_scene->unlockRead();
                }
                else
                {
                    m_scene->unlockWrite();
                }
            }
#else
            void lock()
            {
                //no-op
            }

            void unlock()
            {
                //no-op
            }
#endif // PHYSX_ENABLE_MULTI_THREADING

            physx::PxScene* m_scene = nullptr;
            const char* m_file;
            uint32_t m_line;
        };
    }

}

#ifdef PHYSX_ENABLE_MULTI_THREADING
    #define PHYSX_SCENE_READ_LOCK(scene)  PhysX::Internal::PhysXLock<true> scopedLock(scene, __FILE__, __LINE__);
    #define PHYSX_SCENE_WRITE_LOCK(scene) PhysX::Internal::PhysXLock<false> scopedLock(scene, __FILE__, __LINE__);
#else
    #define PHYSX_SCENE_READ_LOCK(scene) ((void)0)
    #define PHYSX_SCENE_WRITE_LOCK(scene) ((void)0)
#endif //PHYSX_ENABLE_MULTI_THREADING
