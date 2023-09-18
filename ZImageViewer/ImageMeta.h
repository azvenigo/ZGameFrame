#pragma once

#include "ZTypes.h"
#include <filesystem>
#include "helpers/ThreadPool.h"
#include "ZBuffer.h"

const std::string kImageListExt(".imlist");
const uint32_t kImageListTAG = 0xDAFA0002;


class ImageMetaEntry
{
public:
    ImageMetaEntry(const std::string& _filename = "", int64_t _size = 0, int64_t _contests = 0, int64_t _wins = 0, int64_t _elo = 1000)
    {
        filename = _filename;
        filesize = _size;
        contests = _contests;
        wins = _wins;
        elo = _elo;
        mpThumb = nullptr;


    };

    int32_t     ReadEntry(const uint8_t* pData);        // fills out entry and returns number of bytes processed
    int32_t     WriteEntry(uint8_t* pDest);             // writes to buffer and 

    // persisted metadata
    std::string filename;  // just the filename, no path
    int64_t     filesize;  // filename + nFileSize will be considered a unique image
    int32_t     contests;  // number of times the image has been shown 
    int32_t     wins;      // number of times the image has been selected
    int32_t     elo;        // rating

    // not persisted
    tZBufferPtr Thumbnail();    // lazy loads

private:
    tZBufferPtr mpThumb;
};

typedef std::list<ImageMetaEntry> tImageMetaList;
typedef std::map<uint16_t, tImageMetaList> tImageSizeToMeta;  // image metadata will be stored in lists keyed by size mod 16k, so maximum 16k maps. Should be reasonably fast to search through

typedef std::list<std::filesystem::path>    tImageFilenames;

class ImageMeta
{
public:
    ImageMetaEntry& Entry(const std::string& filename, int64_t size = -1);  // if size is -1, retrieve size from filesystem

    bool LoadAll();
    bool Save();        // each list will be stored in a separate file, filename being the size mod 16k
    bool SaveBucket(int64_t size);

    tImageSizeToMeta    mSizeToMetaLists;
    std::filesystem::path basePath;

protected:
    bool Load(const std::filesystem::path& imagelist);
    std::filesystem::path   BucketFilename(int64_t size);

    std::recursive_mutex mMutex;
};

extern ImageMeta gImageMeta;