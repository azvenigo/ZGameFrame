#include "ZThumbCache.h"
#include "ZDebug.h"
#include "helpers/StringHelpers.h"
#include "ZGUIHelpers.h"

ZThumbCache gThumbCache;

bool ZThumbCache::Init(const std::filesystem::path& thumbCachePath)
{
    if (!std::filesystem::exists(thumbCachePath))
    {
        ZERROR("Cannot initialize thumb cache, ", thumbCachePath, " doesn't exist.\n");
        return false;
    }

    mThumbCachePath = thumbCachePath;
    mbInitted = true;

    return true;
}

bool ZThumbCache::Add(const std::filesystem::path& imagePath, tZBufferPtr image)
{
    assert(mbInitted);

    size_t hash = PathHash(imagePath);

    ZRect r(image->GetArea());
    ZRect rThumb = ZGUI::ScaledFit(r, ZRect(0,0,kThumbDimensions.x,kThumbDimensions.y));


    ZThumbEntry& entry = mFilenameToThumbEntry[hash];
    if (!entry.thumb)
    {
        entry.thumb.reset(new ZBuffer());
    }

    entry.thumb->Init(rThumb.Width(), rThumb.Height());
    entry.thumb->BltScaled(image.get());

    std::filesystem::path thumbPath(ThumbPath(imagePath));
    entry.thumb->SaveBuffer(thumbPath.string());
    entry.writeTime = std::filesystem::last_write_time(thumbPath);

    ZDEBUG_OUT("Wrote thumb ", thumbPath, " for image:", imagePath, "\n");

    return true;
}

tZBufferPtr ZThumbCache::GetThumb(const std::filesystem::path& imagePath)
{
    size_t hash = PathHash(imagePath);

    tFilenameToThumbEntry::iterator findit = mFilenameToThumbEntry.find(hash);
    if (findit == mFilenameToThumbEntry.end())
        return nullptr;

    assert((*findit).second.thumb);
    return (*findit).second.thumb;
}


size_t ZThumbCache::PathHash(const std::filesystem::path& p)
{
    std::hash<std::filesystem::path> h;
    return h(p);
}


std::filesystem::path ZThumbCache::ThumbPath(const std::filesystem::path& imagePath)
{
    std::string sThumbHashedFilename(std::to_string(PathHash(imagePath)) + ".jpg");

    std::filesystem::path thumbPath(mThumbCachePath);
    thumbPath.append(sThumbHashedFilename);

    return thumbPath;
}


void ZThumbCache::DeleteOldest(int32_t nCount)
{
}



