#pragma once

#include "ZTypes.h"
#include <filesystem>
#include "ZWin.h"
#include <future>
#include <limits>


class ZWinImage;


typedef std::list<std::filesystem::path>        tImageFilenames;
typedef std::shared_future< tZBufferPtr >       tImageFuture;
typedef std::pair< std::filesystem::path, tImageFuture>   tNamedImagePair;
typedef std::list< tNamedImagePair >            tImageCache;

class ImageViewer : public ZWin
{
public:
    ImageViewer();
    ~ImageViewer();
   
   bool		Init();
   bool     Process();
   bool		Paint();

   bool     ViewImage(const std::filesystem::path& filename);
   bool     SaveImage(const std::filesystem::path& filename);

   bool	    OnKeyDown(uint32_t key);
   bool     OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
   bool	    HandleMessage(const ZMessage& message);

   bool     AcceptedExtension(std::string sExt);
   bool     ShowOpenImageDialog();


   std::atomic<int64_t>     mMaxMemoryUsage;

protected:

    bool                    ScanForImagesInFolder(const std::filesystem::path& folder);
    bool                    InCache(const std::filesystem::path& imagePath);
    tImageFuture*           GetCached(const std::filesystem::path& imagePath);
    bool                    AddToCache(std::filesystem::path imagePath);
    int64_t                 CurMemoryUsage();       // only returns the in-memory bytes of buffers that have finished loading
    int64_t                 ImageIndexInFolder(std::filesystem::path imagePath);

    static tZBufferPtr      LoadImageProc(std::filesystem::path& imagePath, ImageViewer* pThis);

    bool                    Preload();
    void                    SetFirstImage();
    void                    SetLastImage();
    void                    SetNextImage();
    void                    SetPrevImage();


    ZWinImage* mpWinImage;  
    std::filesystem::path   mSelectedFilename;
    std::filesystem::path   mFilenameToLoad;

    tImageFilenames         mImagesInFolder;
    std::filesystem::path   mScannedFolder;

    tImageCache             mImageCache;

    std::list<std::string>  mAcceptedExtensions = { "jpg", "jpeg", "png", "gif", "tga", "bmp", "psd", "gif", "hdr", "pic", "pnm" };
    std::atomic<uint32_t>   mAtomicIndex;   // incrementing index for unloading oldest

};
