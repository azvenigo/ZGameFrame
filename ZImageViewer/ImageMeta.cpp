#include "ImageMeta.h"
#include "helpers/StringHelpers.h"
#include "helpers/LoggingHelpers.h"
#include "ZGUIStyle.h"
#include "ZFont.h"
#include "ZThumbCache.h"
#include <filesystem>

using namespace std;


ImageMeta gImageMeta;

std::filesystem::path ImageMeta::BucketFilename(int64_t size)
{
    uint16_t nSizeBucket = size % (16 * 1024);
    std::filesystem::path filename(basePath);
    filename.append(SH::FromInt(nSizeBucket));
    filename += kImageListExt;

    return filename;
}


ImageMetaEntry& ImageMeta::Entry(const std::string& filename, int64_t size)
{
    if (size < 0)
        size = std::filesystem::file_size(filename);

    uint16_t nSizeBucket = size % (16 * 1024);

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    tImageSizeToMeta::iterator findIt = mSizeToMetaLists.find(nSizeBucket);

    // See if it needs loading
    if (findIt == mSizeToMetaLists.end())
    {
        std::filesystem::path bucketFilename = BucketFilename(size);
        if (std::filesystem::exists(bucketFilename))
        {
            // try loading
            if (!Load(bucketFilename))
            {
                ZERROR("Failed to load bucket:", bucketFilename);
            }
        }
    }

    if (mSizeToMetaLists[nSizeBucket].empty())
    {
//        ZDEBUG_OUT("Creating new bucket:", nSizeBucket, " for size:", size);
        return mSizeToMetaLists[nSizeBucket].emplace_back(ImageMetaEntry(filename, size));
    }

    for (tImageMetaList::iterator imageIt = mSizeToMetaLists[nSizeBucket].begin(); imageIt != mSizeToMetaLists[nSizeBucket].end(); imageIt++)
    {
        if ((*imageIt).filename == filename)
        {
            //ZDEBUG_OUT("Found:", filename, " bucket:", nSizeBucket);
            return *imageIt;
        }
    }

    //ZDEBUG_OUT("Creating entry for :", filename, " bucket:", nSizeBucket);
    return mSizeToMetaLists[nSizeBucket].emplace_back(ImageMetaEntry(filename, size, 0, 0, 1000));
}

bool ImageMeta::LoadAll()
{
    if (basePath.empty())
        return false;
    if (!std::filesystem::exists(basePath))
        return false;

    for (auto filePath : std::filesystem::directory_iterator(basePath))
    {
        if (filePath.is_regular_file() && filePath.path().extension() == kImageListExt)
        {
            Load(filePath.path());
        }
    }

    return true;
}

bool ImageMeta::Load(const std::filesystem::path& imagelist)
{
    ifstream infile(imagelist, ios::binary);
    if (!infile)
    {
        cerr << "Failed to load " << imagelist << "\n";
        return false;
    }

    int64_t nSize = (int64_t)std::filesystem::file_size(imagelist);
    assert(nSize < 4 * 1024 * 1024);    // seems unusually large to be this large

    int64_t nImageSizes = SH::ToInt(imagelist.filename().string());

    assert(nImageSizes > 0 && nImageSizes < 32 * 1024);     // 16bit value

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    mSizeToMetaLists[(uint16_t)nImageSizes].clear();

    uint8_t* pBuf = new uint8_t[nSize];

    infile.read((char*)pBuf, nSize);

    uint8_t* pWalker = pBuf;
    int64_t nBytesLeft = nSize;

    if (*(uint32_t*)pWalker != kImageListTAG)
    {
        ZERROR("Image file:", imagelist, " doesn't start with tag:", kImageListTAG);
        return false;
    }
    pWalker += sizeof(uint32_t);

    uint32_t entries = *(uint32_t*)pWalker;
    pWalker += sizeof(uint32_t);

    nBytesLeft -= sizeof(uint32_t) * 2;


    int nEntriesProcessed = 0;
    while (nBytesLeft > 0)
    {
        ImageMetaEntry entry;
        int32_t nRead = entry.ReadEntry(pWalker);
        assert(nRead > 0 && nRead <= nBytesLeft);
        nBytesLeft -= nRead;
        pWalker += nRead;

        cout << "Read ImageMetaEntry: filename:" << entry.filename << " size:" << entry.filesize << " wins/contests:" << entry.wins << "/" << entry.contests << " elo:" << entry.elo << "\n";

        nEntriesProcessed++;
        if (entry.contests > 0)
            mSizeToMetaLists[(uint16_t)nImageSizes].emplace_back(std::move(entry));
    }
    assert(nEntriesProcessed == entries);
    if (nEntriesProcessed != entries)
    {
        ZERROR("Image file", imagelist, " was supposed to contain:", entries, " entries but only:", nEntriesProcessed, " parsed.");
    }

    delete[] pBuf;
    return true;
}

