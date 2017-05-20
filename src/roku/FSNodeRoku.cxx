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
//============================================================================

#include "FSNodeRoku.hxx"
#include <ndkwrappers/roObject.h>
#include <ndkwrappers/roAssociativeArray.h>
#include <ndkwrappers/roByteArray.h>
#include <ndkwrappers/roList.h>
#include <ndkwrappers/roFileSystem.h>
#include "zlib.h"

static Roku::BrightScript::roFileSystem& getFileSystem()
{
    static Roku::BrightScript::roFileSystem fs;
    return fs;
}

static std::string getStatPath(const std::string &path)
{
    if (path.back() == ':' && path.rfind('/') == 0) {
        return path.substr(1) + "/";
    } else {
        return path.substr(1);
    }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void FilesystemNodeRoku::setFlags()
{
  printf("setFlags(%s)\n", _path.c_str());
  if (_path == "/") {
      printf("  root\n");
    _isValid = true;
    _isFile = false;
    _isDirectory = true;
  } else {
#if 0
      Roku::BrightScript::roAssociativeArray info(getFileSystem().GetVolumeInfo(_path));
      std::string label;
      if (info.DoesExist("label").getBool()) {
          label = info.Lookup("label").getString();
      } else {
          label = _path;
      }
#endif
    Roku::BrightScript::roAssociativeArray stats(getFileSystem().Stat(getStatPath(_path)));
    printf("lookup type: %s\n", stats.Lookup("type").getType().c_str());
    while (stats.IsNext().getBool()) {
        auto obj = stats.Next();
        printf("next: %s\n", obj.toString().c_str());
    }
    Roku::BrightScript::roString type(stats.Lookup("type"));
    printf("string type: %s\n", type.getType().c_str());
    if (type.getType() == "roString") {
      if (type.getString() == "directory") {
        printf("  dir\n");
        _isFile = false;
        _isDirectory = true;
      } else if (type.getString() == "file") {
        printf("  file\n");
        _isFile = true;
        _isDirectory = false;
      } else {
        printf("  neither\n");
        _isValid = false;
        _isFile = false;
        _isDirectory = false;
      }
      if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '/') {
        _path += '/';
      }
    } else {
      printf("  invalid\n");
      _isValid = false;
      _isFile = false;
      _isDirectory = false;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeRoku::FilesystemNodeRoku()
  : _path("/"),          // The root dir.
    _displayName(_path),
    _isValid(true),
    _isFile(false),
    _isDirectory(true)
{
  printf("construct root (/)\n");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
FilesystemNodeRoku::FilesystemNodeRoku(const string& p, bool verify)
  : _isValid(true),
    _isFile(false),
    _isDirectory(true)
{
  printf("construct (%s)\n", p.c_str());
  // Default to home directory
  _path = p.length() > 0 ? p : "/";

  // Expand '~' to the HOME environment variable
  if(_path[0] == '~') {
    if (_path.length() == 1)
      _path = "/";
    else
      _path.replace(0, 1, "");
  }

  _displayName = lastPathComponent(_path);

  if(verify)
    setFlags();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string FilesystemNodeRoku::getShortPath() const
{
  return _path;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeRoku::getChildren(AbstractFSList& myList, ListMode mode,
                                      bool hidden) const
{
  printf("get children(%s)\n", _path.c_str());
  assert(_isDirectory);

  auto fs = getFileSystem();
  Roku::BrightScript::roObject listObj;
  if (_path == "/") {
      listObj = fs.GetVolumeList();
  } else {
      listObj = fs.GetDirectoryListing(_path.substr(1));
  }
  if (listObj.getType() != "roList") {
      printf("not list\n");
      return false;
  }
  Roku::BrightScript::roList list(listObj);
  int count = list.Count().getInt();
  printf("count: %d\n", count);
  for (int i = 0; i < count; ++i) {
    Roku::BrightScript::roString bsName(list.GetEntry(i));
    std::string name = bsName.getString();
    if (_path != "/" || 0 == name.compare(0, 3, "ext")) {
        printf("  item %d: %s\n", i, name.c_str());

      if (name.length() == 0)
        continue;

      // Skip 'invisible' files if necessary
      if (name[0] == '.') {
        if (!hidden)
          continue;
        if (name == "." || name == "..")
          continue;
      }

      string newPath(_path);
      if (newPath.length() > 0 && newPath[newPath.length()-1] != '/')
        newPath += '/';
      newPath += name;

      printf("child: %s\n", newPath.c_str());
      FilesystemNodeRoku entry(newPath, true);

      // Skip files that are invalid for some reason (e.g. because we couldn't
      // properly stat them).
      if (!entry._isValid) {
        printf("  not valid, skipping\n");
        continue;
      }

      // Honor the chosen mode
      if ((mode == FilesystemNode::kListFilesOnly && !entry._isFile) ||
          (mode == FilesystemNode::kListDirectoriesOnly && !entry._isDirectory)) {
          printf("  skip reason 2\n");
        continue;
      }

      printf("A\n");
      myList.emplace_back(new FilesystemNodeRoku(entry));
      printf("B\n");
    }
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeRoku::makeDir()
{
  if(mkdir(_path.c_str(), 0777) == 0)
  {
    // Get absolute path  
    char buf[MAXPATHLEN];
    if(realpath(_path.c_str(), buf))
      _path = buf;

    _displayName = lastPathComponent(_path);
    setFlags();

    // Add a trailing slash, if necessary
    if (_path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';
    
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool FilesystemNodeRoku::rename(const string& newfile)
{
  if(std::rename(_path.c_str(), newfile.c_str()) == 0)
  {
    _path = newfile;

    // Get absolute path  
    char buf[MAXPATHLEN];
    if(realpath(_path.c_str(), buf))
      _path = buf;

    _displayName = lastPathComponent(_path);
    setFlags();

    // Add a trailing slash, if necessary
    if (_isDirectory && _path.length() > 0 && _path[_path.length()-1] != '/')
      _path += '/';
    
    return true;
  }
  else
    return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AbstractFSNode* FilesystemNodeRoku::getParent() const
{
  if (_path == "/")
    return nullptr;

  const char* start = _path.c_str();
  const char* end = lastPathComponent(_path);

  return new FilesystemNodeRoku(string(start, size_t(end - start)));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 FilesystemNodeRoku::read(BytePtr& image) const
{
    printf("xread for %s\n", _path.c_str());
    printf("valid: %s, dir: %s file: %s\n", _isValid ? "y" : "n", _isDirectory ? "y" : "n", _isFile ? "y" : "n");
  uInt32 size = 0;

  // File must actually exist
  if(!(exists() && isReadable()))
    throw runtime_error("File not found/readable");

  Roku::BrightScript::roByteArray bytes;
  Roku::BrightScript::roBoolean ans = bytes.ReadFile(_path.substr(1));
  if (!ans.getBool()) {
      throw runtime_error("Error reading file");
  }

  int count = bytes.Count().getInt();
  printf("BYTES IN FILE: %d\n", count);
  if (count < 1) {
    throw runtime_error("File was empty");
  }

  int ret;
  unsigned have;
  z_stream strm;

  std::unique_ptr<unsigned char[]> in = std::make_unique<unsigned char[]>(count);

  for (int i = 0; i < count; ++i) {
    in[i] = static_cast<unsigned char>(bytes.GetSignedByte(i).getInt());
    printf("byte %d is %d\n", i, in[i]);
  }
  printf("Done reading\n");

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  printf("3\n");
  if (inflateInit2(&(strm), 15/* + 16*/) != Z_OK) {    /* gunzip */
      throw runtime_error("ZLIB init error");
  }
  printf("4\n");

  auto imageSize = 512 * 1024;
  image = make_ptr<uInt8[]>(imageSize);
  printf("5\n");
#if 1
  /* decompress until deflate stream ends or end of file */
  do {

      strm.avail_in = count;
//      if (ferror(source)) {
//          (void)inflateEnd(&strm);
//          return Z_ERRNO;
//      }
//      if (strm.avail_in == 0)
//          break;
      strm.next_in = in.get();

      /* run inflate() on input until output buffer not full */
      do {
          printf("6\n");

          strm.avail_out = imageSize;
          strm.next_out = image.get() + size;
          printf("7\n");

          ret = inflate(&strm, Z_NO_FLUSH);
          printf("7.2: %d\n", ret);
          if (ret == Z_STREAM_ERROR) {
              throw runtime_error("ZLIB stream error");
          }
          printf("8\n");
          //assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
          switch (ret) {
          case Z_NEED_DICT:
              printf("8.1\n");
              throw runtime_error("ZLIB dictionary error");
              // ret = Z_DATA_ERROR;     /* and fall through */
          case Z_DATA_ERROR:
              printf("8.2\n");
              (void)inflateEnd(&strm);
              throw runtime_error("ZLIB data error");
          case Z_MEM_ERROR:
              printf("8.3\n");
              (void)inflateEnd(&strm);
              throw runtime_error("ZLIB memory error");
          }
          printf("9\n");

          have = imageSize - strm.avail_out;
          size += have;
//          if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
//              (void)inflateEnd(&strm);
//              return Z_ERRNO;
//          }
          printf("10\n");

      } while (strm.avail_out == 0);
      printf("11\n");

      /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);
  printf("12\n");

  /* clean up and return */
  (void)inflateEnd(&strm);
  if (ret == Z_STREAM_END) {
      return size;
  } else {
      throw runtime_error("ZLIB data error");
  }
//  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
#endif

#if 0
  // Otherwise, assume the file is either gzip'ed or not compressed at all
  gzFile f = gzopen(getPath().c_str(), "rb");
  if(f)
  {
    image = make_ptr<uInt8[]>(512 * 1024);
    size = gzread(f, image.get(), 512 * 1024);
    gzclose(f);

    if(size == 0)
      throw runtime_error("Zero-byte file");

    return size;
  }
  else
    throw runtime_error("ZLIB open/read error");
#endif
}
