#pragma once

#include "ZTypes.h"
#include <filesystem>
#include "ZWin.h"
#include <future>
#include <limits>
#include "helpers/ThreadPool.h"


class ZWinImage;
class ZWinCheck;
class ZWinControlPanel;


const std::string ksToBeDeleted("ZZ_to_be_deleted");
const std::string ksFavorites("favorites");

class ImageEntry
{
public:
    enum eImageEntryState : uint32_t
    {
        kInit = 0,
        kLoadingExif = 1,
        kExifReady = 2,
        kNoExifAvailable = 3,
        kLoadInProgress = 4,
        kLoaded = 5
    };

    ImageEntry(std::filesystem::path _filename) 
    { 
        filename = _filename; 

        mEXIF.clear();

        mState = kInit;
    }

    bool ReadyToLoad() { return mState == kExifReady || mState == kNoExifAvailable; }
    bool ToBeDeleted(); // true if image is in the ZZ_to_be_deleted subfolder
    bool IsFavorite();  // true if image is in the "favorites" subfolder


    std::filesystem::path   filename;
    tZBufferPtr             pImage;
    easyexif::EXIFInfo      mEXIF;

    eImageEntryState        mState;
};

typedef std::list<std::filesystem::path>        tImageFilenames;
typedef std::vector< ImageEntry > tImageEntryArray;

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

    enum eCachingState : uint32_t
    {
        kReadingAhead = 1,
        kWaiting      = 2
    };

    enum eFilterState : uint32_t
    {
        kAll = 0,
        kFavs = 1,
        kToBeDeleted = 2
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

    bool                    ScanForImagesInFolder(std::filesystem::path folder);

    tZBufferPtr             GetCurImage(); // null if no image or not loaded

    int64_t                 CurMemoryUsage();       // only returns the in-memory bytes of buffers that have finished loading
    int64_t                 GetLoadsInProgress();

    int64_t                 IndexFromPath(std::filesystem::path imagePath);
    bool                    ValidIndex(int64_t index);
    std::filesystem::path   PathFromIndex(int64_t index);

    tImageFilenames         GetImagesFlaggedToBeDeleted();

    bool                    KickCaching();
    bool                    FreeCacheMemory();
    int64_t                 NextImageToCache();



    static void             LoadImageProc(std::filesystem::path& imagePath, ImageEntry* pEntry);
    static void             LoadExifProc(std::filesystem::path& imagePath, ImageEntry* pEntry);

    void                    FlushLoads();

    void                    UpdateCaptions();
    void                    UpdateFilteredView(eFilterState state);

    void                    ToggleUI();
    void                    ToggleToBeDeleted();
    void                    ToggleFavorite();

    void                    DeleteConfimed();
    void                    DeleteFile(std::filesystem::path& f);
    void                    HandleQuitCommand();
    void                    HandleMoveCommand();
    void                    HandleDeleteCommand();
    void                    HandleNavigateToParentFolder();

    void                    MoveSelectedFile(std::filesystem::path& newPath);

    bool                    RemoveImageArrayEntry(int64_t nIndex);



    void                    SetFirstImage();
    void                    SetLastImage();
    void                    SetNextImage();
    void                    SetPrevImage();

    bool                    FindNextImageMatchingFilter(int64_t &nIndex);
    bool                    FindPrevImageMatchingFilter(int64_t &nIndex);
    int64_t                 CountImagesMatchingFilter(eFilterState state);

    bool                    ImageMatchesCurFilter(int64_t nIndex);

    ZWinControlPanel*       mpPanel;

    ZWinImage*              mpWinImage;  


#ifdef _DEBUG
    std::string             LoadStateString();
#endif

    std::filesystem::path   mCurrentFolder;
    std::filesystem::path   mMoveToFolder;

    ThreadPool*             mpPool;              
    tImageEntryArray        mImageArray;
    std::recursive_mutex    mImageArrayMutex;


    int64_t                 mnViewingIndex;
    eLastAction             mLastAction;
    eCachingState           mCachingState;

    std::list<std::string>  mAcceptedExtensions = { "jpg", "jpeg", "png", "gif", "tga", "bmp", "psd", "gif", "hdr", "pic", "pnm" };

    uint32_t                mToggleUIHotkey;
    tZFontPtr               mpSymbolicFont;
    tZFontPtr               mpFavoritesFont;

    ZWinCheck*              mpAllFilterButton;
    ZWinCheck*              mpFavsFilterButton;
    ZWinCheck*              mpDelFilterButton;

    bool                    mbSubsample;
    eFilterState            mFilterState;
};


