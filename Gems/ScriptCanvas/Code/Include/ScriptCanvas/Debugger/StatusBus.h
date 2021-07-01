/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvas
{
    class ValidationEvent;

    class ValidationResults
    {
        friend class Graph;
    public:
        using ValidationEventList = AZStd::vector<ValidationConstPtr>;

        ~ValidationResults()
        {
            ClearResults();
        }

        bool HasResults() const
        {
            return !m_validationEvents.empty();
        }

        bool HasErrors() const
        {
            return HasSeverity(ValidationSeverity::Error);
        }

        int ErrorCount() const
        {
            return CountSeverity(ValidationSeverity::Error);
        }

        bool HasWarnings() const
        {
            return HasSeverity(ValidationSeverity::Warning);
        }

        int WarningCount() const
        {
            return CountSeverity(ValidationSeverity::Warning);
        }

        void ClearResults()
        {
            m_validationEvents.clear();
        }

        const ValidationEventList& GetEvents() const
        {
            return m_validationEvents;
        }

        void AddValidationEvent(const ValidationEvent* validationEvent)
        {
            m_validationEvents.push_back(validationEvent);
        }

        void AddValidationEvent(ValidationEvent* validationEvent)
        {
            m_validationEvents.emplace_back(validationEvent);
        }

    private:

        bool HasSeverity(const ValidationSeverity& severity) const
        {
            for (auto validationEvent : m_validationEvents)
            {
                if (validationEvent->GetSeverity() == severity)
                {
                    return true;
                }
            }

            return false;
        }

        int CountSeverity(const ValidationSeverity& severity) const
        {
            int count = 0;

            for (auto validationEvent : m_validationEvents)
            {
                if (validationEvent->GetSeverity() == severity)
                {
                    ++count;
                }
            }

            return count;
        }

        ValidationEventList m_validationEvents;
    };

    class StatusRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        //! Validates the graph for invalid connections between node's endpoints
        //! Any errors are logged to the "Script Canvas" window
        virtual void ValidateGraph(ValidationResults& validationEvents) = 0;

        virtual void ReportValidationResults(ValidationResults& validationEvents) = 0;
    };

    using StatusRequestBus = AZ::EBus<StatusRequests>;

    class ValidationRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        using BusIdType = ScriptCanvas::ScriptCanvasId;

        //! Validates the graph for invalid connections between node's endpoints
        //! Any errors are logged to the "Script Canvas" window
        virtual AZStd::pair<ScriptCanvas::ScriptCanvasId, ValidationResults> GetValidationResults() = 0;
    };

    using ValidationRequestBus = AZ::EBus<ValidationRequests>;

}
