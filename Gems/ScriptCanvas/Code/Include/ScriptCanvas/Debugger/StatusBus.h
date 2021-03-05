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

        ~ValidationResults()
        {
            ClearResults();
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
            for (auto validationEvent : m_validationEvents)
            {
                delete validationEvent;
            }

            m_validationEvents.clear();
        }

        const AZStd::vector< ValidationEvent* >& GetEvents() const
        {
            return m_validationEvents;
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

        AZStd::vector< ValidationEvent* > m_validationEvents;
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
    };

    using StatusRequestBus = AZ::EBus<StatusRequests>;
}
