/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>

#include "RunningStatistic.h"

namespace AZ
{
    namespace Statistics
    {
        /**
         * @brief A convenient class to assign name and units to a RunningStatistic
         *
         * Also provides convenient methods to format the statistics.
         */
        class NamedRunningStatistic : public RunningStatistic
        {
        public:
            NamedRunningStatistic(const AZStd::string& name = "Unnamed", const AZStd::string& units = "")
                : m_name(name), m_units(units)
            {
            }

            void UpdateName(const AZStd::string& name)
            {
                m_name = name;
            }

            void UpdateUnits(const AZStd::string& units)
            {
                m_units = units;
            }

            virtual ~NamedRunningStatistic() = default;

            const AZStd::string& GetName() const { return m_name; }
            
            const AZStd::string& GetUnits() const { return m_units; }
            
            AZStd::string GetFormatted() const
            {
                return AZStd::string::format("Name=\"%s\", Units=\"%s\", numSamples=%llu, avg=%f, min=%f, max=%f, stdev=%f",
                    m_name.c_str(), m_units.c_str(), GetNumSamples(), GetAverage(), GetMinimum(), GetMaximum(), GetStdev());
            }

            static const char * GetCsvHeader()
            {
                return "Name, Units, numSamples, avg, min, max, stdev";
            }

            AZStd::string GetCsvFormatted() const
            {
                return AZStd::string::format("\"%s\", \"%s\", %llu, %f, %f, %f, %f",
                    m_name.c_str(), m_units.c_str(), GetNumSamples(), GetAverage(), GetMinimum(), GetMaximum(), GetStdev());
            }

        private:
            AZStd::string m_name;
            AZStd::string m_units;
        }; //class NamedRunningStatistic
    }//namespace Statistics
}//namespace AZ
