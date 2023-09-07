#pragma once

#include <map>
#include <filesystem>
#include "ZBuffer.h"
#include "ZTypes.h"

const int64_t kMaxThumbs = 100;
const ZPoint kThumbDimensions(256, 256);

class ZThumbEntry
{
public:
    std::filesystem::file_time_type writeTime;
    tZBufferPtr thumb;
};

typedef std::map<size_t, ZThumbEntry> tFilenameToThumbEntry;


class ZThumbCache
{
public:
    ZThumbCache() : mMaxThumbs(kMaxThumbs)
    {
    }

    bool        Init(const std::filesystem::path& thumbCachePath);

    bool        Add(const std::filesystem::path& imagePath, tZBufferPtr image);
    tZBufferPtr GetThumb(const std::filesystem::path& imagePath);

    std::filesystem::path   ThumbPath(const std::filesystem::path& imagePath); // returns a full path to the thumbnail for an image path

protected:
    size_t                  PathHash(const std::filesystem::path& p);


    void                    DeleteOldest(int32_t nCount);

    bool                    mbInitted;
    int64_t                 mMaxThumbs;
    tFilenameToThumbEntry   mFilenameToThumbEntry;

    std::filesystem::path   mThumbCachePath;
};

extern ZThumbCache gThumbCache;

