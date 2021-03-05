#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    CrySCompileServer.cpp
    Core/Common.h
    Core/Error.cpp
    Core/Error.hpp
    Core/WindowsAPIImplementation.h
    Core/WindowsAPIImplementation.cpp
    Core/Mailer.cpp
    Core/Mailer.h
    Core/MD5.hpp
    Core/StdTypes.hpp
    Core/STLHelper.cpp
    Core/STLHelper.hpp
    Core/Server/CrySimpleCache.cpp
    Core/Server/CrySimpleCache.hpp
    Core/Server/CrySimpleErrorLog.cpp
    Core/Server/CrySimpleErrorLog.hpp
    Core/Server/CrySimpleFileGuard.hpp
    Core/Server/CrySimpleHTTP.cpp
    Core/Server/CrySimpleHTTP.hpp
    Core/Server/CrySimpleJob.cpp
    Core/Server/CrySimpleJob.hpp
    Core/Server/CrySimpleJobCache.cpp
    Core/Server/CrySimpleJobCache.hpp
    Core/Server/CrySimpleJobCompile.cpp
    Core/Server/CrySimpleJobCompile.hpp
    Core/Server/CrySimpleJobCompile1.cpp
    Core/Server/CrySimpleJobCompile1.hpp
    Core/Server/CrySimpleJobCompile2.cpp
    Core/Server/CrySimpleJobCompile2.hpp
    Core/Server/CrySimpleJobRequest.cpp
    Core/Server/CrySimpleJobRequest.hpp
    Core/Server/CrySimpleJobGetShaderList.cpp
    Core/Server/CrySimpleJobGetShaderList.hpp
    Core/Server/CrySimpleMutex.cpp
    Core/Server/CrySimpleMutex.hpp
    Core/Server/CrySimpleServer.cpp
    Core/Server/CrySimpleServer.hpp
    Core/Server/CrySimpleSock.cpp
    Core/Server/CrySimpleSock.hpp
    Core/Server/ShaderList.cpp
    Core/Server/ShaderList.hpp
    External/tinyxml/tinystr.cpp
    External/tinyxml/tinystr.h
    External/tinyxml/tinyxml.cpp
    External/tinyxml/tinyxml.h
    External/tinyxml/tinyxmlerror.cpp
    External/tinyxml/tinyxmlparser.cpp
)