bool ImageMeta::Save()
{
    if (basePath.empty())
        return false;
    if (!std::filesystem::exists(basePath))
        return false;

    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    for (auto& bucket : mSizeToMetaLists)
    {
        SaveBucket(bucket.first);
    }

    return true;
}

bool ImageMeta::SaveBucket(int64_t size)
{
    uint16_t nSizeBucket = size % (16 * 1024);
    const std::lock_guard<std::recursive_mutex> lock(mMutex);
    tImageSizeToMeta::iterator findIt = mSizeToMetaLists.find(nSizeBucket);

    // See if it needs loading
    if (findIt == mSizeToMetaLists.end())
    {
        ZERROR("No bucket for filesize:", size, " bucket not found:", nSizeBucket);
        return false;
    }

    tImageMetaList& metaList = (*findIt).second;
    tImageMetaList filteredList;
    for (auto& entry : metaList)    // remove entries for missing files or that have had not had contests
    {
        if (filesystem::exists(entry.filename) && entry.contests > 0)
        {
            filteredList.push_back(entry);
        }
        else
        {
            ZWARNING("Removing entry for image not found on disk:", entry.filename);
        }
    }


    std::ofstream outBucket(BucketFilename(size), ios::binary | ios::trunc);

    outBucket.write((char*)&kImageListTAG, sizeof(uint32_t));
    uint32_t nEntries = (uint32_t)filteredList.size();
    outBucket.write((char*)&nEntries, sizeof(uint32_t));

    char buf[1024];

    for (auto& entry : filteredList)
    {
        int32_t bytes = entry.WriteEntry((uint8_t*)buf);
        assert(bytes < 1024);
        outBucket.write(buf, bytes);
    }

    return true;
}



int32_t ImageMetaEntry::ReadEntry(const uint8_t* pData)
{
    const uint8_t* pWalker = pData;
    uint8_t filenameSize = *pWalker;
    pWalker++;

    filename.assign((const char*)pWalker, filenameSize);
    pWalker += filenameSize;

    filesize = *((int64_t*)pWalker);
    pWalker += sizeof(int64_t);

    contests = *((int32_t*)pWalker);
    pWalker += sizeof(int32_t);

    wins = *((int32_t*)pWalker);
    pWalker += sizeof(int32_t);

    elo = *((int32_t*)pWalker);
    pWalker += sizeof(int32_t);

    return (int32_t)(pWalker - pData);
}

int32_t ImageMetaEntry::WriteEntry(uint8_t* pDest)
{
    uint8_t* pWriter = pDest;
    assert(filename.size() < 256);

    uint8_t size = (uint8_t) filename.size();
    *pWriter = size;
    pWriter++;

    memcpy(pWriter, filename.data(), size);
    pWriter += size;

    *(int64_t*)pWriter = filesize;
    pWriter += sizeof(int64_t);

    *(int32_t*)pWriter = contests;
    pWriter += sizeof(int32_t);

    *(int32_t*)pWriter = wins;
    pWriter += sizeof(int32_t);

    *(int32_t*)pWriter = elo;
    pWriter += sizeof(int32_t);

    return (int32_t)(pWriter - pDest);
}

tZBufferPtr ImageMetaEntry::Thumbnail()
{
    if (!mpThumb)
        mpThumb = gThumbCache.GetThumb(filename, true);

    if (!mpThumb && !filename.empty())
    {
        mpThumb.reset(new ZBuffer());
        mpThumb->Init(256, 256);
        mpThumb->Fill(0xff000000 | rand());

        ZGUI::ZTextLook look(ZGUI::ZTextLook::kNormal, 0xffffffff, 0xffffffff);
        gDefaultGroupingStyle.Font()->DrawTextA(mpThumb.get(), std::filesystem::path(filename).filename().string(), mpThumb->GetArea(), &look);
    }

    return mpThumb;
}


