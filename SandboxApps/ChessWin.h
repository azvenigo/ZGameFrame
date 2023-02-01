#pragma once

#include "ZStdTypes.h"
#include "ZWin.h"
#include "ZFont.h"
#include <list>

/////////////////////////////////////////////////////////////////////////
// 
struct sMove
{
    sMove(ZPoint m, bool capture = false)
    {
        mGrid = m;
        bCapture = capture;
    }

    sMove(int64_t x, int64_t y, bool capture = false)
    {
        mGrid.Set(x, y);
        bCapture = capture;
    }

    ZPoint mGrid;
    bool bCapture;
};


class ChessBoard
{
public:
    ChessBoard() { ResetBoard(); }
    void    ResetBoard();
    void    ClearBoard();
    //    void    RotateBoard();

    char    Piece(const ZPoint& grid);
    bool    Empty(const ZPoint& grid);
    bool    Empty(int64_t x, int64_t y) { return Empty(ZPoint(x, y)); }
    bool    ValidCoord(const ZPoint& l) { return (l.mX >= 0 && l.mX < 8 && l.mY >= 0 && l.mY < 8); }

    bool    MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst);
    bool    SetPiece(const ZPoint& gridDst, char c);

    bool    IsWhite(char c) { return  (c == 'q' || c == 'k' || c == 'r' || c == 'n' || c == 'b' || c == 'p'); }
    bool    IsBlack(char c) { return  (c == 'Q' || c == 'K' || c == 'R' || c == 'N' || c == 'B' || c == 'P'); }

    bool    LegalMove(const ZPoint& src, const ZPoint& dst);
    bool    SameSide(char c, const ZPoint& grid); // true if the grid coordinate has a piece on the same side (black or white)
    bool    Opponent(char c, const ZPoint& grid); // true if an opponent is on the grid location
    bool    GetMoves(char c, const ZPoint& src, std::list<sMove>& moves);

    void    IsOneOfMoves(const ZPoint& src, const std::list<sMove>& moves, bool& bIncluded, bool& bCapture);


    char     mBoard[8][8];
};

class ChessPiece
{
public:

    bool    GenerateImageFromSymbolicFont(char c, int64_t nSize, ZDynamicFont* pFont, bool bOutline);


    tZBufferPtr mpImage;
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

    ZPoint  ScreenToGrid(int64_t x, int64_t y);
    ZPoint  ScreenToSquareOffset(int64_t x, int64_t y);

    char    ScreenToPalettePiece(int64_t x, int64_t y);

    ZRect   SquareArea(const ZPoint& grid);
    uint32_t SquareColor(const ZPoint& grid);

    ZDynamicFont*    mpSymbolicFont;

    int64_t          mnPieceHeight;
    bool             mbOutline;
    bool            mbEditMode;
    bool            mbCopyKeyEnabled;
    bool            mbViewWhiteOnBottom;

    int64_t         mnPalettePieceHeight;

    uint32_t        mLightSquareCol;
    uint32_t        mDarkSquareCol;

    char mPalettePieces[12] = { 'Q', 'K', 'R', 'N', 'B', 'P', 'p', 'b', 'n', 'r', 'k', 'q' };


    ZRect mrBoardArea;

    ZRect mrPaletteArea;


    ChessPiece mPieceData[128]; // keyed by ascii chars for pieces, 'r', 'Q', 'P', etc.
    ChessBoard mBoard;

    tZBufferPtr mpDraggingPiece;
    ZRect       mrDraggingPiece;
    char        mDraggingPiece;
    ZPoint      mDraggingSourceGrid;

};
