// $Id: Geant4Setup.cpp 578 2013-05-17 22:33:09Z markus.frank $
//====================================================================
//  AIDA Detector description implementation for LCD
//--------------------------------------------------------------------
//
//  Author     : M.Frank
//
//====================================================================
// Framework include files
#include "DD4hep/LCDD.h"
#include "DD4hep/Printout.h"
#include "DD4hep/DetectorTools.h"
#include "DDAlign/AlignmentCache.h"
#include "DDAlign/AlignmentOperators.h"

// ROOT include files
#include "TGeoManager.h"

// C/C++ include files
#include <stdexcept>

using namespace std;
using namespace DD4hep;
using namespace DD4hep::Geometry;
using namespace DD4hep::Geometry::DDAlign_standard_operations;

typedef AlignmentStack::StackEntry Entry;

namespace {
  /// one-at-time hash function
  inline unsigned int hash32(const char* key) {
    unsigned int hash = 0;
    const char* k = key;
    for (; *k; k++) {
      hash += *k; 
      hash += (hash << 10); 
      hash ^= (hash >> 6);
    }
    hash += (hash << 3); 
    hash ^= (hash >> 11); hash += (hash << 15);
    return hash;
  }

}

DetElement _detector(DetElement child)   {
  if ( child.isValid() )   {
    DetElement p(child.parent());
    if ( p.isValid() && !p.parent().isValid() )
      return child;
    else if ( !p.isValid() )  // World detector element...
      return child;
    return _detector(p);
  }
  throw runtime_error("DD4hep: DetElement cannot determine detector parent [Invalid handle]");
}

/// Default constructor
AlignmentCache::AlignmentCache(LCDD& lcdd, const std::string& sdPath, bool top)
  : m_lcdd(lcdd), m_sdPath(sdPath), m_sdPathLen(sdPath.length()), m_refCount(1), m_top(top)
{
}

/// Default destructor
AlignmentCache::~AlignmentCache()   {
  int nentries = (int)m_cache.size();
  int nsect = (int)m_detectors.size();
  releaseObjects(m_detectors)();
  m_cache.clear();
  printout(INFO,"AlignmentCache",
	   "Destroy cache for subdetector %s [%d section(s), %d entrie(s)]",
	   m_sdPath.c_str(),nsect,nentries);
}

/// Add reference count
int AlignmentCache::addRef()   {
  return ++m_refCount;
}

/// Release object. If reference count goes to NULL, automatic deletion is triggered.
int AlignmentCache::release()   {
  int value = --m_refCount;
  if ( value == 0 )  {
    delete this;
  }
  return value;
}

/// Add a new entry to the cache. The key is the placement path
bool AlignmentCache::insert(Alignment alignment)  {
  TGeoPhysicalNode* pn = alignment.ptr();
  unsigned int index = hash32(pn->GetName()+m_sdPathLen);
  Cache::const_iterator i = m_cache.find(index);
  printout(ALWAYS,"AlignmentCache","Section: %s adding entry: %s",
	   name().c_str(),alignment->GetName());
  if ( i == m_cache.end() )   {
    m_cache[index] = pn;
    return true;
  }
  return false;
}

/// Retrieve the cache section corresponding to the path of an entry.
AlignmentCache* AlignmentCache::section(const std::string& path_name) const   {
  size_t idx, idq;
  if ( path_name[0] != '/' )   {
    return section(m_lcdd.world().placementPath()+'/'+path_name);
  }
  else if ( (idx=path_name.find('/',1)) == string::npos )  {
    return (m_sdPath == path_name.c_str()+1) ? (AlignmentCache*)this : 0;
  }
  else if ( m_detectors.empty() )  {
    return 0;
  }
  if ( (idq=path_name.find('/',idx+1)) != string::npos ) --idq;
  string path = path_name.substr(idx+1,idq-idx);
  SubdetectorAlignments::const_iterator j = m_detectors.find(path);
  return (j==m_detectors.end()) ? 0 : (*j).second;
}

/// Retrieve an alignment entry by its lacement path
Alignment AlignmentCache::get(const std::string& path_name) const   {
  size_t idx, idq;
  unsigned int index = hash32(path_name.c_str()+m_sdPathLen);
  Cache::const_iterator i = m_cache.find(index);
  if ( i != m_cache.end() )  {
    return Alignment((*i).second);
  }
  else if ( m_detectors.empty() )  {
    return Alignment(0);
  }
  else if ( path_name[0] != '/' )   {
    return get(m_lcdd.world().placementPath()+'/'+path_name);
  }
  else if ( (idx=path_name.find('/',1)) == string::npos )  {
    // Escape: World volume and not found in cache --> not present
    return Alignment(0);
  }
  if ( (idq=path_name.find('/',idx+1)) != string::npos ) --idq;
  string path = path_name.substr(idx+1,idq-idx);
  SubdetectorAlignments::const_iterator j = m_detectors.find(path);
  if ( j != m_detectors.end() ) return (*j).second->get(path_name);
  return Alignment(0);
}

