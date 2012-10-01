/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 */

#include "config.h"
#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystemBlackBerry.h"

#include "AsyncFileSystemCallbacks.h"
#include "AsyncFileWriterBlackBerry.h"
#include "AsyncFileWriterClient.h"
#include "DOMFileSystemBase.h"
#include "ExceptionCode.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "PlatformAsyncFileSystemCallbacks.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

bool AsyncFileSystem::isAvailable()
{
    return true;
}

// We don't use this one!
PassOwnPtr<AsyncFileSystem> AsyncFileSystem::create()
{
    ASSERT_NOT_REACHED();
    return PassOwnPtr<AsyncFileSystem>();
}

// We don't use this one!
void AsyncFileSystem::openFileSystem(const String& basePath, const String& storageIdentifier, bool, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    UNUSED_PARAM(basePath);
    UNUSED_PARAM(storageIdentifier);
    UNUSED_PARAM(callbacks);
    ASSERT_NOT_REACHED();
}

void AsyncFileSystemBlackBerry::openFileSystem(const KURL& rootURL, const String& basePath, const String& storageIdentifier, FileSystemType type, long long size, bool create, bool sandboxed, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    BlackBerry::Platform::WebFileSystem::openFileSystem(BlackBerry::Platform::webKitThreadMessageClient(), basePath.utf8().data(), storageIdentifier.utf8().data(), static_cast<BlackBerry::Platform::WebFileSystem::Type>(type), size, create, new PlatformAsyncFileSystemCallbacks(callbacks, rootURL), sandboxed ? BlackBerry::Platform::WebFileSystem::SandboxStandard : BlackBerry::Platform::WebFileSystem::NotSandboxed);
}

void AsyncFileSystem::deleteFileSystem(const String& basePath, const String& storageIdentifier, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    UNUSED_PARAM(basePath);
    UNUSED_PARAM(storageIdentifier);
    UNUSED_PARAM(type);

    notImplemented();
    callbacks->didFail(NOT_SUPPORTED_ERR);
}

AsyncFileSystemBlackBerry::AsyncFileSystemBlackBerry(PassOwnPtr<BlackBerry::Platform::WebFileSystem> platformFileSystem)
    : AsyncFileSystem() // FIXME: type ???
    , m_platformFileSystem(platformFileSystem)
{
}

AsyncFileSystemBlackBerry::~AsyncFileSystemBlackBerry()
{
}

void AsyncFileSystemBlackBerry::move(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->move(fileSystemURLToPath(sourcePath).utf8().data(), fileSystemURLToPath(destinationPath).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::copy(const KURL& sourcePath, const KURL& destinationPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->copy(fileSystemURLToPath(sourcePath).utf8().data(), fileSystemURLToPath(destinationPath).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->remove(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->removeRecursively(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->readMetadata(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->createFile(fileSystemURLToPath(path).utf8().data(), exclusive, new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->createDirectory(fileSystemURLToPath(path).utf8().data(), exclusive, new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->fileExists(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->directoryExists(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->readDirectory(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

void AsyncFileSystemBlackBerry::createWriter(AsyncFileWriterClient* client, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    BlackBerry::Platform::WebFileWriter::createWriter(m_platformFileSystem.get(), fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks, client));
}

void AsyncFileSystemBlackBerry::createSnapshotFileAndReadMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    m_platformFileSystem->createSnapshotFileAndReadMetadata(fileSystemURLToPath(path).utf8().data(), new PlatformAsyncFileSystemCallbacks(callbacks));
}

String AsyncFileSystemBlackBerry::fileSystemURLToPath(const KURL& url)
{
    FileSystemType type;
    String fullPath;
    bool result = DOMFileSystemBase::crackFileSystemURL(url, type, fullPath);
    ASSERT_UNUSED(result, result);
    return fullPath;
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
