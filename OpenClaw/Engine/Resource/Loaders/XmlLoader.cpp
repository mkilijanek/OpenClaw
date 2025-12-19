#include "../../GameApp/BaseGameApp.h"

#include "XmlLoader.h"
#include "../ResourceMgr.h"

#include <algorithm>
#include <fstream>

namespace
{
    // Fail fast on unreasonably large XML files to protect memory usage.
    const uint32_t MAX_XML_DOCUMENT_SIZE = 5 * 1024 * 1024; // 5 MB

    bool HasXmlPlausibleHeader(const std::string& xmlContent)
    {
        auto firstNonWhitespace = xmlContent.find_first_not_of(" \t\r\n");
        if (firstNonWhitespace == std::string::npos)
        {
            return false;
        }

        return xmlContent[firstNonWhitespace] == '<';
    }
}

//=================================================================================================
// class XmlResourceExtraData
//
//     This class implements the IResourceExtraData
//

bool XmlResourceExtraData::ParseXml(const char* rawBuffer, uint32 rawSize)
{
    if (rawBuffer == nullptr || rawSize == 0)
    {
        LOG_ERROR("XmlResourceExtraData received empty buffer to parse.");
        return false;
    }

    if (rawSize > MAX_XML_DOCUMENT_SIZE)
    {
        LOG_ERROR("XML resource exceeds maximum allowed size.");
        return false;
    }

    std::string xmlContent(rawBuffer, rawSize);

    if (!HasXmlPlausibleHeader(xmlContent))
    {
        LOG_ERROR("XML resource appears to be malformed or missing header.");
        return false;
    }

    _xmlDocument.Clear();
    _xmlDocument.Parse(xmlContent.c_str());

    if (_xmlDocument.Error())
    {
        LOG_ERROR(std::string("Failed to parse XML resource: ") + _xmlDocument.ErrorDesc());
        return false;
    }

    if (_xmlDocument.RootElement() == nullptr)
    {
        LOG_ERROR("Parsed XML resource does not contain a root element.");
        return false;
    }

    return true;
}

TiXmlElement* XmlResourceExtraData::GetRoot()
{
    return _xmlDocument.RootElement();
}

TiXmlDocument* XmlResourceExtraData::GetDocument()
{
    return &_xmlDocument;
}

//=================================================================================================
// class XmlResourceLoader
//
//     This class implements the IResourceLoader interface with XML document loading
//

bool XmlResourceLoader::VLoadResource(char* rawBuffer, uint32 rawSize, std::shared_ptr<ResourceHandle> handle)
{
    if (rawSize <= 0 || rawBuffer == NULL)
    {
        LOG_ERROR("Received invalid rawBuffer or its size");
        return false;
    }

    shared_ptr<XmlResourceExtraData> extraData = shared_ptr<XmlResourceExtraData>(new XmlResourceExtraData());
    if (!extraData->ParseXml(rawBuffer, rawSize))
    {
        LOG_ERROR("Failed to parse XML resource during load.");
        return false;
    }

    handle->SetExtraData(extraData);

    return true;
}

std::shared_ptr<TiXmlDocument> XmlResourceLoader::LoadAndReturnRootXmlElement(const char* resourceString, bool fromLocalFile)
{
    if (resourceString == nullptr)
    {
        LOG_ERROR("Null resource string provided to XmlResourceLoader.");
        return nullptr;
    }

    if (fromLocalFile)
    {
        std::ifstream input(resourceString, std::ios::binary | std::ios::ate);
        if (!input.is_open())
        {
            LOG_ERROR("Could not open XML document: " + std::string(resourceString));
            return nullptr;
        }

        const std::streamoff fileSize = input.tellg();
        if (fileSize <= 0 || fileSize > static_cast<std::streamoff>(MAX_XML_DOCUMENT_SIZE))
        {
            LOG_ERROR("XML document \"" + std::string(resourceString) + "\" exceeds allowed size or is empty.");
            return nullptr;
        }

        input.seekg(0, std::ios::beg);
        std::string headerSample;
        headerSample.resize(static_cast<size_t>(std::min<std::streamoff>(fileSize, 64)));
        input.read(&headerSample[0], headerSample.size());
        if (!HasXmlPlausibleHeader(headerSample))
        {
            LOG_ERROR("XML document \"" + std::string(resourceString) + "\" seems malformed.");
            return nullptr;
        }

        auto doc = std::make_shared<TiXmlDocument>();
        doc->LoadFile(resourceString);
        if (doc->Error())
        {
            LOG_ERROR("Could not load XML document: " + std::string(resourceString) + ". Error: " + doc->ErrorDesc());
            return nullptr;
        }

        if (doc->RootElement() == nullptr)
        {
            LOG_ERROR("XML document missing root element: " + std::string(resourceString));
            return nullptr;
        }

        return doc;
    }
    else
    {
        Resource resource(resourceString);

        // XMLs are only located in my own archive
        shared_ptr<ResourceHandle> pHandle = g_pApp->GetResourceMgr()->VGetHandle(&resource, CUSTOM_RESOURCE);
        if (!pHandle)
        {
            LOG_ERROR("Could not retrieve resource handle for: " + std::string(resourceString));
            return nullptr;
        }

        shared_ptr<XmlResourceExtraData> extraData = std::static_pointer_cast<XmlResourceExtraData>(pHandle->GetExtraData());

        if (!extraData)
        {
            LOG_ERROR("Could not cast type to XmlResourceExtraData. Check if XmlResourceLoader is registered.");
            return nullptr;
        }

        TiXmlDocument* document = extraData->GetDocument();
        if (document == nullptr || document->RootElement() == nullptr)
        {
            LOG_ERROR("XML resource missing root element: " + std::string(resourceString));
            return nullptr;
        }

        // Alias the document lifetime to the resource handle to avoid dangling references.
        return std::shared_ptr<TiXmlDocument>(pHandle, document);
    }
}

std::shared_ptr<XmlResourceLoader> XmlResourceLoader::Create()
{
    return shared_ptr<XmlResourceLoader>(new XmlResourceLoader());
}
