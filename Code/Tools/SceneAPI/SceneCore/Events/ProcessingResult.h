#pragma once

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

#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            enum class ProcessingResult
            {
                Ignored, // Event didn't apply to the processor or there was no work to do.
                Success, // Data was successfully processed.
                Failure  // Attempts to process data failed.
            };

            // Combines ProcessingResult together with the stored value such that
            //      Ignored doesn't change the stored value, 
            //      Failure is always stored,
            //      Success is only stored if not already set to failure.
            class ProcessingResultCombiner
            {
            public:
                SCENE_CORE_API ProcessingResultCombiner();
                SCENE_CORE_API void operator=(ProcessingResult rhs); // For use with EBus
                SCENE_CORE_API void operator+=(ProcessingResult rhs); // Common use.
                SCENE_CORE_API ProcessingResult GetResult() const;

            private:
                void Combine(ProcessingResult rhs);
                ProcessingResult m_value;
            };
        } // Events
    } // SceneAPI
} // AZ
