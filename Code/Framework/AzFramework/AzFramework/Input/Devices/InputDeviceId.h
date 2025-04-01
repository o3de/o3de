/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/string/fixed_string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Class that identifies a specific input device
    class InputDeviceId
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constants
        static constexpr int MAX_NAME_LENGTH = 64;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceId, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_TYPE_INFO(InputDeviceId, "{E58630A4-D380-4289-AA29-83300636A954}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] name Name of the input device (will be truncated if exceeds MAX_NAME_LENGTH)
        //! \param[in] index Index of the input device (optional)
        explicit constexpr InputDeviceId(AZStd::string_view name, AZ::u32 index = 0)
            : m_name(name.substr(0, MAX_NAME_LENGTH))
            , m_crc32(name.substr(0, MAX_NAME_LENGTH))
            , m_index(index)
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying and moving
        AZ_DEFAULT_COPY_MOVE(InputDeviceId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputDeviceId() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input device's name
        //! \return Name of the input device
        constexpr const char* GetName() const
        {
            return m_name.c_str();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the crc32 of the input device's name
        //! \return crc32 of the input device name
        constexpr const AZ::Crc32& GetNameCrc32() const
        {
            return m_crc32;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input device's index. Used for differentiating between multiple instances
        //! of the same device type, regardless of whether the device has a local user id assigned.
        //! In some cases the device index and local user are the same, but this cannot be assumed.
        //! For example, by default the engine supports up to four gamepad devices that are created
        //! at startup using indicies 0->3. As gamepads connect/disconnect at runtime we assign the
        //! appropriate (system dependent) local user id (see InputDevice::GetAssignedLocalUserId).
        //! \return Index of the input device
        constexpr AZ::u32 GetIndex() const
        {
            return m_index;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Equality comparison operator
        //! \param[in] other Another instance of the class to compare for equality
        constexpr bool operator==(const InputDeviceId& other) const
        {
            return (m_crc32 == other.m_crc32) && (m_index == other.m_index);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inequality comparison operator
        //! \param[in] other Another instance of the class to compare for inequality
        constexpr bool operator!=(const InputDeviceId& other) const
        {
            return !(*this == other);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Less than comparison operator
        //! \param[in] other Another instance of the class to compare
        constexpr bool operator<(const InputDeviceId& other) const
        {
            if (m_index == other.m_index)
            {
                return m_crc32 < other.m_crc32;
            }
            return m_index < other.m_index;
        }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::fixed_string<MAX_NAME_LENGTH> m_name; //!< Name of the input device
        AZ::Crc32 m_crc32; //!< Crc32 of the input device name
        AZ::u32 m_index; //!< Index of the input device
    };
} // namespace AzFramework

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZStd
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Hash structure specialization for InputDeviceId
    template<> struct hash<AzFramework::InputDeviceId>
    {
        inline size_t operator()(const AzFramework::InputDeviceId& inputDeviceId) const
        {
            size_t hashValue = inputDeviceId.GetNameCrc32();
            AZStd::hash_combine(hashValue, inputDeviceId.GetIndex());
            return hashValue;
        }
    };
} // namespace AZStd