/// Return all entries matching a given path. 
vector<Alignment> AlignmentCache::matches(const string& match, bool exclude_exact) const   {
  vector<Alignment> result;
  AlignmentCache* c = section(match);
  if ( c )  {
    size_t len = match.length();
    result.reserve(c->m_cache.size());
    for(Cache::const_iterator i=c->m_cache.begin(); i!=c->m_cache.end();++i)  {
      const Cache::value_type& v = *i;
      const char* n = v.second->GetName();
      if ( 0 == ::strncmp(n,match.c_str(),len) )   {
	if ( exclude_exact && len == ::strlen(n) ) continue;
	result.push_back(Alignment(v.second));
      }
    }
  }
  return result;
}

/// Open a new transaction stack (Note: only one stack allowed!)
void AlignmentCache::openTransaction()   {
  /// Check if transaction already present. If not, open, else issue an error
  if ( !AlignmentStack::exists() )  {
    AlignmentStack::create();
    return;
  }
  string msg = "Request to open a second alignment transaction stack -- not allowed!";
  string err = format("Alignment<alignment>",msg);
  printout(FATAL,"AlignmentCache",msg);
  throw runtime_error(err);
}

/// Close existing transaction stack and apply all alignments
void AlignmentCache::closeTransaction()   {
  /// Check if transaction is open. If yes, close it and apply alignment stack.
  /// If no transaction is active, ignore the staement, but issue a warning.
  if ( AlignmentStack::exists() )  {
    TGeoManager& mgr = m_lcdd.manager();
    mgr.UnlockGeometry();
    apply(AlignmentStack::get());
    AlignmentStack::get().release();
    printout(INFO,"AlignmentCache","Alignments were applied. Refreshing physical nodes....");
    mgr.LockGeometry();
    mgr.GetCurrentNavigator()->ResetAll();
    mgr.GetCurrentNavigator()->BuildCache();
    mgr.RefreshPhysicalNodes();
    return;
  }
  printout(WARNING,"Alignment<alignment>",
	   "Request to close a non-existing alignmant transaction.");
}

/// Create and install a new instance tree
void AlignmentCache::install(LCDD& lcdd)   {
  AlignmentCache* cache = lcdd.extension<AlignmentCache>(false);
  if ( !cache )  {
    lcdd.addExtension<AlignmentCache>(new AlignmentCache(lcdd,"world",true));
  }
}

/// Unregister and delete a tree instance
void AlignmentCache::uninstall(LCDD& lcdd)   {
  if ( lcdd.extension<AlignmentCache>(false) )  {
    lcdd.removeExtension<AlignmentCache>(true);
  }
}

/// Retrieve branch cache by name. If not present it will be created
AlignmentCache* AlignmentCache::subdetectorAlignments(const std::string& name)    {
  SubdetectorAlignments::const_iterator i = m_detectors.find(name);
  if ( i == m_detectors.end() )   {
    AlignmentCache* ptr = new AlignmentCache(m_lcdd,name,false);
    m_detectors.insert(make_pair(name,ptr));
    return ptr;
  }
  return (*i).second;
}

/// Apply a complete stack of ordered alignments to the geometry structure
void AlignmentCache::apply(AlignmentStack& stack)    {
  typedef map<TNamed*,vector<Entry*> > sd_entries_t;
  sd_entries_t all;
  while(stack.size() > 0)    {
    Entry* e = stack.pop().release();
    DetElement det = _detector(e->detector);
    all[det.ptr()].push_back(e);
  }
  for(sd_entries_t::iterator i=all.begin(); i!=all.end(); ++i)  {
    DetElement det(Ref_t((*i).first));
    AlignmentCache* sd_cache = subdetectorAlignments(det.placement().name());
    sd_cache->apply( (*i).second );
    (*i).second.clear();
  }
}


/// Apply a vector of SD entries of ordered alignments to the geometry structure
void AlignmentCache::apply(const vector<Entry*>& changes)   {
  typedef std::map<std::string,std::pair<TGeoPhysicalNode*,Entry*> > Nodes;
  typedef vector<Entry*> Changes;
  Nodes nodes;

  AlignmentSelector selector(*this,nodes,changes);
  for_each(m_cache.begin(),m_cache.end(),selector.reset());
  for_each(nodes.begin(),nodes.end(),AlignmentActor<node_print>(*this,nodes));
  for_each(nodes.begin(),nodes.end(),AlignmentActor<node_reset>(*this,nodes));

  for_each(changes.begin(),changes.end(),selector.reset());
  for_each(nodes.begin(),nodes.end(),AlignmentActor<node_align>(*this,nodes));
  for_each(nodes.begin(),nodes.end(),AlignmentActor<node_delete>(*this,nodes));
}