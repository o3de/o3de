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
        AZ_CLASS_ALLOCATOR(InputChannelId, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_TYPE_INFO(InputChannelId, "{7004B466-F6B4-41EF-AFFD-96A456121271}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Reflection
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] name Name of the input channel (will be ignored if exceeds MAX_NAME_LENGTH)
        explicit constexpr InputChannelId(AZStd::string_view name = "")
            : m_name(name)
            , m_crc32(name)
        {
        }

        constexpr InputChannelId(const InputChannelId& other) = default;
        constexpr InputChannelId(InputChannelId&& other) = default;
        constexpr InputChannelId& operator=(const InputChannelId& other)
        {
            m_name = other.m_name;
            m_crc32 = other.m_crc32;
            return *this;
        }
        constexpr InputChannelId& operator=(InputChannelId&& other)
        {
            m_name = AZStd::move(other.m_name);
            m_crc32 = AZStd::move(other.m_crc32);
            other.m_crc32 = 0;
            return *this;
        }
        ~InputChannelId() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the input channel's name
        //! \return Name of the input channel
        const char* GetName() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Access to the crc32 of the input channel's name
        //! \return crc32 of the input channel name
        const AZ::Crc32& GetNameCrc32() const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        ///@{
        //! Equality comparison operator
        //! \param[in] other Another instance of the class to compare for equality
        bool operator==(const InputChannelId& other) const;
        bool operator!=(const InputChannelId& other) const;
        ///@}

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::fixed_string<MAX_NAME_LENGTH> m_name; //!< Name of the input channel
        AZ::Crc32 m_crc32;                  //!< Crc32 of the input channel
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
