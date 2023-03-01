#pragma once

#include "ZTypes.h"
#include "ZChess.h"
#include "ZWin.h"
#include "ZFont.h"
#include "ZFormattedTextWin.h"
#include <list>


class ChessPiece
{
public:
    ChessPiece()
    {
    }

    ~ChessPiece()
    {
    }


    bool    GenerateImageFromSymbolicFont(char c, int64_t nSize, ZDynamicFont* pFont, bool bOutline);


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



class ZChoosePGNWin : public ZWin
{
public:
    ZChoosePGNWin();

    bool    Init();
    bool    Paint();
    bool    HandleMessage(const ZMessage& message);

    bool    ListGamesFromPGN(std::string& sFilename, std::string& sPGNFile);


private:
    ZFormattedTextWin* mpGamesList;

    std::string     msCaption;
    uint32_t        mFillColor;
    ZFontParams     mFont;
};


class ZPGNWin : public ZWin
{
public:
    ZPGNWin();

    bool    Init();
    bool    Paint();
    bool    HandleMessage(const ZMessage& message);

    bool    FromPGN(ZChessPGN& pgn);
    bool    Clear();

    bool    SetHalfMove(int64_t nHalfMove);

private:

    void    UpdateView();

    ZFormattedTextWin* mpGameTagsWin;
    ZFormattedTextWin* mpMovesWin;

    uint32_t    mFillColor;
    int64_t  mCurrentHalfMoveNumber;
    ZFontParams mFont;
    ZFontParams mBoldFont;
    ZChessPGN mPGN;
};


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


    bool    Process();
    bool    Paint();
    virtual bool	HandleMessage(const ZMessage& message);

    bool    OnChar(char key);

   
private:
    void    DrawBoard();
    void    DrawPalette();
    void    UpdateSize();

    void    UpdateWatchPanel();

    void    ShowPromotingWin(const ZPoint& grid);

    ZPoint  ScreenToGrid(int64_t x, int64_t y);
    ZPoint  ScreenToSquareOffset(int64_t x, int64_t y);

    char    ScreenToPalettePiece(int64_t x, int64_t y);

    void    LoadRandomPosition();
    bool    LoadPGN(std::string sFilename);
    bool    FromPGN(class ZChessPGN& pgn);



    ZRect   SquareArea(const ZPoint& grid);
    uint32_t SquareColor(const ZPoint& grid);

    ZDynamicFont*   mpSymbolicFont;


    int64_t         mnPieceHeight;
    bool            mbEditMode;
    bool            mbViewWhiteOnBottom;
    bool            mbShowAttackCount;

    int64_t         mnPalettePieceHeight;

    uint32_t        mLightSquareCol;
    uint32_t        mDarkSquareCol;

    char mPalettePieces[12] = { 'q', 'k', 'r', 'n', 'b', 'p', 'P', 'B', 'N', 'R', 'K', 'Q' };


    ZRect mrBoardArea;

    ZRect mrPaletteArea;


    ChessPiece mPieceData[128]; // keyed by ascii chars for pieces, 'r', 'Q', 'P', etc.
    ChessBoard mBoard;

    tZBufferPtr mpDraggingPiece;
    ZRect       mrDraggingPiece;
    char        mDraggingPiece;
    ZPoint      mDraggingSourceGrid;

    ZPiecePromotionWin* mpPiecePromotionWin;
    ZPGNWin* mpPGNWin;
    ZChoosePGNWin* mpChoosePGNWin;

    std::vector<ChessBoard>   mHistory;   // boards for each move in pgn history, etc.



    // watch panel
    std::string msDebugStatus;
    std::string msFEN;


};
