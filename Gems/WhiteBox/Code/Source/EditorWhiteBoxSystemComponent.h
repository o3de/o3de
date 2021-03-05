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

#include "WhiteBoxSystemComponent.h"

namespace WhiteBox
{
    //! System component for the White Box Editor/Tool application.
    class EditorWhiteBoxSystemComponent : public WhiteBoxSystemComponent
    {
    public:
        AZ_COMPONENT(EditorWhiteBoxSystemComponent, "{42D40E84-A8C4-474B-A4D6-B665CCEA8A83}", WhiteBoxSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

    private:
        // AZ::Component ...
        void Activate() override;

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
    };
} // namespace WhiteBox
