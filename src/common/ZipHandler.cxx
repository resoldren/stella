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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <zlib.h>

#include "ZipHandler.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::ZipHandler()
  : myZip(nullptr)
{
  for(int cachenum = 0; cachenum < ZIP_CACHE_SIZE; ++cachenum)
    myZipCache[cachenum] = nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ZipHandler::~ZipHandler()
{
  zip_file_cache_clear();
  free_zip_file(myZip);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::open(const string& filename)
{
  // Close already open file
  if(myZip)
    zip_file_close(myZip);

  // And open a new one
  zip_file_open(filename.c_str(), &myZip);
  reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::reset()
{
  // Reset the position and go from there
  if(myZip)
    myZip->cd_pos = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::hasNext()
{
  return myZip && (myZip->cd_pos < myZip->ecd.cd_size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string ZipHandler::next()
{
  if(myZip)
  {
    bool valid = false;
    const zip_file_header* header = nullptr;
    do {
      header = zip_file_next_file(myZip);

      // Ignore zero-length files and '__MACOSX' virtual directories
      valid = header && (header->uncompressed_length > 0) &&
              !BSPF::startsWithIgnoreCase(header->filename, "__MACOSX");
    }
    while(!valid && myZip->cd_pos < myZip->ecd.cd_size);

    return valid ? header->filename : EmptyString;
  }
  else
    return EmptyString;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 ZipHandler::decompress(BytePtr& image)
{
  static const char* zip_error_s[] = {
    "ZIPERR_NONE",
    "ZIPERR_OUT_OF_MEMORY",
    "ZIPERR_FILE_ERROR",
    "ZIPERR_BAD_SIGNATURE",
    "ZIPERR_DECOMPRESS_ERROR",
    "ZIPERR_FILE_TRUNCATED",
    "ZIPERR_FILE_CORRUPT",
    "ZIPERR_UNSUPPORTED",
    "ZIPERR_BUFFER_TOO_SMALL"
  };

  if(myZip)
  {
    uInt32 length = myZip->header.uncompressed_length;
    image = make_ptr<uInt8[]>(length);

    ZipHandler::zip_error err = zip_file_decompress(myZip, image.get(), length);
    if(err == ZIPERR_NONE)
      return length;
    else
      throw runtime_error(zip_error_s[err]);
  }
  else
    throw runtime_error("Invalid ZIP archive");
}

/*-------------------------------------------------
    replaces functionality of various osd_xxx
    file access functions
-------------------------------------------------*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::stream_open(const char* filename, istream** stream,
                             uInt64& length)
{
  istream* in = new fstream(filename, fstream::in | fstream::binary);
  if(!in || in->fail())
  {
    *stream = nullptr;
    length = 0;
    return false;
  }
  else
  {
    in->exceptions( std::ios_base::failbit | std::ios_base::badbit | std::ios_base::eofbit );
    *stream = in;
    in->seekg(0, std::ios::end);
    length = in->tellg();
    in->seekg(0, std::ios::beg);
    return true;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ZipHandler::stream_close(istream** stream)
{
  if(*stream)
  {
    delete *stream;
    *stream = nullptr;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ZipHandler::stream_read(istream* stream, void* buffer, uInt64 offset,
                             uInt32 length, uInt32& actual)
{
  try
  {
    stream->seekg(offset);
    stream->read((char*)buffer, length);

    actual = (uInt32)stream->gcount();
    return true;
  }
  catch(...)
  {
    return false;
  }
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*-------------------------------------------------
    zip_file_open - opens a ZIP file for reading
-------------------------------------------------*/
ZipHandler::zip_error ZipHandler::zip_file_open(const char* filename, zip_file** zip)
{
  zip_error ziperr = ZIPERR_NONE;
  uInt32 read_length;
  zip_file* newzip;
  char* string;
  int cachenum;
  bool success;

  // Ensure we start with a NULL result
  *zip = nullptr;

  // See if we are in the cache, and reopen if so
  for(cachenum = 0; cachenum < ZIP_CACHE_SIZE; ++cachenum)
  {
    zip_file* cached = myZipCache[cachenum];

    // If we have a valid entry and it matches our filename, use it and remove
    // from the cache
    if(cached != nullptr && cached->filename != nullptr &&
       strcmp(filename, cached->filename) == 0)
    {
      *zip = cached;
      myZipCache[cachenum] = nullptr;
      return ZIPERR_NONE;
    }
  }

  // Allocate memory for the zip_file structure
  newzip = (zip_file*)malloc(sizeof(*newzip));
  if (newzip == nullptr)
    return ZIPERR_OUT_OF_MEMORY;
  memset(newzip, 0, sizeof(*newzip));

  // Open the file
  if(!stream_open(filename, &newzip->file, newzip->length))
  {
    ziperr = ZIPERR_FILE_ERROR;
    goto error;
  }

  // Read ecd data
  ziperr = read_ecd(newzip);
  if(ziperr != ZIPERR_NONE)
    goto error;

  // Verify that we can work with this zipfile (no disk spanning allowed)
  if (newzip->ecd.disk_number != newzip->ecd.cd_start_disk_number ||
      newzip->ecd.cd_disk_entries != newzip->ecd.cd_total_entries)
  {
    ziperr = ZIPERR_UNSUPPORTED;
    goto error;
  }

  // Allocate memory for the central directory
  newzip->cd = (uInt8*)malloc(newzip->ecd.cd_size + 1);
  if(newzip->cd == nullptr)
  {
    ziperr = ZIPERR_OUT_OF_MEMORY;
    goto error;
  }

  // Read the central directory
  success = stream_read(newzip->file, newzip->cd, newzip->ecd.cd_start_disk_offset,
                        newzip->ecd.cd_size, read_length);
  if(!success || read_length != newzip->ecd.cd_size)
  {
    ziperr = success ? ZIPERR_FILE_TRUNCATED : ZIPERR_FILE_ERROR;
    goto error;
  }

  // Make a copy of the filename for caching purposes
  string = (char*)malloc(strlen(filename) + 1);
  if (string == nullptr)
  {
    ziperr = ZIPERR_OUT_OF_MEMORY;
    goto error;
  }

  strcpy(string, filename);
  newzip->filename = string;
  *zip = newzip;

  // Count ROM files (we do it at this level so it will be cached)
  while(hasNext())
  {
    const std::string& file = next();
    if(BSPF::endsWithIgnoreCase(file, ".a26") ||
       BSPF::endsWithIgnoreCase(file, ".bin") ||
       BSPF::endsWithIgnoreCase(file, ".rom"))
      (*zip)->romfiles++;
  }

  return ZIPERR_NONE;

error:
  free_zip_file(newzip);
  return ziperr;
}


/*-------------------------------------------------
    zip_file_close - close a ZIP file and add it
    to the cache
-------------------------------------------------*/
void ZipHandler::zip_file_close(zip_file* zip)
{
  int cachenum;

  // Close the open files
  if(zip->file)
    stream_close(&zip->file);

  // Find the first NULL entry in the cache
  for(cachenum = 0; cachenum < ZIP_CACHE_SIZE; ++cachenum)
    if(myZipCache[cachenum] == nullptr)
      break;

  // If no room left in the cache, free the bottommost entry
  if(cachenum == ZIP_CACHE_SIZE)
    free_zip_file(myZipCache[--cachenum]);

  // Move everyone else down and place us at the top
  if(cachenum != 0)
    memmove(&myZipCache[1], &myZipCache[0], cachenum * sizeof(myZipCache[0]));
  myZipCache[0] = zip;
}


/*-------------------------------------------------
    zip_file_cache_clear - clear the ZIP file
    cache and free all memory
-------------------------------------------------*/
void ZipHandler::zip_file_cache_clear()
{
  // Clear call cache entries
  for(int cachenum = 0; cachenum < ZIP_CACHE_SIZE; ++cachenum)
  {
    if(myZipCache[cachenum] != nullptr)
    {
      free_zip_file(myZipCache[cachenum]);
      myZipCache[cachenum] = nullptr;
    }
  }
}


/***************************************************************************
    CONTAINED FILE ACCESS
***************************************************************************/

/*-------------------------------------------------
    zip_file_next_entry - return the next entry
    in the ZIP
-------------------------------------------------*/
const ZipHandler::zip_file_header* ZipHandler::zip_file_next_file(zip_file* zip)
{
  // Fix up any modified data
  if(zip->header.raw != nullptr)
  {
    zip->header.raw[ZIPCFN + zip->header.filename_length] = zip->header.saved;
    zip->header.raw = nullptr;
  }

  // If we're at or past the end, we're done
  if(zip->cd_pos >= zip->ecd.cd_size)
    return nullptr;

  // Extract file header info
  zip->header.raw                 = zip->cd + zip->cd_pos;
  zip->header.rawlength           = ZIPCFN;
  zip->header.signature           = read_dword(zip->header.raw + ZIPCENSIG);
  zip->header.version_created     = read_word (zip->header.raw + ZIPCVER);
  zip->header.version_needed      = read_word (zip->header.raw + ZIPCVXT);
  zip->header.bit_flag            = read_word (zip->header.raw + ZIPCFLG);
  zip->header.compression         = read_word (zip->header.raw + ZIPCMTHD);
  zip->header.file_time           = read_word (zip->header.raw + ZIPCTIM);
  zip->header.file_date           = read_word (zip->header.raw + ZIPCDAT);
  zip->header.crc                 = read_dword(zip->header.raw + ZIPCCRC);
  zip->header.compressed_length   = read_dword(zip->header.raw + ZIPCSIZ);
  zip->header.uncompressed_length = read_dword(zip->header.raw + ZIPCUNC);
  zip->header.filename_length     = read_word (zip->header.raw + ZIPCFNL);
  zip->header.extra_field_length  = read_word (zip->header.raw + ZIPCXTL);
  zip->header.file_comment_length = read_word (zip->header.raw + ZIPCCML);
  zip->header.start_disk_number   = read_word (zip->header.raw + ZIPDSK);
  zip->header.internal_attributes = read_word (zip->header.raw + ZIPINT);
  zip->header.external_attributes = read_dword(zip->header.raw + ZIPEXT);
  zip->header.local_header_offset = read_dword(zip->header.raw + ZIPOFST);
  zip->header.filename            = (char*)zip->header.raw + ZIPCFN;

  // Make sure we have enough data
  zip->header.rawlength += zip->header.filename_length;
  zip->header.rawlength += zip->header.extra_field_length;
  zip->header.rawlength += zip->header.file_comment_length;
  if(zip->cd_pos + zip->header.rawlength > zip->ecd.cd_size)
    return nullptr;

  // NULL terminate the filename
  uInt32 size = ZIPCFN + zip->header.filename_length;
  zip->header.saved = zip->header.raw[size];
  zip->header.raw[size] = 0;

  // Advance the position
  zip->cd_pos += zip->header.rawlength;
  return &zip->header;
}

/*-------------------------------------------------
    zip_file_decompress - decompress a file
    from a ZIP into the target buffer
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::zip_file_decompress(zip_file* zip, void* buffer, uInt32 length)
{
  zip_error ziperr;
  uInt64 offset;

  // If we don't have enough buffer, error
  if(length < zip->header.uncompressed_length)
    return ZIPERR_BUFFER_TOO_SMALL;

  // Make sure the info in the header aligns with what we know
  if(zip->header.start_disk_number != zip->ecd.disk_number)
    return ZIPERR_UNSUPPORTED;

  // Get the compressed data offset
  ziperr = get_compressed_data_offset(zip, offset);
  if(ziperr != ZIPERR_NONE)
    return ziperr;

  // Handle compression types
  switch(zip->header.compression)
  {
    case 0:
      ziperr = decompress_data_type_0(zip, offset, buffer, length);
      break;

    case 8:
      ziperr = decompress_data_type_8(zip, offset, buffer, length);
      break;

    default:
      ziperr = ZIPERR_UNSUPPORTED;
      break;
  }
  return ziperr;
}

/***************************************************************************
    CACHE MANAGEMENT
***************************************************************************/

/*-------------------------------------------------
    free_zip_file - free all the data for a
    zip_file
-------------------------------------------------*/
void ZipHandler::free_zip_file(zip_file* zip)
{
  if(zip != nullptr)
  {
    if(zip->file)
      stream_close(&zip->file);
    if(zip->filename != nullptr)
      free((void*)zip->filename);
    if(zip->ecd.raw != nullptr)
      free(zip->ecd.raw);
    if(zip->cd != nullptr)
      free(zip->cd);
    free(zip);
  }
}

/***************************************************************************
    ZIP FILE PARSING
***************************************************************************/

/*-------------------------------------------------
    read_ecd - read the ECD data
-------------------------------------------------*/

ZipHandler::zip_error ZipHandler::read_ecd(zip_file* zip)
{
  uInt32 buflen = 1024;
  uInt8* buffer;

  // We may need multiple tries
  while(buflen < 65536)
  {
    uInt32 read_length;
    Int32 offset;

    // Max out the buffer length at the size of the file
    if(buflen > zip->length)
      buflen = (uInt32)zip->length;

    // Allocate buffer
    buffer = (uInt8*)malloc(buflen + 1);
    if(buffer == nullptr)
      return ZIPERR_OUT_OF_MEMORY;

    // Read in one buffers' worth of data
    bool success = stream_read(zip->file, buffer, zip->length - buflen,
                               buflen, read_length);
    if(!success || read_length != buflen)
    {
      free(buffer);
      return ZIPERR_FILE_ERROR;
    }

    // Find the ECD signature
    for(offset = buflen - 22; offset >= 0; offset--)
      if(buffer[offset + 0] == 'P' && buffer[offset + 1] == 'K' &&
         buffer[offset + 2] == 0x05 && buffer[offset + 3] == 0x06)
        break;

    // If we found it, fill out the data
    if(offset >= 0)
    {
      // Reuse the buffer as our ECD buffer
      zip->ecd.raw = buffer;
      zip->ecd.rawlength = buflen - offset;

      // Append a NULL terminator to the comment
      memmove(&buffer[0], &buffer[offset], zip->ecd.rawlength);
      zip->ecd.raw[zip->ecd.rawlength] = 0;

      // Extract ecd info
      zip->ecd.signature            = read_dword(zip->ecd.raw + ZIPESIG);
      zip->ecd.disk_number          = read_word (zip->ecd.raw + ZIPEDSK);
      zip->ecd.cd_start_disk_number = read_word (zip->ecd.raw + ZIPECEN);
      zip->ecd.cd_disk_entries      = read_word (zip->ecd.raw + ZIPENUM);
      zip->ecd.cd_total_entries     = read_word (zip->ecd.raw + ZIPECENN);
      zip->ecd.cd_size              = read_dword(zip->ecd.raw + ZIPECSZ);
      zip->ecd.cd_start_disk_offset = read_dword(zip->ecd.raw + ZIPEOFST);
      zip->ecd.comment_length       = read_word (zip->ecd.raw + ZIPECOML);
      zip->ecd.comment              = (const char *)(zip->ecd.raw + ZIPECOM);
      return ZIPERR_NONE;
    }

    // Didn't find it; free this buffer and expand our search
    free(buffer);
    if(buflen < zip->length)
      buflen *= 2;
    else
      return ZIPERR_BAD_SIGNATURE;
  }
  return ZIPERR_OUT_OF_MEMORY;
}

/*-------------------------------------------------
    get_compressed_data_offset - return the
    offset of the compressed data
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::get_compressed_data_offset(zip_file* zip, uInt64& offset)
{
  uInt32 read_length;

  // Make sure the file handle is open
  if(zip->file == nullptr && !stream_open(zip->filename, &zip->file, zip->length))
    return ZIPERR_FILE_ERROR;

  // Now go read the fixed-sized part of the local file header
  bool success = stream_read(zip->file, zip->buffer, zip->header.local_header_offset,
                             ZIPNAME, read_length);
  if(!success || read_length != ZIPNAME)
    return success ? ZIPERR_FILE_TRUNCATED : ZIPERR_FILE_ERROR;

  // Compute the final offset
  offset = zip->header.local_header_offset + ZIPNAME;
  offset += read_word(zip->buffer + ZIPFNLN);
  offset += read_word(zip->buffer + ZIPXTRALN);

  return ZIPERR_NONE;
}

/***************************************************************************
    DECOMPRESSION INTERFACES
***************************************************************************/

/*-------------------------------------------------
    decompress_data_type_0 - "decompress"
    type 0 data (which is uncompressed)
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::decompress_data_type_0(zip_file* zip, uInt64 offset,
                                       void* buffer, uInt32 length)
{
  uInt32 read_length;

  // The data is uncompressed; just read it
  bool success = stream_read(zip->file, buffer, offset,
                             zip->header.compressed_length, read_length);
  if(!success)
    return ZIPERR_FILE_ERROR;
  else if(read_length != zip->header.compressed_length)
    return ZIPERR_FILE_TRUNCATED;
  else
    return ZIPERR_NONE;
}

/*-------------------------------------------------
    decompress_data_type_8 - decompress
    type 8 data (which is deflated)
-------------------------------------------------*/
ZipHandler::zip_error
    ZipHandler::decompress_data_type_8(zip_file* zip, uInt64 offset,
                                       void* buffer, uInt32 length)
{
  uInt32 input_remaining = zip->header.compressed_length;
  uInt32 read_length;
  z_stream stream;
  int zerr;

#if 0
  // TODO - check newer versions of ZIP, and determine why this specific
  //        version (0x14) is important
  // Make sure we don't need a newer mechanism
  if (zip->header.version_needed > 0x14)
    return ZIPERR_UNSUPPORTED;
#endif

  // Reset the stream
  memset(&stream, 0, sizeof(stream));
  stream.next_out = (Bytef *)buffer;
  stream.avail_out = length;

  // Initialize the decompressor
  zerr = inflateInit2(&stream, -MAX_WBITS);
  if(zerr != Z_OK)
    return ZIPERR_DECOMPRESS_ERROR;

  // Loop until we're done
  for(;;)
  {
    // Read in the next chunk of data
    bool success = stream_read(zip->file, zip->buffer, offset,
                      std::min(input_remaining, (uInt32)sizeof(zip->buffer)),
                      read_length);
    if(!success)
    {
      inflateEnd(&stream);
      return ZIPERR_FILE_ERROR;
    }
    offset += read_length;

    // If we read nothing, but still have data left, the file is truncated
    if(read_length == 0 && input_remaining > 0)
    {
      inflateEnd(&stream);
      return ZIPERR_FILE_TRUNCATED;
    }

    // Fill out the input data
    stream.next_in = zip->buffer;
    stream.avail_in = read_length;
    input_remaining -= read_length;

    // Add a dummy byte at end of compressed data
    if(input_remaining == 0)
      stream.avail_in++;

    // Now inflate
    zerr = inflate(&stream, Z_NO_FLUSH);
    if(zerr == Z_STREAM_END)
      break;
    if(zerr != Z_OK)
    {
      inflateEnd(&stream);
      return ZIPERR_DECOMPRESS_ERROR;
    }
  }

  // Finish decompression
  zerr = inflateEnd(&stream);
  if(zerr != Z_OK)
    return ZIPERR_DECOMPRESS_ERROR;

  // If anything looks funny, report an error
  if(stream.avail_out > 0 || input_remaining > 0)
    return ZIPERR_DECOMPRESS_ERROR;

  return ZIPERR_NONE;
}
