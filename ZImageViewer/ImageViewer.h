#pragma once

#include "ZTypes.h"
#include <filesystem>
#include "ZWin.h"
#include <future>
#include <limits>


class ZWinImage;
class ZWinControlPanel;


typedef std::list<std::filesystem::path>        tImageFilenames;
typedef std::shared_future< tZBufferPtr >       tImageFuture;
typedef std::pair< std::filesystem::path, tImageFuture>   tNamedImagePair;
typedef std::list< tNamedImagePair >            tImageCache;

class ImageViewer : public ZWin
{
    enum eLastAction : uint32_t
    {
        kNone           = 0,
        kNextImage      = 1,
        kPrevImage      = 2,
        kPageForward    = 3,
        kPageBack       = 4,
        kBeginning      = 5,
        kEnd            = 6
    };


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
   bool     OnParentAreaChange();
   void     SetArea(const ZRect& newArea);

   std::atomic<int64_t>     mMaxMemoryUsage;
   std::atomic<int64_t>     mMaxCacheReadAhead;

protected:

    void                    Clear();
    void                    ResetControlPanel();

    bool                    ScanForImagesInFolder(const std::filesystem::path& folder);
    bool                    InCache(const std::filesystem::path& imagePath);
    bool                    ImagePreloading();
    tImageFuture*           GetCached(const std::filesystem::path& imagePath);
    bool                    AddToCache(std::filesystem::path imagePath);
    bool                    RemoveFromCache(std::filesystem::path imagePath);
    int64_t                 CurMemoryUsage();       // only returns the in-memory bytes of buffers that have finished loading
    int64_t                 ImageIndexInFolder(std::filesystem::path imagePath);

    static tZBufferPtr      LoadImageProc(std::filesystem::path& imagePath, ImageViewer* pThis);
    void                    UpdateCaptions();

    bool                    InDeletionList(std::filesystem::path& imagePath);
    void                    ToggleDeletionList(std::filesystem::path& imagePath);

    void                    DeleteConfimed();
    void                    DeleteFile(std::filesystem::path& f);
    void                    HandleQuitCommand();
    void                    HandleMoveCommand();
    void                    HandleDeleteCommand();
    void                    HandleNavigateToParentFolder();

    void                    MoveSelectedFile(std::filesystem::path& newPath);

    bool                    Preload();
    void                    SetFirstImage();
    void                    SetLastImage();
    void                    SetNextImage();
    void                    SetPrevImage();

    ZWinControlPanel*       mpPanel;

    ZWinImage*              mpWinImage;  
    std::filesystem::path   mSelectedFilename;
    std::filesystem::path   mFilenameToLoad;

    tImageFilenames         mImagesInFolder;
    std::filesystem::path   mCurrentFolder;

    std::filesystem::path   mMoveToFolder;

    tImageCache             mImageCache;
    std::recursive_mutex    mImageCacheMutex;

    eLastAction             mLastAction;
    //std::filesystem::path   mLastPreload;

    std::list<std::string>  mAcceptedExtensions = { "jpg", "jpeg", "png", "gif", "tga", "bmp", "psd", "gif", "hdr", "pic", "pnm" };
    std::atomic<uint32_t>   mAtomicIndex;   // incrementing index for unloading oldest

    uint32_t                mToggleUIHotkey;

    // image manipulation
    tImageFilenames         mDeletionList;
    tZFontPtr               mpSymbolicFont;
};
