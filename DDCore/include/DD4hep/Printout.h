// $Id: Handle.h 570 2013-05-17 07:47:11Z markus.frank $
//====================================================================
//  AIDA Detector description implementation for LCD
//--------------------------------------------------------------------
//
//  Author     : M.Frank
//
//====================================================================

#ifndef DD4HEP_PRINTOUT_H
#define DD4HEP_PRINTOUT_H

// Framework include files
//#include "DD4hep/Handle.h"

// C/C++ include files
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>

/*
 *   DD4hep namespace declaration
 */
namespace DD4hep {
  
  /// Forward declarations
  namespace Geometry {    
    template <typename T> struct Handle;
    class LCDD;
    class VisAttr;
    class DetElement;
    class PlacedVolume;
  }

  enum PrintLevel {
    NOLOG=0,
    VERBOSE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL,
    ALWAYS
  };

  typedef size_t (*output_function_t)(void*, PrintLevel severity, const char*, const char*);

  /** Calls the display action
   *  @arg severity   [int,read-only]      Display severity flag (see enum)
   *  @arg src        [string,read-only]   Information source (component, etc.)
   *  @arg fmt        [string,read-only]   Format string for ellipsis args
   *  @return Status code indicating success or failure
   */
  int printout(PrintLevel severity, const char* src, const char* fmt, ...);

  /// Set new print level. Returns the old print level
  PrintLevel setPrintLevel(PrintLevel new_level);

  /// Customize printer function
  void setPrinter(void* print_arg, output_function_t fcn);

  /** @class Printer Conversions.h  DD4hep/compact/Conversions.h
   *
   *  Small helper class to print objects
   *
   *  @author   M.Frank
   *  @version  1.0
   */
  template <typename T> struct Printer  {
    /// Reference to the detector description object
    const Geometry::LCDD*   lcdd;
    /// Reference to the output stream object, the Printer object should write
    std::ostream&           os;
    /// Optional text prefix when formatting the output
    std::string             prefix;
    /// Initializing constructor of the functor
    Printer(const Geometry::LCDD* l, std::ostream& stream, const std::string& p="") 
    : lcdd(l), os(stream), prefix(p) {}
    /// Callback operator to be specialized depending on the element type
    void operator()(const T& value) const;
  };

  template <typename T> inline 
    std::ostream& print(const T& object,std::ostream& os=std::cout,const std::string& indent="")  {
    Printer<T>(0,os,indent)(object);
    return os;
  }

  /** @class PrintMap Conversions.h  DD4hep/compact/Conversions.h
   *
   *  Small helper class to print maps of objects
   *
   *  @author   M.Frank
   *  @version  1.0
   */
  template <typename T> struct PrintMap {
    typedef T item_type;
    typedef const std::map<std::string,Geometry::Handle<TNamed> > cont_type;

    /// Reference to the detector description object
    const Geometry::LCDD*   lcdd;
    /// Reference to the output stream object, the Printer object should write
    std::ostream&           os;
    /// Optional text prefix when formatting the output
    std::string             text;
    /// Reference to the container data of the map.
    cont_type&              cont;
    /// Initializing constructor of the functor
    PrintMap(const Geometry::LCDD* l, std::ostream& stream, cont_type& c, const std::string& t="") 
    : lcdd(l), os(stream), text(t), cont(c)  {}
    /// Callback operator to be specialized depending on the element type
    void operator()() const;
  };

  /// Helper function to print booleans in format YES/NO
  inline const char* yes_no(bool value)     { return value ? "YES"   : "NO ";   }
  /// Helper function to print booleans in format true/false
  inline const char* true_false(bool value) { return value ? "true " : "false"; }

}         /* End namespace DD4hep      */
#endif    /* DD4HEP_PRINTOUT_H         */