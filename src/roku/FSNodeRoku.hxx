//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2016 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FSNodeROKU.hxx 3239 2015-12-29 19:22:46Z stephena $
//============================================================================

#ifndef FS_NODE_ROKU_HXX
#define FS_NODE_ROKU_HXX

#include "FSNode.hxx"

#ifdef MACOSX
  #include <sys/types.h>
#endif

#include <sys/param.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <sstream>

#ifndef MAXPATHLEN // No MAXPATHLEN, as happens on Hurd
  #define MAXPATHLEN 1024
#endif

/*
 * Implementation of the Stella file system API based on Roku (for Linux and OSX)
 *
 * Parts of this class are documented in the base interface class, AbstractFSNode.
 */
class FilesystemNodeRoku : public AbstractFSNode
{
  public:
    /**
     * Creates a FilesystemNodeRoku with the root node as path.
     */
    FilesystemNodeRoku();

    /**
     * Creates a FilesystemNodeRoku for a given path.
     *
     * @param path    String with the path the new node should point to.
     * @param verify  true if the isValid and isDirectory/isFile flags should
     *                be verified during the construction.
     */
    FilesystemNodeRoku(const string& path, bool verify = true);

    bool exists() const override { return _isValid; }
    const string& getName() const override { return _displayName; }
    const string& getPath() const override { return _path; }
    string getShortPath() const override;
    bool isDirectory() const override { return _isDirectory; }
    bool isFile() const override      { return _isFile;      }
    bool isReadable() const override  { return exists(); }
    bool isWritable() const override  { return false; }
    bool makeDir() override;
    bool rename(const string& newfile) override;
    uInt32 read(BytePtr& buffer) const override;

    bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const override;
    AbstractFSNode* getParent() const override;

  protected:
    string _path;
    string _displayName;
    bool _isValid;
    bool _isFile;
    bool _isDirectory;

  private:
    /**
     * Tests and sets the _isValid and _isDirectory/_isFile flags,
     * using the stat() function.
     */
    virtual void setFlags();

    /**
     * Returns the last component of a given path.
     *
     * Examples:
     *			/foo/bar.txt would return /bar.txt
     *			/foo/bar/    would return /bar/
     *
     * @param str String containing the path.
     * @return Pointer to the first char of the last component inside str.
     */
    static const char* lastPathComponent(const string& str)
    {
      if(str.empty())
        return "";

      const char* start = str.c_str();
      const char* cur = start + str.size() - 2;

      while (cur >= start && *cur != '/')
        --cur;

      return cur + 1;
    }

};

#endif
