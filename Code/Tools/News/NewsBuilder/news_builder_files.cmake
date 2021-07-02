#
# Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
# 
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    main.cpp
    S3Connector.cpp
    S3Connector.h
    UidGenerator.cpp
    UidGenerator.h
    EndpointManager.cpp
    EndpointManager.h
    Qt/ArticleDetails.cpp
    Qt/ArticleDetails.h
    Qt/ArticleDetails.ui
    Qt/ArticleDetailsContainer.cpp
    Qt/ArticleDetailsContainer.h
    Qt/ArticleDetailsContainer.ui
    Qt/AwsDialog.ui
    Qt/BuilderArticleViewContainer.cpp
    Qt/BuilderArticleViewContainer.h
    Qt/BuilderArticleViewContainer.ui
    Qt/EndpointEntryView.cpp
    Qt/EndpointEntryView.h
    Qt/EndpointEntryView.ui
    Qt/EndpointManagerView.cpp
    Qt/EndpointManagerView.h
    Qt/EndpointManagerView.ui
    Qt/ImageItem.cpp
    Qt/ImageItem.h
    Qt/ImageItem.ui
    Qt/LogContainer.cpp
    Qt/LogContainer.h
    Qt/LogContainer.ui
    Qt/NewsBuilder.cpp
    Qt/NewsBuilder.h
    Qt/Newsbuilder.qrc
    Qt/Newsbuilder.ui
    Qt/QCustomMessageBox.cpp
    Qt/QCustomMessageBox.h
    Qt/QCustomMessageBox.ui
    Qt/SelectImage.cpp
    Qt/SelectImage.h
    Qt/SelectImage.ui
    ResourceManagement/BuilderResourceManifest.cpp
    ResourceManagement/BuilderResourceManifest.h
    ResourceManagement/DeleteDescriptor.cpp
    ResourceManagement/DeleteDescriptor.h
    ResourceManagement/ImageDescriptor.cpp
    ResourceManagement/ImageDescriptor.h
    ResourceManagement/UploadDescriptor.cpp
    ResourceManagement/UploadDescriptor.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    # Fix for unity causing the 'News::BuilderResourceManifest::UpdateResourceA' symbol to be unresolved
    Qt/ArticleDetails.cpp
    # Fix for unity causing the ' Aws::S3::S3Client::GetObjectA' symbol to be unresolved
    S3Connector.cpp

)
