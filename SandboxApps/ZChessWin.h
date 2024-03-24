#pragma once

#include "ZTypes.h"
#include "ZChess.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZWinFormattedText.h"
#include "ZWinText.H"
#include <list>

class ZWinControlPanel;

class ChessPiece
{
public:
    ChessPiece()
    {
    }

    ~ChessPiece()
    {
    }


    bool    GenerateImageFromSymbolicFont(char c, int64_t nSize, ZDynamicFont* pFont, bool bOutline, uint32_t whiteCol = 0xffffffff, uint32_t blackCol = 0xff000000);


    tZBufferPtr mpImage;
};

class ZPiecePromotionWin : public ZWin
{
public:
    ZPiecePromotionWin() : mpPieceData(nullptr) {}

    bool    Init(ChessPiece* pPieceData, ZPoint grid);
    bool    OnMouseDownL(int64_t x, int64_t y);

    bool    Paint();

private:
    char        mPromotionPieces[8] = { 'Q', 'R', 'N', 'B', 'q', 'r', 'n', 'b' };

    ChessPiece* mpPieceData; // keyed by ascii chars for pieces, 'r', 'Q', 'P', etc.
    ZPoint      mDest;
};



typedef std::pair<std::string, std::string> tPGNEntry;
typedef std::list< tPGNEntry > tPGNEntries;

class ZChoosePGNWin : public ZWin
{
public:
    ZChoosePGNWin();

    bool    Init();
    bool    Paint();
    bool    Process();
    bool    HandleMessage(const ZMessage& message);

    bool    ListGamesFromPGN(std::string& sFilename, std::string& sPGNFile);

    void    SelectEntry(size_t index);


private:

    void    RefreshList();

    ZWinFormattedDoc* mpGamesList;

    tPGNEntries     mPGNEntries;
    std::string     msFilter;
    std::string     msLastFiltered; // when different, refreshes the list

    std::string     msCaption;
    uint32_t        mFillColor;
    ZFontParams     mFont;
};


class ZPGNWin : public ZWin
{
public:
    ZPGNWin();

    bool        Init();
    bool        Paint();
    bool        HandleMessage(const ZMessage& message);

    bool        FromPGN(ZChessPGN& pgn);
    std::string GetPGN();
    bool        AddAction(const std::string& sAction, size_t nHalfMoveNumber);
    bool        Clear();


    bool        SetHalfMove(int64_t nHalfMove);

private:

    void        UpdateView();

    ZWinFormattedDoc*  mpGameTagsWin;
    ZWinFormattedDoc*  mpMovesWin;

    uint32_t            mFillColor;
    int64_t             mCurrentHalfMoveNumber;
    ZFontParams         mMoveFont;
    ZFontParams         mTagsFont;
    ZFontParams         mBoldFont;
    ZChessPGN           mPGN;
};


class ZWinBtn;

class ZChessWin : public ZWin
{
public:
    ZChessWin();
   
    bool    Init();
    bool    Shutdown();
    bool    OnMouseDownL(int64_t x, int64_t y);
    bool    OnMouseUpL(int64_t x, int64_t y);
    bool    OnMouseMove(int64_t x, int64_t y);
    bool    OnMouseDownR(int64_t x, int64_t y);

    bool    OnKeyDown(uint32_t key);
    bool    OnKeyUp(uint32_t key);


    void    SetDemoMode(bool bDemo) { mbDemoMode = bDemo; } // load random game and play through it

    bool    Process();
    bool    Paint();
    virtual bool	HandleMessage(const ZMessage& message);

    bool    OnChar(char key);


    void    UpdateStatus(const std::string& sText, uint32_t col = 0xffffffff);
   
private:
    void    DrawBoard();
    void    DrawPalette();
    void    UpdateSize();

    void    ShowPromotingWin(const ZPoint& grid);

    ZPoint  ScreenToGrid(int64_t x, int64_t y);
    ZPoint  ScreenToSquareOffset(int64_t x, int64_t y);

    char    ScreenToPalettePiece(int64_t x, int64_t y);

    void    ClearHistory();
    void    LoadRandomGame();
    bool    LoadPGN(std::string sFilename);
    bool    SavePGN(std::string sFilename);
    bool    FromPGN(class ZChessPGN& pgn);



    ZRect   SquareArea(const ZPoint& grid);
    uint32_t SquareColor(const ZPoint& grid);

    ZDynamicFont*   mpSymbolicFont;

    bool            mbDemoMode;
    int64_t         mnDemoModeNextMoveTimeStamp;


    int64_t         mnPieceHeight;
    bool            mbEditMode;
    bool            mbViewWhiteOnBottom;
    bool            mbShowAttackCount;

    int64_t         mnPalettePieceHeight;

    ZGUI::Palette   mPalette;

/*    uint32_t*       mpLightSquareCol;
    uint32_t*       mpDarkSquareCol;
    uint32_t*       mpWhiteCol;
    uint32_t*       mpBlackCol;*/

    char mPalettePieces[12] = { 'q', 'k', 'r', 'n', 'b', 'p', 'P', 'B', 'N', 'R', 'K', 'Q' };


    ZRect mrBoardArea;

    ZRect mrPaletteArea;

    //ZWinLabel*              mpStatusWin;
    ZWinTextEdit*           mpStatusWin;
    std::string             msStatus;


    ChessPiece              mPieceData[128]; // keyed by ascii chars for pieces, 'r', 'Q', 'P', etc.
    ChessBoard              mBoard;

    tZBufferPtr             mpDraggingPiece;
    ZRect                   mrDraggingPiece;
    char                    mDraggingPiece;
    ZPoint                  mDraggingSourceGrid;

    ZPiecePromotionWin*     mpPiecePromotionWin;
    ZPGNWin* mpPGNWin;
    ZChoosePGNWin*          mpChoosePGNWin;
    ZWinControlPanel*       mpEditBoardWin;

    std::vector<ChessBoard> mHistory;   // boards for each move in pgn history, etc.
    std::recursive_mutex    mHistoryMutex;

    int64_t                 mAutoplayMSBetweenMoves;


    // watch panel
    std::string msDebugStatus;
    std::string msFEN;

    class ZAnimator* mpAnimator;
    ZPoint mHiddenSquare;   // square that is not to be drawn because it is animating
};
