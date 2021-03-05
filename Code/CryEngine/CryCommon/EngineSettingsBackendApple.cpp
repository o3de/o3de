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

#include "EngineSettingsBackendApple.h"

#ifdef CRY_ENABLE_RC_HELPER

#include "AzCore/PlatformDef.h"

#if AZ_TRAIT_OS_PLATFORM_APPLE

#include "EngineSettingsManager.h"
#include "SettingsManagerHelpers.h"

#include "platform.h"

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/nlist.h>

#include <cassert>
#include <codecvt>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>

using namespace SettingsManagerHelpers;

static const char gDefaultRegistryLocation[] = "/EngineSettings.reg";

#define REG_SOFTWARE            L"Software\\"
#define REG_COMPANY_NAME        L"Amazon\\"
#define REG_PRODUCT_NAME        L"Lumberyard\\"
#define REG_SETTING             L"Settings\\"
#define REG_BASE_SETTING_KEY  REG_SOFTWARE REG_COMPANY_NAME REG_PRODUCT_NAME REG_SETTING

//////////////////////////////////////////////////////////////////////////
class SimpleRegistry
{
    typedef std::map< std::wstring, std::wstring > WStringMap;
    std::map< std::wstring, WStringMap * > m_modules;

public:
    SimpleRegistry();
    ~SimpleRegistry();

    void setBoolValue(const std::wstring& module, const std::wstring& key, bool value);
    void setIntValue(const std::wstring& module, const std::wstring& key, int value);
    void setStrValue(const std::wstring& module, const std::wstring& key, const std::wstring& value);

    bool getBoolValue(const std::wstring& module, const std::wstring& key, bool& value);
    bool getIntValue(const std::wstring& module, const std::wstring& key, int& value);
    bool getStrValue(const std::wstring& module, const std::wstring& key, std::wstring& value);

    bool loadFromFile(const char* fileName);
    bool saveToFile(const char* fileName);

protected:
    void clear();

private:
    static const wchar_t gSimpleMagic[];
    static const size_t gMetaCharCount;
};

const wchar_t SimpleRegistry::gSimpleMagic[] = L"FR0";
const size_t SimpleRegistry::gMetaCharCount = sizeof(size_t) / sizeof(wchar_t);

SimpleRegistry::SimpleRegistry()
{
}

SimpleRegistry::~SimpleRegistry()
{
    clear();
}

void SimpleRegistry::setBoolValue(const std::wstring& module, const std::wstring& key, bool value)
{
    return setStrValue(module, key, value ? L"true" : L"false");
}

void SimpleRegistry::setIntValue(const std::wstring& module, const std::wstring& key, int value)
{
    return setStrValue(module, key, std::to_wstring(value));
}

void SimpleRegistry::setStrValue(const std::wstring& module, const std::wstring& key, const std::wstring& value)
{
    WStringMap *map = nullptr;

    auto i = m_modules.find(module);

    if (i == m_modules.end())
    {
        map = new WStringMap;
        m_modules.emplace(module, map);
    }
    else
    {
        map = i->second;
    }

    assert(map);

    (*map)[key] = value;
}

bool SimpleRegistry::getBoolValue(const std::wstring& module, const std::wstring& key, bool& value)
{
    std::wstring str;

    if (!getStrValue(module, key, str))
    {
        return false;
    }

    value = (0 == str.compare(L"true"));

    return true;
}

bool SimpleRegistry::getIntValue(const std::wstring& module, const std::wstring& key, int& value)
{
    std::wstring str;

    if (!getStrValue(module, key, str))
    {
        return false;
    }

    value = std::stoi(str);

    return true;
}

bool SimpleRegistry::getStrValue(const std::wstring& module, const std::wstring& key, std::wstring& value)
{
    WStringMap *map = nullptr;

    auto mi = m_modules.find(module);
    if (mi == m_modules.end())
    {
        return false;
    }

    map = mi->second;
    assert(map);

    auto ki = map->find(key);
    if (ki == map->end())
    {
        return false;
    }

    value = ki->second;

    return true;
}

