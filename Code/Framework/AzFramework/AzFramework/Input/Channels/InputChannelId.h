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
    //! Class that identifies a specific input channel
    class InputChannelId
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Constants
        static constexpr int MAX_NAME_LENGTH = 64;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputChannelId, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_TYPE_INFO(InputChannelId, "{7004B466-F6B4-41EF-AFFD-96A456121271}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] name Name of the input channel (will be truncated if exceeds MAX_NAME_LENGTH)
        explicit constexpr InputChannelId(AZStd::string_view name = "")
            : m_name(name.substr(0, MAX_NAME_LENGTH))
            , m_crc32(name.substr(0, MAX_NAME_LENGTH))
        {
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Default copying and moving
        AZ_DEFAULT_COPY_MOVE(InputChannelId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~InputChannelId() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's name
        //! \return Name of the input channel
        constexpr const char* GetName() const
        {
            return m_name.c_str();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the crc32 of the input channel's name
        //! \return crc32 of the input channel name
        constexpr const AZ::Crc32& GetNameCrc32() const
        {
            return m_crc32;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Equality comparison operator
        //! \param[in] other Another instance of the class to compare for equality
        constexpr bool operator==(const InputChannelId& other) const
        {
            return m_crc32 == other.m_crc32;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Inequality comparison operator
        //! \param[in] other Another instance of the class to compare for inequality
        constexpr bool operator!=(const InputChannelId& other) const
        {
            return !(*this == other);
        }

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::fixed_string<MAX_NAME_LENGTH> m_name; //!< Name of the input channel
        AZ::Crc32 m_crc32; //!< Crc32 of the input channel name
    };
} // namespace AzFramework

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZStd
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Hash structure specialization for InputChannelId
    template<> struct hash<AzFramework::InputChannelId>
    {
        inline size_t operator()(const AzFramework::InputChannelId& inputChannelId) const
        {
            return inputChannelId.GetNameCrc32();
        }
    };
} // namespace AZStd
