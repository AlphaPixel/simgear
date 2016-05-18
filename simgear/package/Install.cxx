// Copyright (C) 2013  James Turner - zakalawe@mac.com
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <simgear/package/Install.hxx>

#include <boost/foreach.hpp>
#include <fstream>

#include <simgear/package/unzip.h>
#include <simgear/package/md5.h>

#include <simgear/structure/exception.hxx>
#include <simgear/props/props_io.hxx>
#include <simgear/package/Catalog.hxx>
#include <simgear/package/Package.hxx>
#include <simgear/package/Root.hxx>
#include <simgear/io/HTTPRequest.hxx>
#include <simgear/io/HTTPClient.hxx>
#include <simgear/misc/sg_dir.hxx>
#include <simgear/misc/strutils.hxx>

extern "C" {
    void fill_memory_filefunc (zlib_filefunc_def*);
}

namespace simgear {

namespace pkg {

class Install::PackageArchiveDownloader : public HTTP::Request
{
public:
    PackageArchiveDownloader(InstallRef aOwner) :
        HTTP::Request("" /* dummy URL */),
        m_owner(aOwner),
        m_downloaded(0)
    {
        m_urls = m_owner->package()->downloadUrls();
        if (m_urls.empty()) {
            throw sg_exception("no package download URLs");
        }

        // TODO randomise order of m_urls

        m_extractPath = aOwner->path().dir();
        m_extractPath.append("_extract_" + aOwner->package()->md5());

        // clean up any existing files
        Dir d(m_extractPath);
        if (d.exists()) {
            d.remove(true /* recursive */);
        }
    }

    ~PackageArchiveDownloader()
    {
        // always clean up our extraction dir: if we successfully downloaded
        // and installed it will be an empty dir, if we failed it might contain
        // (some) of the package files.
        Dir d(m_extractPath);
        if (d.exists()) {
            d.remove(true /* recursive */);
        }
    }

    size_t downloadedBytes() const
    {
        return m_downloaded;
    }

    int percentDownloaded() const
    {
        if (responseLength() <= 0) {
            return 0;
        }

        return (m_downloaded * 100) / responseLength();
    }
protected:
    virtual std::string url() const
    {
        return m_urls.front();
    }

    virtual void responseHeadersComplete()
    {
        Request::responseHeadersComplete();

        Dir d(m_extractPath);
        d.create(0755);

        memset(&m_md5, 0, sizeof(SG_MD5_CTX));
        SG_MD5Init(&m_md5);
    }

    virtual void gotBodyData(const char* s, int n)
    {
        m_buffer += std::string(s, n);
        SG_MD5Update(&m_md5, (unsigned char*) s, n);

        m_downloaded = m_buffer.size();
        m_owner->installProgress(m_buffer.size(), responseLength());
    }

    virtual void onDone()
    {
        if (responseCode() != 200) {
            SG_LOG(SG_GENERAL, SG_ALERT, "download failure:" << responseCode() <<
                   "\n\t" << url());
            Delegate::StatusCode code = Delegate::FAIL_DOWNLOAD;
            if (responseCode() == 404) {
                code = Delegate::FAIL_NOT_FOUND;
            }

            doFailure(code);
            return;
        }

        unsigned char digest[MD5_DIGEST_LENGTH];
        SG_MD5Final(digest, &m_md5);
        std::string const hex_md5 =
          strutils::encodeHex(digest, MD5_DIGEST_LENGTH);

        if (hex_md5 != m_owner->package()->md5()) {
            SG_LOG(SG_GENERAL, SG_ALERT, "md5 verification failed:\n"
                << "\t" << hex_md5 << "\n\t"
                << m_owner->package()->md5() << "\n\t"
                << "downloading from:" << url());
            doFailure(Delegate::FAIL_CHECKSUM);
            return;
        }

        if (!extractUnzip()) {
            SG_LOG(SG_GENERAL, SG_WARN, "zip extraction failed");
            doFailure(Delegate::FAIL_EXTRACT);
            return;
        }

        if (m_owner->path().exists()) {
            Dir destDir(m_owner->path());
            destDir.remove(true /* recursive */);
        }

        // build a path like /path/to/packages/org.some.catalog/Aircraft/extract_xxxx/MyAircraftDir
        SGPath extractedPath = m_extractPath;
        extractedPath.append(m_owner->package()->dirName());

        // rename it to path/to/packages/org.some.catalog/Aircraft/MyAircraftDir
        bool ok = extractedPath.rename(m_owner->path());
        if (!ok) {
            doFailure(Delegate::FAIL_FILESYSTEM);
            return;
        }

        // extract_xxxx directory is now empty, so remove it
        if (m_extractPath.exists()) {
            simgear::Dir(m_extractPath).remove();
        }

        m_owner->m_revision = m_owner->package()->revision();
        m_owner->writeRevisionFile();
        m_owner->m_download.reset(); // so isDownloading reports false

        m_owner->installResult(Delegate::STATUS_SUCCESS);
    }

