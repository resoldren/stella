#include "bspf.hxx"
#include <fstream>
#include "FSNodeStream.hxx"
#include "FSNodeFactory.hxx"

WholeFileBuffer::WholeFileBuffer()
{
}

WholeFileBuffer::~WholeFileBuffer()
{
}

bool WholeFileBuffer::valid() const {
	return gptr() != nullptr;
}

std::streampos WholeFileBuffer::seekoff(std::streamoff off,
		std::ios_base::seekdir way, std::ios_base::openmode which)
{
	char* begin = eback();
	char* curOrig = gptr();
	char* cur = nullptr;
	char* end = egptr();
	switch (way) {
		case std::ios_base::cur:
			cur = curOrig + off;
			break;
		case std::ios_base::end:
			cur = end + off;
			break;
		case std::ios_base::beg:
			cur = begin + off;
			break;
		default:
			// cur is nullptr, so will return an error below
			break;
	}
	// printf("%p seekoff: %d %d->%d %d (%d %d %d)\n", this, 0, curOrig - begin, cur - begin, end - begin, off, way, which);
	if (cur == nullptr || cur < begin || cur > end) {
		return pos_type(off_type(-1));
	}
	setg(begin, cur, end);
	return pos_type(cur - begin);
}

std::streampos WholeFileBuffer::seekpos(std::streampos sp, std::ios_base::openmode which)
{
	return seekoff(off_type(sp), std::ios_base::beg, which);
}

void WholeFileBuffer::setBuffer(char* data, uInt32 length)
{
	setg(data, data, data + length);
}

FilesystemNodeBuffer::FilesystemNodeBuffer(const std::string& path)
{
  std::unique_ptr<AbstractFSNode> node(FilesystemNodeFactory::create(path, FilesystemNodeFactory::SYSTEM));
  try {
	  auto length = node->read(data);
	  if (data) {
		  setBuffer(reinterpret_cast<char*>(data.get()), length);
	  }
  } catch (...) {
  }
}

FilesystemNodeStream::FilesystemNodeStream(const std::string& path) :
	std::istream(nullptr),
	buffer(path)
{
	if (buffer.valid()) {
		rdbuf(&buffer);
	} else {
		setstate(std::ios_base::failbit);
	}
}

#ifdef SUPPORT_STANDARDFILE_STREAMING

StandardFileBuffer::StandardFileBuffer(const std::string& path)
{
  std::fstream in(path, std::fstream::in | std::fstream::binary);
  if (in)
  {
	in.seekg(0, std::ios::end);
	auto length = in.tellg();
	data = make_ptr<char[]>(length);
	in.seekg(0, std::ios::beg);
	if (in.read(data.get(), length)) {
		setBuffer(data.get(), length);
	} else {
		data.reset();
	}
  }
}

StandardFileStream::StandardFileStream(const std::string& path) :
	std::istream(nullptr),
	buffer(path)
{
	if (buffer.valid()) {
		rdbuf(&buffer);
	} else {
		setstate(std::ios_base::failbit);
	}
}

#endif

