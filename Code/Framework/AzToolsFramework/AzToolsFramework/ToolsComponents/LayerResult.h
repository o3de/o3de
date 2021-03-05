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

namespace AzToolsFramework
{
    namespace Layers
    {
        enum class LayerResultStatus
        {
            Success,
            Warning,
            Error
        };

        /**
         * Holds the status of a layer operation, and a message detailing that status when not a success.
         */
        struct LayerResult
        {
        public:
            LayerResult() { }

            LayerResult(LayerResultStatus resultStatus, QString resultMessage) :
                m_result(resultStatus),
                m_message(resultMessage)
            {

            }

            /**
             * A helper function to make it obvious at callsites that a success is intended.
             */
            static LayerResult CreateSuccess() { return LayerResult(); }

            /**
            * Returns true if this LayerResult was a success, false if not.
            */
            bool IsSuccess() const
            {
                return m_result == LayerResultStatus::Success;
            }

            /**
             * If a failure occured, outputs the current failure to the appropriate system.
             * On success, does nothing.
             */
            void MessageResult();

            LayerResultStatus m_result = LayerResultStatus::Success;
            QString m_message;
        };

    }
}