    virtual void onFail()
    {
        if (responseCode() == -1) {
            doFailure(Delegate::USER_CANCELLED);
        } else {
            doFailure(Delegate::FAIL_DOWNLOAD);
        }
    }

private:

    void extractCurrentFile(unzFile zip, char* buffer, size_t bufferSize)
    {
        unz_file_info fileInfo;
        unzGetCurrentFileInfo(zip, &fileInfo,
            buffer, bufferSize,
            NULL, 0,  /* extra field */
            NULL, 0 /* comment field */);

        std::string name(buffer);
    // no absolute paths, no 'up' traversals
    // we could also look for suspicious file extensions here (forbid .dll, .exe, .so)
        if ((name[0] == '/') || (name.find("../") != std::string::npos) || (name.find("..\\") != std::string::npos)) {
            throw sg_format_exception("Bad zip path", name);
        }

        if (fileInfo.uncompressed_size == 0) {
            // assume it's a directory for now
            // since we create parent directories when extracting
            // a path, we're done here
            return;
        }

        int result = unzOpenCurrentFile(zip);
        if (result != UNZ_OK) {
            throw sg_io_exception("opening current zip file failed", sg_location(name));
        }

        std::ofstream outFile;
        bool eof = false;
        SGPath path(m_extractPath);
        path.append(name);

    // create enclosing directory heirarchy as required
        Dir parentDir(path.dir());
        if (!parentDir.exists()) {
            bool ok = parentDir.create(0755);
            if (!ok) {
                throw sg_io_exception("failed to create directory heirarchy for extraction", path.c_str());
            }
        }

        outFile.open(path.c_str(), std::ios::binary | std::ios::trunc | std::ios::out);
        if (outFile.fail()) {
            throw sg_io_exception("failed to open output file for writing", path.c_str());
        }

        while (!eof) {
            int bytes = unzReadCurrentFile(zip, buffer, bufferSize);
            if (bytes < 0) {
                throw sg_io_exception("unzip failure reading curent archive", sg_location(name));
            } else if (bytes == 0) {
                eof = true;
            } else {
                outFile.write(buffer, bytes);
            }
        }

        outFile.close();
        unzCloseCurrentFile(zip);
    }

    bool extractUnzip()
    {
        bool result = true;
        zlib_filefunc_def memoryAccessFuncs;
        fill_memory_filefunc(&memoryAccessFuncs);

        char bufferName[128];
        snprintf(bufferName, 128, "%p+%lx", m_buffer.data(), m_buffer.size());
        unzFile zip = unzOpen2(bufferName, &memoryAccessFuncs);

        const size_t BUFFER_SIZE = 32 * 1024;
        void* buf = malloc(BUFFER_SIZE);

        try {
            int result = unzGoToFirstFile(zip);
            if (result != UNZ_OK) {
                throw sg_exception("failed to go to first file in archive");
            }

            while (true) {
                extractCurrentFile(zip, (char*) buf, BUFFER_SIZE);
                result = unzGoToNextFile(zip);
                if (result == UNZ_END_OF_LIST_OF_FILE) {
                    break;
                } else if (result != UNZ_OK) {
                    throw sg_io_exception("failed to go to next file in the archive");
                }
            }
        } catch (sg_exception& ) {
            result = false;
        }

        free(buf);
        unzClose(zip);
        return result;
    }

    void doFailure(Delegate::StatusCode aReason)
    {
        Dir dir(m_extractPath);
        if (dir.exists()) {
            dir.remove(true /* recursive */);
        }

        // TODO - try other mirrors
        m_owner->m_download.reset(); // ensure we get cleaned up
        m_owner->installResult(aReason);
    }

