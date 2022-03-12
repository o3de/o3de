/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>


namespace AZ::SettingsRegistryVisitorUtils
{
    //! Interface for visiting the fields of an array or object
    //! To access the values, use the SettingsRegistryInterface Get/GetObject methods
    struct FieldVisitor
        : public AZ::SettingsRegistryInterface::Visitor
    {
        using VisitResponse = AZ::SettingsRegistryInterface::VisitResponse;
        using VisitAction = AZ::SettingsRegistryInterface::VisitAction;
        using Type = AZ::SettingsRegistryInterface::Type;

        FieldVisitor();

        // Bring the base class visitor functions into scope
        using AZ::SettingsRegistryInterface::Visitor::Visit;
        virtual void Visit(AZStd::string_view path, AZStd::string_view arrayIndex, Type type) = 0;

    protected:
        // VisitFieldType is used for filtering the type of referenced by the root path
        enum class VisitFieldType
        {
            Array,
            Object,
            ArrayOrObject
        };
        FieldVisitor(const VisitFieldType visitFieldType);
    private:
        VisitResponse Traverse(AZStd::string_view path, AZStd::string_view valueName,
            VisitAction action, Type type) override;

        VisitFieldType m_visitFieldType{ VisitFieldType::ArrayOrObject };
        AZStd::optional<AZ::SettingsRegistryInterface::FixedValueString> m_rootPath;
    };

    //! Interface for visiting the fields of an array
    //! To access the values, use the SettingsRegistryInterface Get/GetObject methods
    struct ArrayVisitor
        : public FieldVisitor
    {
        ArrayVisitor();
    };

    //! Interface for visiting the fields of an object
    //! To access the values, use the SettingsRegistryInterface Get/GetObject methods
    struct ObjectVisitor
        : public FieldVisitor
    {
        ObjectVisitor();
    };

    //! Signature of callback funcition invoked when visiting an element of an array or object
    using VisitorCallback = AZStd::function<void(AZStd::string_view path, AZStd::string_view fieldName,
        AZ::SettingsRegistryInterface::Type)>;

    //! Invokes the visitor callback for each element of either the array or object at @path
    //! If @path is not an array or object, then no elements are visited
    //! This function will not recurse into children of elements
    //! @visitCallback functor that is invoked for each array or object element found
    bool VisitField(AZ::SettingsRegistryInterface& settingsRegistry, const VisitorCallback& visitCallback, AZStd::string_view path);
    //! Invokes the visitor callback for each element of the array at @path
    //! If @path is not an array, then no elements are visited
    //! This function will not recurse into children of elements
    //! @visitCallback functor that is invoked for each array element found
    bool VisitArray(AZ::SettingsRegistryInterface& settingsRegistry, const VisitorCallback& visitCallback, AZStd::string_view path);
    //! Invokes the visitor callback for each element of the object at @path
    //! If @path is not an object, then no elements are visited
    //! This function will not recurse into children of elements
    //! @visitCallback functor that is invoked for each object element found
    bool VisitObject(AZ::SettingsRegistryInterface& settingsRegistry, const VisitorCallback& visitCallback, AZStd::string_view path);
}
