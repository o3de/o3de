/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <QColor>

namespace AzQtComponents
{
    //! StyleManagerInterface
    class AZ_QT_COMPONENTS_API StyleManagerInterface
    {
    public:
        AZ_RTTI(StyleManagerInterface, "{92DFE816-91B7-4B71-9300-C404E9831A46}");

        virtual bool IsStylePropertyDefined(const char* propertyKey) const = 0;
        virtual QString GetStylePropertyAsString(const char* propertyKey) const = 0;
        virtual int GetStylePropertyAsInteger(const char* propertyKey) const = 0;
        virtual QColor GetStylePropertyAsColor(const char* propertyKey) const = 0;
    };

} // namespace AzToolsFramework
