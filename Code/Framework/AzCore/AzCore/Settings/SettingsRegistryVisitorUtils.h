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

        FieldVisitor();

        // Bring the base class visitor functions into scope
        using AZ::SettingsRegistryInterface::Visitor::Visit;

        //! @return VisitResponse of Done halts further iteration of sibling fields
        //! all other VisitRepsonse values are ignored
        virtual AZ::SettingsRegistryInterface::VisitResponse Visit(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs) = 0;

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
        VisitResponse Traverse(const AZ::SettingsRegistryInterface::VisitArgs& visitArgs, VisitAction action) override;

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

    //! Signature of callback function invoked when visiting an element of an array or object
    //! @return VisitResponse of Done halts further iteration of sibling fields
    //! all other VisitRepsonse values are ignored
    using VisitorCallback = AZStd::function<AZ::SettingsRegistryInterface::VisitResponse(const AZ::SettingsRegistryInterface::VisitArgs&)>;

    //! Invokes the visitor callback for each element of either the array or object at @path
    //! If @path is not an array or object, then no elements are visited
    //! This function will not recurse into children of elements
    //! @visitCallback functor that is invoked for each array or object element found
    //! @return Whether or not entries could be visited.
    bool VisitField(AZ::SettingsRegistryInterface& settingsRegistry, const VisitorCallback& visitCallback, AZStd::string_view path);
    //! Invokes the visitor callback for each element of the array at @path
    //! If @path is not an array, then no elements are visited
    //! This function will not recurse into children of elements
    //! @visitCallback functor that is invoked for each array element found
    //! @return Whether or not entries could be visited.
    bool VisitArray(AZ::SettingsRegistryInterface& settingsRegistry, const VisitorCallback& visitCallback, AZStd::string_view path);
    //! Invokes the visitor callback for each element of the object at @path
    //! If @path is not an object, then no elements are visited
    //! This function will not recurse into children of elements
    //! @visitCallback functor that is invoked for each object element found
    //! @return Whether or not entries could be visited.
    bool VisitObject(AZ::SettingsRegistryInterface& settingsRegistry, const VisitorCallback& visitCallback, AZStd::string_view path);
}
