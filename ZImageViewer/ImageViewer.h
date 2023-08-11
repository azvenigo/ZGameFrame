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


const std::string ksToBeDeleted("ZZ_TO_BE_DELETED_ZZ");
const std::string ksFavorites("favorites");

class ViewingIndex
{
public:
    ViewingIndex(int64_t _abs = -1, int64_t _del = -1, int64_t _fav = -1)
    {
        absoluteIndex = _abs;
        delIndex = _del;
        favIndex = _fav;
    };

    int64_t absoluteIndex;
    int64_t delIndex;
    int64_t favIndex;
};


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

    void Unload()
    {
        pImage = nullptr;
        if (mEXIF.ImageWidth > 0 && mEXIF.ImageHeight > 0)
            mState = ImageEntry::kExifReady;
        else
            mState = ImageEntry::kInit;
    }

    bool ReadyToLoad() { return mState == kExifReady || mState == kNoExifAvailable; }
    bool ToBeDeleted(); // true if image is in the ZZ_to_be_deleted subfolder
    bool IsFavorite();  // true if image is in the "favorites" subfolder


    std::filesystem::path   filename;
    tZBufferPtr             pImage;
    easyexif::EXIFInfo      mEXIF;

    eImageEntryState        mState;
};

typedef std::list<std::filesystem::path>    tImageFilenames;
typedef std::shared_ptr<ImageEntry>         tImageEntryPtr;
typedef std::vector< tImageEntryPtr >       tImageEntryArray;

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

   bool	    OnKeyUp(uint32_t key);
   bool	    OnKeyDown(uint32_t key);
   bool     OnMouseWheel(int64_t x, int64_t y, int64_t nDelta);
   bool	    HandleMessage(const ZMessage& message);

   bool     AcceptedExtension(std::string sExt);
   bool     ShowOpenImageDialog();
   bool     OnParentAreaChange();

   std::atomic<int64_t>     mMaxMemoryUsage;
   std::atomic<int64_t>     mMaxCacheReadAhead;

protected:

    void                    Clear();
    void                    UpdateControlPanel();

    bool                    ScanForImagesInFolder(std::filesystem::path folder);

    tZBufferPtr             GetCurImage(); // null if no image or not loaded

    int64_t                 CurMemoryUsage();       // only returns the in-memory bytes of buffers that have finished loading
    int64_t                 GetLoadsInProgress();

    int64_t                 IndexInCurMode();
    int64_t                 CountInCurMode();

    ViewingIndex            IndexFromPath(const std::filesystem::path& imagePath);
    std::filesystem::path   ToBeDeletedPath();
    std::filesystem::path   FavoritesPath();

    void                    LimitIndex();
    bool                    ValidIndex(const ViewingIndex& vi);
    tImageEntryPtr          EntryFromIndex(const ViewingIndex& vi);

//    tImageFilenames         GetImagesFlaggedToBeDeleted();

    bool                    KickCaching();
    bool                    FreeCacheMemory();
    tImageEntryPtr          NextImageToCache();



    static void             LoadImageProc(std::filesystem::path& imagePath, std::shared_ptr<ImageEntry> pEntry);
    static void             LoadExifProc(std::filesystem::path& imagePath, std::shared_ptr<ImageEntry> pEntry);

    void                    FlushLoads();

    void                    UpdateCaptions();
    void                    UpdateFilteredView(eFilterState state);

    void                    UpdateUI();
    void                    ToggleToBeDeleted();
    void                    ToggleFavorite();

    void                    DeleteConfimed();
    void                    DeleteFile(std::filesystem::path& f);
    void                    HandleQuitCommand();
    void                    HandleMoveCommand();
    void                    HandleCopyCommand();
    void                    HandleNavigateToParentFolder();

    bool                    MoveImage(std::filesystem::path oldPath, std::filesystem::path newPath);
    bool                    CopyImage(std::filesystem::path curPath, std::filesystem::path newPath);

    bool                    RemoveImageArrayEntry(const ViewingIndex& vi);



    void                    SetFirstImage();
    void                    SetLastImage();
    void                    SetNextImage();
    void                    SetPrevImage();

    //bool                    FindNextImageMatchingFilter(ViewingIndex& vi);
    //bool                    FindPrevImageMatchingFilter(ViewingIndex& vi);
    bool                    FindImageMatchingCurFilter(ViewingIndex& vi, int64_t offset);


    int64_t                 CountImagesMatchingFilter(eFilterState state);

    bool                    ImageMatchesCurFilter(const ViewingIndex& vi);

    void                    ShowHelpDialog();

    ZWinControlPanel*       mpPanel;
    std::recursive_mutex    mPanelMutex;

    ZWinImage*              mpWinImage;  


#ifdef _DEBUG
    std::string             LoadStateString();
#endif

    std::filesystem::path   mCurrentFolder;
    std::filesystem::path   mMoveToFolder;
    std::filesystem::path   mCopyToFolder;

    std::filesystem::path   mUndoFrom;  // previous move source
    std::filesystem::path   mUndoTo;    // previous move dest


    ThreadPool*             mpPool;              
    tImageEntryArray        mImageArray;
    tImageEntryArray        mFavImageArray;
    tImageEntryArray        mToBeDeletedImageArray;
    std::recursive_mutex    mImageArrayMutex;



//    int64_t                 mnViewingIndex;
    ViewingIndex            mViewingIndex;
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
    bool                    mbShowUI;
    eFilterState            mFilterState;

   
};


