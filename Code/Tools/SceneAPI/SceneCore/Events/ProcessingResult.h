#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
