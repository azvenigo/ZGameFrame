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

typedef std::list<std::filesystem::path>    tImageFilenames;

const int kLeft = 0;
const int kRight = 1;

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
    };

    int32_t     ReadEntry(const uint8_t* pData);        // fills out entry and returns number of bytes processed
    int32_t     WriteEntry(uint8_t* pDest);             // writes to buffer and 

    std::string filename;  // just the filename, no path
    int64_t     filesize;  // filename + nFileSize will be considered a unique image

    int32_t     contests;  // number of times the image has been shown 
    int32_t     wins;      // number of times the image has been selected
    int32_t     elo;        // rating
};

typedef std::list<ImageMetaEntry> tImageMetaList;
typedef std::map<uint16_t, tImageMetaList> tImageSizeToMeta;  // image metadata will be stored in lists keyed by size mod 16k, so maximum 16k maps. Should be reasonably fast to search through

typedef std::list<std::filesystem::path>    tImageFilenames;

class ImageMeta
{
public:
    ImageMetaEntry& Entry(const std::string& filename, int64_t size);

    bool LoadAll();
    bool Save();        // each list will be stored in a separate file, filename being the size mod 16k
    bool SaveBucket(int64_t size);

    tImageSizeToMeta    mSizeToMetaLists;
    std::filesystem::path basePath;

protected:
    bool Load(const std::filesystem::path& imagelist);
    std::filesystem::path   BucketFilename(int64_t size);
};


class ImageContest : public ZWin
{
public:
    ImageContest();
    ~ImageContest();
   
   bool		Init();
   bool		Paint();


   bool	    OnKeyUp(uint32_t key);
   bool	    OnKeyDown(uint32_t key);
   bool	    HandleMessage(const ZMessage& message);
   bool     OnMouseDownL(int64_t x, int64_t y);

   bool     AcceptedExtension(std::string sExt);
   bool     ShowOpenFolderDialog();
   bool     OnParentAreaChange();

   bool     ScanFolder(std::filesystem::path folder);

   bool     SelectWinner(int leftOrRight);
   void     ShowWinnersDialog();
   void     ResetContest();


protected:

    void                    UpdateControlPanel();

    bool                    PickRandomPair();


    void                    UpdateCaptions();

    void                    UpdateUI();

    void                    HandleQuitCommand();

    void                    ShowHelpDialog();

    ZWinControlPanel*       mpPanel;
    std::recursive_mutex    mPanelMutex;

    ZWinImage*              mpWinImage[2];  // left and right
    ImageMetaEntry*         mImageMeta[2];  // left and right

    std::filesystem::path   mCurrentFolder;

    tImageFilenames         mImagesInCurrentFolder;

    std::list<std::string>  mAcceptedExtensions = { "jpg", "jpeg", "png", "gif", "tga", "bmp", "psd", "gif", "hdr", "pic", "pnm", "svg" };

    uint32_t                mToggleUIHotkey;

    bool                    mbShowUI;
    ImageMeta               mMeta;
};


