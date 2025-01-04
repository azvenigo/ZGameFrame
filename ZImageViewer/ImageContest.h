#pragma once

#include "ZTypes.h"
#include <filesystem>
#include "ZWin.h"
#include <future>
#include <limits>
#include "helpers/ThreadPool.h"
#include "ImageMeta.h"


class ZWinImage;
class ZWinCheck;
class ZWinBtn;
class ZWinControlPanel;
class WinTopWinners;

typedef std::list<std::filesystem::path>    tImageFilenames;

const int kLeft = 0;
const int kRight = 1;

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
    enum State
    {
        kNone = 0,
        kSelectingFromPair = 1,
        kShowingSingle = 2
    };


    void                    UpdateControlPanel();

    bool                    PickRandomPair();


    void                    UpdateCaptions();

    void                    UpdateUI();

    void                    HandleQuitCommand();

    void                    ShowHelpDialog();

    void                    ScanMetadata();

    ZWinControlPanel*       mpPanel;
    std::recursive_mutex    mPanelMutex;

    ZGUI::Style             mSymbolicStyle;

    ZWinImage*              mpWinImage[2];  // left and right
    ImageMetaEntry*         mImageMeta[2];  // left and right

    WinTopWinners*          mpRatedImagesStrip;

    std::filesystem::path   mCurrentFolder;

    tImageFilenames         mImagesInCurrentFolder;

    std::list<std::string>  mAcceptedExtensions = { "jpg", "jpeg", "png", "gif", "tga", "bmp", "psd", "gif", "hdr", "pic", "pnm", "svg" };

    uint32_t                mToggleUIHotkey;

    bool                    mbShowUI;
    tImageMetaList          mCurrentFolderImageMeta;
    ZWinBtn*                mpChooseBtn[2];

    State                   mState;
};


