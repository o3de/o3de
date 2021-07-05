/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "JsonDescriptor.h"
#include "Resource.h"

#include <QJsonDocument>

using namespace News;

JsonDescriptor::JsonDescriptor(Resource& resource)
    : Descriptor(resource)
    , m_doc(QJsonDocument::fromJson(m_resource.GetData()))
    , m_json(m_doc.object())
{
}
