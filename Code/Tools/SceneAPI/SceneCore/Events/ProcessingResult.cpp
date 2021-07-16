/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            ProcessingResultCombiner::ProcessingResultCombiner()
                : m_value(ProcessingResult::Ignored)
            {
            }

            void ProcessingResultCombiner::operator=(ProcessingResult rhs)
            {
                Combine(rhs);
            }

            void ProcessingResultCombiner::operator+=(ProcessingResult rhs)
            {
                Combine(rhs);
            }

            void ProcessingResultCombiner::Combine(ProcessingResult rhs)
            {
                switch (rhs)
                {
                case ProcessingResult::Ignored:
                    return;
                case ProcessingResult::Success:
                    m_value = m_value != ProcessingResult::Failure ? rhs : ProcessingResult::Failure;
                    return;
                case ProcessingResult::Failure:
                    m_value = ProcessingResult::Failure;
                    return;
                }
            }

            ProcessingResult ProcessingResultCombiner::GetResult() const
            {
                return m_value;
            }
        } // Events
    } // SceneAPI
} // AZ