bool SimpleRegistry::loadFromFile(const char* fileName)
{
    clear();

    std::wifstream file(fileName, std::ios_base::in|std::ios_base::binary);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf16<wchar_t>));
    if (!file.is_open())
    {
        AZ_Warning("EngineSettings", false, "Failed to open registry settings file: %s", fileName);
        return false;
    }

    std::wstring module;
    std::wstring key;
    std::wstring value;

    wchar_t buffer[512];
    size_t size;
    wchar_t meta[gMetaCharCount];

    /* magic number */
    if(!file.read(buffer, sizeof(gSimpleMagic) / sizeof(wchar_t)) ||
       wcsncmp(gSimpleMagic, buffer, sizeof(gSimpleMagic) / sizeof(wchar_t)) != 0)
    {
        file.close();
        AZ_Warning("EngineSettings", false, "Failed to load registry settings from file: %s", fileName);
        return false;
    }

    while (file.good())
    {
        file.read(meta, gMetaCharCount);
        if (!file.good())
        {
            break;
        }

        memcpy(&size, meta, sizeof(size));
        file.read(buffer, size);
        buffer[file.gcount()] = L'\0';
        module = buffer;

        file.read(meta, gMetaCharCount);
        memcpy(&size, meta, sizeof(size));
        file.read(buffer, size);
        buffer[file.gcount()] = L'\0';
        key = buffer;

        file.read(meta, gMetaCharCount);
        memcpy(&size, meta, sizeof(size));
        file.read(buffer, size);
        buffer[file.gcount()] = L'\0';
        value = buffer;

        setStrValue(module, key, value);
    }

    file.close();

    return true;
}

bool SimpleRegistry::saveToFile(const char* fileName)
{
    std::wofstream file(fileName, std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf16<wchar_t>));
    if (!file.is_open())
    {
        return false;
    }

    std::wstring module;

    size_t size;
    wchar_t meta[gMetaCharCount];

    /* magic number */
    file.write(gSimpleMagic, sizeof(gSimpleMagic) / sizeof(wchar_t));

    for (auto j : m_modules)
    {
        module = j.first;

        for (auto i : *j.second)
        {
            size = module.size();
            memcpy(meta, &size, sizeof(meta));
            file.write(meta, gMetaCharCount);
            file.write(module.c_str(), size);

            size = i.first.size();
            memcpy(meta, &size, sizeof(meta));
            file.write(meta, gMetaCharCount);
            file.write(i.first.c_str(), size);

            size = i.second.size();
            memcpy(meta, &size, sizeof(meta));
            file.write(meta, gMetaCharCount);
            file.write(i.second.c_str(), size);
        }
    }

    file.close();

    return true;
}

void SimpleRegistry::clear()
{
    for (auto pair : m_modules)
    {
        delete pair.second;
    }

    m_modules.clear();
}
//////////////////////////////////////////////////////////////////////////

CEngineSettingsBackendApple::CEngineSettingsBackendApple(CEngineSettingsManager* parent, const wchar_t* moduleName)
    : CEngineSettingsBackend(parent, moduleName)
    , m_registry(new SimpleRegistry)
    , m_registryFilePath()
{
    std::string rootValue = gEnv->pFileIO->GetAlias("@root@");
    if (rootValue.empty())
    {
        AZ_Warning("EngineSettings", false, "Could not get engine root.");
        return;
    }

    rootValue.append(gDefaultRegistryLocation);
    m_registryFilePath = rootValue;
}

CEngineSettingsBackendApple::~CEngineSettingsBackendApple()
{
    delete m_registry, m_registry = nullptr;
}

std::wstring CEngineSettingsBackendApple::GetModuleFilePath() const
{
    std::string path;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::string module = converter.to_bytes(moduleName());

    void* handle = ::dlopen(module.c_str(), RTLD_LAZY);
    if (handle)
    {
        const int c = _dyld_image_count();
        for (int i = 0; i < c; ++i)
        {
            const char* image = _dyld_get_image_name(i);
            const void* altHandle = dlopen(image, RTLD_LAZY);
            if (handle == altHandle)
            {
                char absImage[PATH_MAX];
                realpath(image, absImage);
                char *ext = rindex(absImage, '.');
                if (ext)
                {
                    *ext = '\0';
                }
                path.append(absImage);
                path.append(".ini");
                break;
            }
        }
    }

    return converter.from_bytes(path);
}

bool CEngineSettingsBackendApple::GetModuleSpecificStringEntryUtf16(const char* key, CWCharBuffer wbuffer)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wkey = converter.from_bytes(key);

    std::wstring str;
    if (!m_registry->getStrValue(moduleName(), wkey, str))
    {
        return false;
    }

    std::wcscpy(wbuffer.getPtr(), str.c_str());

    return true;
}

