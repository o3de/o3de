/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/intrusive_refcount.h>
#include <AzCore/std/string/string.h>

namespace ScriptCanvas
{
    enum class ValidationSeverity
    {
        Unknown = -1,
        Error,
        Warning,
        Informative,
    };
    
    class ValidationEvent
        : public AZStd::intrusive_refcount<AZ::u32>
    {
    protected:
        ValidationEvent(const ValidationSeverity& validationType)
            : m_validationType(validationType)
        {
        }

        void SetValidationType(const ValidationSeverity& validationType)
        {
            m_validationType = validationType;
        }

    public:
        AZ_RTTI(ValidationEvent, "{58F76284-987C-4A15-A31B-407475586958}");
        AZ_CLASS_ALLOCATOR(ValidationEvent, AZ::SystemAllocator);

        virtual ~ValidationEvent() = default;

        void SetDescription(AZStd::string_view description)
        {
            m_description = description;
        }

        AZStd::string_view GetDescription() const
        {
            return m_description;
        }

        virtual bool CanAutoFix() const
        {
            return false;
        }
        
        // Returns an Identifier in order to display in the status window
        virtual AZStd::string GetIdentifier() const = 0;

        // Returns a CRC Identifier to do operations on.
        virtual AZ::Crc32 GetIdCrc() const = 0;

        // Returns a tool tip used to describe the event id generically
        virtual AZStd::string_view GetTooltip() const = 0;
        
        // Returns the type of event this is to be interpreted as.
        ValidationSeverity GetSeverity() const
        {
            return m_validationType;
        }
        
    private:
    
        ValidationSeverity  m_validationType;
        AZStd::string       m_description;
    };
    using ValidationPtr = AZStd::intrusive_ptr<ValidationEvent>;
    using ValidationConstPtr = AZStd::intrusive_ptr<const ValidationEvent>;
}
