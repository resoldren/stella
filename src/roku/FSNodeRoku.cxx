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
      _path = "/ext1:";
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
  // File must actually exist
  if(!(exists() && isReadable()))
    throw runtime_error("File not found/readable");

  Roku::BrightScript::roByteArray bytes;
  Roku::BrightScript::roBoolean ans = bytes.ReadFile(_path.substr(1));
  if (!ans.getBool()) {
      throw runtime_error("Error reading file");
  }

  int length = bytes.Count().getInt();
  if (length < 1) {
    throw runtime_error("File was empty");
  }

  image = make_ptr<uInt8[]>(length);
  std::string byteString = bytes.ToHexString().getString();
  for (int i = 0; i < length; ++i) {
    image.get()[i] = std::stoul(byteString.substr(i * 2, 2), 0, 16);
  }
  return length;
}