    InstallRef m_owner;
    string_list m_urls;
    SG_MD5_CTX m_md5;
    std::string m_buffer;
    SGPath m_extractPath;
    size_t m_downloaded;
};

////////////////////////////////////////////////////////////////////
Install::Install(PackageRef aPkg, const SGPath& aPath) :
    m_package(aPkg),
    m_path(aPath),
    m_status(Delegate::STATUS_IN_PROGRESS)
{
    parseRevision();
    m_package->catalog()->root()->registerInstall(this);
}

Install::~Install()
{
}

InstallRef Install::createFromPath(const SGPath& aPath, CatalogRef aCat)
{
    std::string path = aPath.file();
    PackageRef pkg = aCat->getPackageByPath(path);
    if (!pkg)
        throw sg_exception("no package with path:" + path);

    return new Install(pkg, aPath);
}

void Install::parseRevision()
{
    SGPath revisionFile = m_path;
    revisionFile.append(".revision");
    if (!revisionFile.exists()) {
        m_revision = 0;
        return;
    }

    std::ifstream f(revisionFile.c_str(), std::ios::in);
    f >> m_revision;
}

void Install::writeRevisionFile()
{
    SGPath revisionFile = m_path;
    revisionFile.append(".revision");
    std::ofstream f(revisionFile.c_str(), std::ios::out | std::ios::trunc);
    f << m_revision << std::endl;
}

bool Install::hasUpdate() const
{
    return m_package->revision() > m_revision;
}

void Install::startUpdate()
{
    if (m_download) {
        return; // already active
    }

    m_download = new PackageArchiveDownloader(this);
    m_package->catalog()->root()->makeHTTPRequest(m_download);
    m_package->catalog()->root()->startInstall(this);
}

bool Install::uninstall()
{
    Dir d(m_path);
    if (!d.remove(true)) {
        SG_LOG(SG_GENERAL, SG_ALERT, "package uninstall failed: couldn't remove path " << m_path);
        return false;
    }

    m_package->catalog()->root()->unregisterInstall(this);
    return true;
}

bool Install::isDownloading() const
{
    return (m_download.valid());
}

bool Install::isQueued() const
{
    return m_package->catalog()->root()->isInstallQueued(const_cast<Install*>(this));
}

int Install::downloadedPercent() const
{
    if (!m_download.valid()) {
        return -1;
    }

    PackageArchiveDownloader* dl = static_cast<PackageArchiveDownloader*>(m_download.get());
    return dl->percentDownloaded();
}

size_t Install::downloadedBytes() const
{
    if (!m_download.valid()) {
        return -1;
    }

    PackageArchiveDownloader* dl = static_cast<PackageArchiveDownloader*>(m_download.get());
    return dl->downloadedBytes();

}

void Install::cancelDownload()
{
    if (m_download.valid()) {
        m_package->catalog()->root()->cancelHTTPRequest(m_download, "User cancelled download");
    }

    if (m_revision == 0) {
        SG_LOG(SG_GENERAL, SG_INFO, "cancel install of package, will unregister");
        m_package->catalog()->root()->unregisterInstall(this);
    }

    m_package->catalog()->root()->cancelDownload(this);
}

struct PathAppender
{
    PathAppender(const SGPath& p) : m_path(p) {}

    SGPath operator()(const std::string& s) const
    {
        SGPath p(m_path);
        p.append(s);
        return p;
    }

    SGPath m_path;
};

PathList Install::thumbnailPaths() const
{
    const string_list& thumbs(m_package->thumbnails());
    PathList result;
    if (thumbs.empty())
        return result;

    std::transform(thumbs.begin(), thumbs.end(),
                   std::back_inserter(result),
                   PathAppender(m_path));
    return result;
}

SGPath Install::primarySetPath() const
{
    SGPath setPath(m_path);
    std::string ps(m_package->id());
    setPath.append(ps + "-set.xml");
    return setPath;
}

//------------------------------------------------------------------------------
Install* Install::done(const Callback& cb)
{
  if( m_status == Delegate::STATUS_SUCCESS )
    cb(this);
  else
    _cb_done.push_back(cb);

  return this;
}

//------------------------------------------------------------------------------
Install* Install::fail(const Callback& cb)
{
  if(    m_status != Delegate::STATUS_SUCCESS
      && m_status != Delegate::STATUS_IN_PROGRESS )
    cb(this);
  else
    _cb_fail.push_back(cb);

  return this;
}

//------------------------------------------------------------------------------
Install* Install::always(const Callback& cb)
{
  if( m_status != Delegate::STATUS_IN_PROGRESS )
    cb(this);
  else
    _cb_always.push_back(cb);

  return this;
}

//------------------------------------------------------------------------------
Install* Install::progress(const ProgressCallback& cb)
{
  _cb_progress.push_back(cb);
  return this;
}

//------------------------------------------------------------------------------
void Install::installResult(Delegate::StatusCode aReason)
{
    m_package->catalog()->root()->finishInstall(this, aReason);
    if (aReason == Delegate::STATUS_SUCCESS) {
        _cb_done(this);
    } else {
        _cb_fail(this);
    }

    _cb_always(this);
}

//------------------------------------------------------------------------------
void Install::installProgress(unsigned int aBytes, unsigned int aTotal)
{
  m_package->catalog()->root()->installProgress(this, aBytes, aTotal);
  _cb_progress(this, aBytes, aTotal);
}


} // of namespace pkg

} // of namespace simgear
