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

#include <SceneAPI/SceneCore/Events/CallProcessorBinder.h>

struct IConvertContext;

namespace AZ
{
    namespace SceneAPI
    {
        namespace Events
        {
            class ExportEventContext;
        }
    }

    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;

        class CgfExporter
            : public SceneEvents::CallProcessorBinder
        {
        public:
            CgfExporter(IConvertContext* convertContext);
            ~CgfExporter() override = default;

            SceneEvents::ProcessingResult ProcessContext(SceneEvents::ExportEventContext& context);

        private:
            IConvertContext* m_convertContext;
        };
    } // namespace RC
} // namespace AZ
