#ifndef FSNODE_STREAM_HXX
#define FSNODE_STREAM_HXX

#include <streambuf>
#include <istream>

class WholeFileBuffer : public std::streambuf
{
public:
	WholeFileBuffer();
	virtual ~WholeFileBuffer();
	bool valid() const;

protected:
	std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way,
			std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
	std::streampos seekpos(std::streampos sp,
			std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
	void setBuffer(char* data, uInt32 length);

private:
	WholeFileBuffer(const WholeFileBuffer&) = delete;
	WholeFileBuffer(const WholeFileBuffer&&) = delete;
	WholeFileBuffer& operator=(const WholeFileBuffer&) = delete;
	WholeFileBuffer& operator=(const WholeFileBuffer&&) = delete;
};

class FilesystemNodeBuffer : public WholeFileBuffer
{
public:
	FilesystemNodeBuffer(const std::string& path);
private:
	BytePtr data;
};

class FilesystemNodeStream : public std::istream
{
public:
	FilesystemNodeStream(const std::string& path);
private:
	FilesystemNodeBuffer buffer;
};

#ifdef SUPPORT_STANDARDFILE_STREAMING

class StandardFileBuffer : public WholeFileBuffer
{
public:
	StandardFileBuffer(const std::string& path);
private:
	std::unique_ptr<char[]> data;
};

class StandardFileStream : public std::istream
{
public:
	StandardFileStream(const std::string& path);
private:
	StandardFileBuffer buffer;
};

#endif

#endif
