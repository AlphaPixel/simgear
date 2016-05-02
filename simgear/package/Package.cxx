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

#include <simgear/package/Package.hxx>

#include <cassert>
#include <boost/foreach.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <simgear/debug/logstream.hxx> 
#include <simgear/structure/exception.hxx>

#include <simgear/package/Catalog.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Root.hxx>

namespace simgear {
    
namespace pkg {

Package::Package(const SGPropertyNode* aProps, CatalogRef aCatalog) :
    m_catalog(aCatalog)
{
    initWithProps(aProps);
}

void Package::initWithProps(const SGPropertyNode* aProps)
{
    m_props = const_cast<SGPropertyNode*>(aProps);
// cache tag values
    BOOST_FOREACH(const SGPropertyNode* c, aProps->getChildren("tag")) {
      std::string t(c->getStringValue());
      m_tags.insert(boost::to_lower_copy(t));
    }

    m_id = m_props->getStringValue("id");
}

void Package::updateFromProps(const SGPropertyNode* aProps)
{
    m_tags.clear();
    initWithProps(aProps);
}

bool Package::matches(const SGPropertyNode* aFilter) const
{
    int nChildren = aFilter->nChildren();
    for (int i = 0; i < nChildren; i++) {
        const SGPropertyNode* c = aFilter->getChild(i);
        const std::string& filter_name = c->getNameString();

        if (strutils::starts_with(filter_name, "rating-")) {
            int minRating = c->getIntValue();
            std::string rname = c->getName() + 7;
            int ourRating = m_props->getChild("rating")->getIntValue(rname, 0);
            if (ourRating < minRating) {
                return false;
            }
        }
        else if (filter_name == "tag") {
            std::string tag(c->getStringValue());
            boost::to_lower(tag);
            if (m_tags.find(tag) == m_tags.end()) {
                return false;
            }
        }
        // substring search of name, description
        else if (filter_name == "name") {
          std::string n(c->getStringValue());
          boost::to_lower(n);
          size_t pos = boost::to_lower_copy(name()).find(n);
          if (pos == std::string::npos) {
            return false;
          }
        }
        else if (filter_name == "description") {
          std::string n(c->getStringValue());
          boost::to_lower(n);
          size_t pos = boost::to_lower_copy(description()).find(n);
          if (pos == std::string::npos) {
            return false;
          }
        }
        else if (filter_name == "installed") {
          if (isInstalled() != c->getBoolValue()) {
            return false;
          }
        }
        else
          SG_LOG(SG_GENERAL, SG_WARN, "unknown filter term:" << filter_name);
    } // of filter props iteration
    
    return true;
}

bool Package::isInstalled() const
{
    // anything to check for? look for a valid revision file?
    return pathOnDisk().exists();
}

SGPath Package::pathOnDisk() const
{
    SGPath p(m_catalog->installRoot());
    p.append("Aircraft");
    p.append(dirName());
    return p;
}

InstallRef Package::install()
{
    InstallRef ins = existingInstall();
    if (ins) {
        return ins;
    }
  
  // start a new install
    ins = new Install(this, pathOnDisk());
    m_catalog->root()->scheduleToUpdate(ins);

    _install_cb(this, ins);

    return ins;
}

InstallRef Package::existingInstall(const InstallCallback& cb) const
{
    InstallRef install;
    try {
        install = m_catalog->root()->existingInstallForPackage(const_cast<Package*>(this));
    } catch (std::exception& ) {
        return InstallRef();
    }

  if( cb )
  {
    _install_cb.push_back(cb);

    if( install )
      cb(const_cast<Package*>(this), install);
  }

  return install;
}

std::string Package::id() const
{
    return m_id;
}

std::string Package::qualifiedId() const
{
    return m_catalog->id() + "." + id();
}

std::string Package::qualifiedVariantId(const unsigned int variantIndex) const
{
    return m_catalog->id() + "." + variants()[variantIndex];
}

std::string Package::md5() const
{
    return m_props->getStringValue("md5");
}

std::string Package::dirName() const
{
    std::string r(m_props->getStringValue("dir"));
    if (r.empty())
        throw sg_exception("missing dir property on catalog package entry for " + m_id);
    return r;
}

unsigned int Package::revision() const
{
    if (!m_props) {
        return 0;
    }
    
    return m_props->getIntValue("revision");
}
    
std::string Package::name() const
{
    return m_props->getStringValue("name");
}

size_t Package::fileSizeBytes() const
{
    return m_props->getIntValue("file-size-bytes");
}
  
std::string Package::description() const
{
    return getLocalisedProp("description");
}

string_set Package::tags() const
{
    return m_tags;
}
    
SGPropertyNode* Package::properties() const
{
    return m_props.ptr();
}

string_list Package::thumbnailUrls() const
{
    string_list r;
    if (!m_props) {
        return r;
    }
    
    BOOST_FOREACH(SGPropertyNode* dl, m_props->getChildren("thumbnail")) {
        r.push_back(dl->getStringValue());
    }
    return r;
}

string_list Package::thumbnails() const
{
    string_list r;
    if (!m_props) {
        return r;
    }
    
    BOOST_FOREACH(SGPropertyNode* dl, m_props->getChildren("thumbnail-path")) {
        r.push_back(dl->getStringValue());
    }
    return r;
}
    
string_list Package::downloadUrls() const
{
    string_list r;
    if (!m_props) {
        return r;
    }
    
    BOOST_FOREACH(SGPropertyNode* dl, m_props->getChildren("url")) {
        r.push_back(dl->getStringValue());
    }
    return r;
}

std::string Package::getLocalisedProp(const std::string& aName) const
{
    return getLocalisedString(m_props, aName.c_str());
}

std::string Package::getLocalisedString(const SGPropertyNode* aRoot, const char* aName) const
{
    std::string locale = m_catalog->root()->getLocale();
    if (aRoot->hasChild(locale)) {
        const SGPropertyNode* localeRoot = aRoot->getChild(locale.c_str());
        if (localeRoot->hasChild(aName)) {
            return localeRoot->getStringValue(aName);
        }
    }
    
    return aRoot->getStringValue(aName);
}

PackageList Package::dependencies() const
{
    PackageList result;
    
    BOOST_FOREACH(SGPropertyNode* dep, m_props->getChildren("depends")) {
        std::string depName = dep->getStringValue("id");
        unsigned int rev = dep->getIntValue("revision", 0);
        
    // prefer local hangar package if possible, in case someone does something
    // silly with naming. Of course flightgear's aircraft search doesn't know
    // about hangars, so names still need to be unique.
        PackageRef depPkg = m_catalog->getPackageById(depName);
        if (!depPkg) {   
            Root* rt = m_catalog->root();
            depPkg = rt->getPackageById(depName);
            if (!depPkg) {
                throw sg_exception("Couldn't satisfy dependency of " + id() + " : " + depName);
            }
        }
        
        if (depPkg->revision() < rev) {
            throw sg_range_exception("Couldn't find suitable revision of " + depName);
        }
    
    // forbid recursive dependency graphs, we don't need that level
    // of complexity for aircraft resources
        assert(depPkg->dependencies() == PackageList());
        
        result.push_back(depPkg);
    }
    
    return result;
}

string_list Package::variants() const
{
    string_list result;
    result.push_back(id());

    BOOST_FOREACH(SGPropertyNode* var, m_props->getChildren("variant")) {
        result.push_back(var->getStringValue("id"));
    }

    return result;
}

std::string Package::nameForVariant(const std::string& vid) const
{
    if (vid == id()) {
        return name();
    }

    BOOST_FOREACH(SGPropertyNode* var, m_props->getChildren("variant")) {
        if (vid == var->getStringValue("id")) {
            return var->getStringValue("name");
        }
    }


    throw sg_exception("Unknow variant +" + vid + " in package " + id());
}

std::string Package::nameForVariant(const unsigned int vIndex) const
{
    if (vIndex == 0)
        return name();

    // offset by minus one to allow for index 0 being the primary
    SGPropertyNode_ptr var = m_props->getChild("variant", vIndex - 1);
    if (var)
        return var->getStringValue("name");

    throw sg_exception("Unknow variant in package " + id());
}


} // of namespace pkg

} // of namespace simgear
