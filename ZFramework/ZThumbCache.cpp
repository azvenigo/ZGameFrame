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


    const std::lock_guard<std::recursive_mutex> surfaceLock(mCacheMutex);
    ZThumbEntry& entry = mFilenameToThumbEntry[hash];
    if (!entry.thumb)
    {
        entry.thumb.reset(new ZBuffer());
    }

    entry.thumb->Init(rThumb.Width(), rThumb.Height());
    entry.thumb->BltScaled(image.get());

    std::filesystem::path thumbPath(ThumbPath(imagePath));

    assert(!std::filesystem::exists(thumbPath));

    entry.thumb->SaveBuffer(thumbPath.string());
    entry.writeTime = std::filesystem::last_write_time(thumbPath);

    ZDEBUG_OUT("Wrote thumb ", thumbPath, " for image:", imagePath, "\n");

    return true;
}

tZBufferPtr ZThumbCache::GetThumb(const std::filesystem::path& imagePath, bool bGenerateIfMissing)
{
    size_t hash = PathHash(imagePath);

    const std::lock_guard<std::recursive_mutex> surfaceLock(mCacheMutex);

    tFilenameToThumbEntry::iterator findit = mFilenameToThumbEntry.find(hash);
    if (findit != mFilenameToThumbEntry.end())
    {
        assert((*findit).second.thumb);
        return (*findit).second.thumb;
    }

    // not already loaded..... see if thumbnail available

    std::filesystem::path thumbPath(ThumbPath(imagePath));
    if (std::filesystem::exists(thumbPath))
    {
        tZBufferPtr thumb(new ZBuffer());
        if (thumb->LoadBuffer(thumbPath.string()))
        {
            ZDEBUG_OUT("Loaded thumb for image:", imagePath, "\n");
            size_t hash = PathHash(imagePath);
            ZThumbEntry& entry = mFilenameToThumbEntry[hash];
            entry.thumb = thumb;
            entry.writeTime = std::filesystem::last_write_time(thumbPath);
            return thumb;
        }

    }

    if (bGenerateIfMissing)
    {
        ZDEBUG_OUT("Generating missing thumb:", imagePath, "\n");
        tZBufferPtr original(new ZBuffer());
        if (original->LoadBuffer(imagePath.string()))
        {
            Add(imagePath, original);
            return original;
        }
    }


    return nullptr;
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



