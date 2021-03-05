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