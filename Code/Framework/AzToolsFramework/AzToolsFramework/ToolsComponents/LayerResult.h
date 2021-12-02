/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QtCore/QString>

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
