
/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CacheStorageEngineConnection.h"

#include "Logging.h"
#include "NetworkConnectionToWebProcess.h"
#include "WebCacheStorageConnectionMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/CacheQueryOptions.h>

using namespace WebCore::DOMCacheEngine;
using namespace WebKit::CacheStorage;

namespace WebKit {

#define RELEASE_LOG_IF_ALLOWED(fmt, ...) RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), CacheStorage, "%p - CacheStorageEngineConnection::" fmt, &m_connection.connection(), ##__VA_ARGS__)
#define RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK(functionName, fmt, resultGetter) \
    if (!result.has_value())\
        RELEASE_LOG_ERROR_IF(sessionID.isAlwaysOnLoggingAllowed(), CacheStorage, "%p - CacheStorageEngineConnection::%s (%llu) - failed - error %d", connection.ptr(), functionName, requestIdentifier, (int)result.error()); \
    else {\
        auto value = resultGetter(result.value()); \
        UNUSED_PARAM(value); \
        RELEASE_LOG_IF(sessionID.isAlwaysOnLoggingAllowed(), CacheStorage, "%p - CacheStorageEngineConnection::%s (%llu) - succeeded - " fmt, connection.ptr(), functionName, requestIdentifier, value); \
    }
CacheStorageEngineConnection::CacheStorageEngineConnection(NetworkConnectionToWebProcess& connection)
    : m_connection(connection)
{
}

CacheStorageEngineConnection::~CacheStorageEngineConnection()
{
    for (auto& keyValue : m_cachesLocks) {
        auto& sessionID = keyValue.key;
        for (auto& references : keyValue.value) {
            ASSERT(references.value);
            Engine::from(sessionID).unlock(references.key);
        }
    }
}

void CacheStorageEngineConnection::open(PAL::SessionID sessionID, uint64_t requestIdentifier, const WebCore::ClientOrigin& origin, const String& cacheName)
{
    RELEASE_LOG_IF_ALLOWED("open (%llu)", requestIdentifier);
    Engine::from(sessionID).open(origin, cacheName, [connection = makeRef(m_connection.connection()), sessionID, requestIdentifier](const CacheIdentifierOrError& result) {
        RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK("open", "cache identifier is %llu", [](const auto& value) { return value.identifier; });
        connection->send(Messages::WebCacheStorageConnection::OpenCompleted(requestIdentifier, result), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::remove(PAL::SessionID sessionID, uint64_t requestIdentifier, uint64_t cacheIdentifier)
{
    RELEASE_LOG_IF_ALLOWED("remove (%llu) cache %llu", requestIdentifier, cacheIdentifier);
    Engine::from(sessionID).remove(cacheIdentifier, [connection = makeRef(m_connection.connection()), sessionID, requestIdentifier](const CacheIdentifierOrError& result) {
        RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK("remove", "removed cache %llu", [](const auto& value) { return value.identifier; });
        connection->send(Messages::WebCacheStorageConnection::RemoveCompleted(requestIdentifier, result), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::caches(PAL::SessionID sessionID, uint64_t requestIdentifier, const WebCore::ClientOrigin& origin, uint64_t updateCounter)
{
    RELEASE_LOG_IF_ALLOWED("caches (%llu)", requestIdentifier);
    Engine::from(sessionID).retrieveCaches(origin, updateCounter, [connection = makeRef(m_connection.connection()), sessionID, origin, requestIdentifier](CacheInfosOrError&& result) {
        RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK("caches", "caches size is %lu", [](const auto& value) { return value.infos.size(); });
        connection->send(Messages::WebCacheStorageConnection::UpdateCaches(requestIdentifier, result), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::retrieveRecords(PAL::SessionID sessionID, uint64_t requestIdentifier, uint64_t cacheIdentifier, WebCore::URL&& url)
{
    RELEASE_LOG_IF_ALLOWED("retrieveRecords (%llu) in cache %llu", requestIdentifier, cacheIdentifier);
    Engine::from(sessionID).retrieveRecords(cacheIdentifier, WTFMove(url), [connection = makeRef(m_connection.connection()), sessionID, requestIdentifier](RecordsOrError&& result) {
        RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK("retrieveRecords", "records size is %lu", [](const auto& value) { return value.size(); });
        connection->send(Messages::WebCacheStorageConnection::UpdateRecords(requestIdentifier, result), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::deleteMatchingRecords(PAL::SessionID sessionID, uint64_t requestIdentifier, uint64_t cacheIdentifier, WebCore::ResourceRequest&& request, WebCore::CacheQueryOptions&& options)
{
    RELEASE_LOG_IF_ALLOWED("deleteMatchingRecords (%llu) in cache %llu", requestIdentifier, cacheIdentifier);
    Engine::from(sessionID).deleteMatchingRecords(cacheIdentifier, WTFMove(request), WTFMove(options), [connection = makeRef(m_connection.connection()), sessionID, requestIdentifier](RecordIdentifiersOrError&& result) {
        RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK("deleteMatchingRecords", "deleted %lu records",  [](const auto& value) { return value.size(); });
        connection->send(Messages::WebCacheStorageConnection::DeleteRecordsCompleted(requestIdentifier, result), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::putRecords(PAL::SessionID sessionID, uint64_t requestIdentifier, uint64_t cacheIdentifier, Vector<Record>&& records)
{
    RELEASE_LOG_IF_ALLOWED("putRecords (%llu) in cache %llu, %lu records", requestIdentifier, cacheIdentifier, records.size());
    Engine::from(sessionID).putRecords(cacheIdentifier, WTFMove(records), [connection = makeRef(m_connection.connection()), sessionID, requestIdentifier](RecordIdentifiersOrError&& result) {
        RELEASE_LOG_FUNCTION_IF_ALLOWED_IN_CALLBACK("putRecords", "put %lu records",  [](const auto& value) { return value.size(); });
        connection->send(Messages::WebCacheStorageConnection::PutRecordsCompleted(requestIdentifier, result), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::reference(PAL::SessionID sessionID, uint64_t cacheIdentifier)
{
    RELEASE_LOG_IF_ALLOWED("reference cache %llu", cacheIdentifier);
    auto& references = m_cachesLocks.ensure(sessionID, []() {
        return HashMap<CacheIdentifier, LockCount> { };
    }).iterator->value;
    auto& counter = references.ensure(cacheIdentifier, []() {
        return 0;
    }).iterator->value;
    if (!counter++)
        Engine::from(sessionID).lock(cacheIdentifier);
}

void CacheStorageEngineConnection::dereference(PAL::SessionID sessionID, uint64_t cacheIdentifier)
{
    RELEASE_LOG_IF_ALLOWED("dereference cache %llu", cacheIdentifier);
    ASSERT(m_cachesLocks.contains(sessionID));
    auto& references = m_cachesLocks.ensure(sessionID, []() {
        return HashMap<CacheIdentifier, LockCount> { };
    }).iterator->value;

    auto referenceResult = references.find(cacheIdentifier);
    ASSERT(referenceResult != references.end());
    if (referenceResult == references.end())
        return;

    ASSERT(referenceResult->value);
    if (--referenceResult->value)
        return;

    Engine::from(sessionID).unlock(cacheIdentifier);
    references.remove(referenceResult);
}

void CacheStorageEngineConnection::clearMemoryRepresentation(PAL::SessionID sessionID, uint64_t requestIdentifier, const WebCore::ClientOrigin& origin)
{
    Engine::from(sessionID).clearMemoryRepresentation(origin, [connection = makeRef(m_connection.connection()), sessionID, requestIdentifier] (std::optional<Error>&& error) {
        connection->send(Messages::WebCacheStorageConnection::ClearMemoryRepresentationCompleted(requestIdentifier, error), sessionID.sessionID());
    });
}

void CacheStorageEngineConnection::engineRepresentation(PAL::SessionID sessionID, uint64_t requestIdentifier)
{
    m_connection.connection().send(Messages::WebCacheStorageConnection::EngineRepresentationCompleted(requestIdentifier, Engine::from(sessionID).representation()), sessionID.sessionID());
}

}