bool CEngineSettingsBackendApple::GetModuleSpecificIntEntry(const char* key, int& value)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wkey = converter.from_bytes(key);

    return m_registry->getIntValue(moduleName(), wkey, value);
}

bool CEngineSettingsBackendApple::GetModuleSpecificBoolEntry(const char* key, bool& value)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wkey = converter.from_bytes(key);

    return m_registry->getBoolValue(moduleName(), wkey, value);
}

bool CEngineSettingsBackendApple::SetModuleSpecificStringEntryUtf16(const char* key, const wchar_t* str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wkey = converter.from_bytes(key);

    m_registry->setStrValue(moduleName(), wkey, str);

    return true;
}

bool CEngineSettingsBackendApple::SetModuleSpecificIntEntry(const char* key, const int& value)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wkey = converter.from_bytes(key);

    m_registry->setIntValue(moduleName(), wkey, value);

    return true;
}

bool CEngineSettingsBackendApple::SetModuleSpecificBoolEntry(const char* key, const bool& value)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wkey = converter.from_bytes(key);

    m_registry->setBoolValue(moduleName(), wkey, value);

    return true;
}

bool CEngineSettingsBackendApple::GetInstalledBuildRootPathUtf16(const int index, CWCharBuffer name, CWCharBuffer path)
{
    return false;
}

bool CEngineSettingsBackendApple::StoreEngineSettingsToRegistry()
{
    bool bRet = true;
    wchar_t buffer[1024];

    // ResourceCompiler Specific
    if (parent()->GetValueByRef("RC_ShowWindow", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        const bool b = wcscmp(buffer, L"true") == 0;
        m_registry->setBoolValue(REG_BASE_SETTING_KEY, L"RC_ShowWindow", b);
    }

    if (parent()->GetValueByRef("RC_HideCustom", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        const bool b = wcscmp(buffer, L"true") == 0;
        m_registry->setBoolValue(REG_BASE_SETTING_KEY, L"RC_HideCustom", b);
    }

    if (parent()->GetValueByRef("RC_Parameters", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        m_registry->setStrValue(REG_BASE_SETTING_KEY, L"RC_Parameters", buffer);
    }

    if (parent()->GetValueByRef("RC_EnableSourceControl", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer))))
    {
        const bool b = wcscmp(buffer, L"true") == 0;
        m_registry->setBoolValue(REG_BASE_SETTING_KEY, L"RC_EnableSourceControl", b);
    }

    bRet &= m_registry->saveToFile(m_registryFilePath.c_str());
    return bRet;
}

void CEngineSettingsBackendApple::LoadEngineSettingsFromRegistry()
{
    if (!m_registry->loadFromFile(m_registryFilePath.c_str()))
    {
        return;
    }

    std::wstring wStrResult;
    bool bResult;

    if (m_registry->getStrValue(REG_BASE_SETTING_KEY, L"RootPath", wStrResult))
    {
        parent()->SetKey("ENG_RootPath", wStrResult.c_str());
    }

    // Engine Specific
    if (m_registry->getStrValue(REG_BASE_SETTING_KEY, L"ENG_RootPath", wStrResult))
    {
        parent()->SetKey("ENG_RootPath", wStrResult.c_str());
    }

    // ResourceCompiler Specific
    if (m_registry->getBoolValue(REG_BASE_SETTING_KEY, L"RC_ShowWindow", bResult))
    {
        parent()->SetKey("RC_ShowWindow", bResult);
    }
    if (m_registry->getBoolValue(REG_BASE_SETTING_KEY, L"RC_HideCustom", bResult))
    {
        parent()->SetKey("RC_HideCustom", bResult);
    }
    if (m_registry->getStrValue(REG_BASE_SETTING_KEY, L"RC_Parameters", wStrResult))
    {
        parent()->SetKey("RC_Parameters", wStrResult.c_str());
    }
    if (m_registry->getBoolValue(REG_BASE_SETTING_KEY, L"RC_EnableSourceControl", bResult))
    {
        parent()->SetKey("RC_EnableSourceControl", bResult);
    }
}

#endif // AZ_TRAIT_OS_PLATFORM_APPLE
#endif // CRY_ENABLE_RC_HELPER

