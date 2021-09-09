/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class BehaviorClass;
    class BehaviorMethod;
    class BehaviorEBus;
    class BehaviorProperty;
}

namespace EditorPythonBindings
{
    //! An interface to track exported Python symbols 
    class PythonSymbolEvents
        : public AZ::EBusTraits
    {
    public:
        // the symbols will be written out in the future
        static const bool EnableEventQueue = true;

        //! logs a behavior class type
        virtual void LogClass(const AZStd::string moduleName, const AZ::BehaviorClass* behaviorClass) = 0;

        //! logs a behavior class type with an override to its name
        virtual void LogClassWithName(
            const AZStd::string moduleName,
            const AZ::BehaviorClass* behaviorClass,
            const AZStd::string className) = 0;

        //! logs a static class method with a specified global method name
        virtual void LogClassMethod(
            const AZStd::string moduleName,
            const AZStd::string globalMethodName,
            const AZ::BehaviorClass* behaviorClass,
            const AZ::BehaviorMethod* behaviorMethod) = 0;

        //! logs a behavior bus with a specified bus name
        virtual void LogBus(const AZStd::string moduleName, const AZStd::string busName, const AZ::BehaviorEBus* behaviorEBus) = 0;

        //! logs a global method from the behavior context registry with a specified method name
        virtual void LogGlobalMethod(
            const AZStd::string moduleName,
            const AZStd::string methodName,
            const AZ::BehaviorMethod* behaviorMethod) = 0;

        //! logs a global property, enum, or constant from the behavior context registry with a specified property name
        virtual void LogGlobalProperty(
            const AZStd::string moduleName,
            const AZStd::string propertyName,
            const  AZ::BehaviorProperty* behaviorProperty) = 0;

        //! signals the end of the logging of symbols
        virtual void Finalize() = 0;
    };
    using PythonSymbolEventBus = AZ::EBus<PythonSymbolEvents>;

} // namespace EditorPythonBindings

