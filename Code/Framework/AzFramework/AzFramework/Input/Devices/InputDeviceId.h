/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/hash.h>

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
        static const int NAME_BUFFER_SIZE = 64;
        static const int MAX_NAME_LENGTH = NAME_BUFFER_SIZE - 1;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceId, AZ::SystemAllocator, 0);

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
        explicit InputDeviceId(const char* name, AZ::u32 index = 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Copy constructor
        //! \param[in] other Another instance of the class to copy from
        InputDeviceId(const InputDeviceId& other);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Copy assignment operator
        //! \param[in] other Another instance of the class to copy from
        InputDeviceId& operator=(const InputDeviceId& other);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputDeviceId() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input device's name
        //! \return Name of the input device
        const char* GetName() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the crc32 of the input device's name
        //! \return crc32 of the input device name
        const AZ::Crc32& GetNameCrc32() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input device's index. Used for differentiating between multiple instances
        //! of the same device type, regardless of whether the device has a local user id assigned.
        //! In some cases the device index and local user are the same, but this cannot be assumed.
        //! For example, by default the engine supports up to four gamepad devices that are created
        //! at startup using indicies 0->3. As gamepads connect/disconnect at runtime we assign the
        //! appropriate (system dependent) local user id (see InputDevice::GetAssignedLocalUserId).
        //! \return Index of the input device
        AZ::u32 GetIndex() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Equality comparison operator
        //! \param[in] other Another instance of the class to compare for equality
        bool operator==(const InputDeviceId& other) const;
        bool operator!=(const InputDeviceId& other) const;
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Less than comparison operator
        //! \param[in] other Another instance of the class to compare
        bool operator<(const InputDeviceId& other) const;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        char      m_name[NAME_BUFFER_SIZE]; //!< Name of the input device
        AZ::Crc32 m_crc32;                  //!< Crc32 of the input device
        AZ::u32   m_index;                  //!< Index of the input device
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
