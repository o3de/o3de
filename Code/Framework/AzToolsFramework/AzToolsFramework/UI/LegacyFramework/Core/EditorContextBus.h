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

#ifndef EDITORCONTEXTBUS_H
#define EDITORCONTEXTBUS_H

#include <AzCore/base.h>

#pragma once

#include <AzCore/EBus/EBus.h>

namespace LegacyFramework
{
    // editor contexts derive from this to do all the boilerplate messaging between their GUI and their contexts
    // messages from the GUI parts head to this interface:
    class EditorContextMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // there's going to be many editor contexts for various kinds of assets, each with an 'address'.
        typedef AZ::Uuid BusIdType;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Single; // but there's only one such context at each address.
        //////////////////////////////////////////////////////////////////////////
        virtual ~EditorContextMessages() {}
    };

    // messages from the editor context to the gui parts head to this interface:
    class EditorContextClientMessages
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // Bus configuration
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById; // there's going to be more than one client in existence (for example every lua panel)
        typedef AZ::Uuid BusIdType;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ:: EBusHandlerPolicy::Multiple; // and there might be more than one listener at that address (many panels for the same document for example)
        //////////////////////////////////////////////////////////////////////////
        virtual ~EditorContextClientMessages() {}
    };
}

#endif