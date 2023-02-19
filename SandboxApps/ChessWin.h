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

typedef std::list<sMove> tMoveList;


class ChessBoard
{
public:
    enum eCastlingFlags : uint8_t
    {
        kNone           = 0,
        kWhiteKingSide  = 1,
        kWhiteQueenSide = 2,
        kBlackKingSide  = 4,
        kBlackQueenSide = 8,
        kAll = kWhiteKingSide|kWhiteQueenSide|kBlackKingSide|kBlackQueenSide
    };

    ChessBoard() { ResetBoard(); }
    void            ResetBoard();
    void            ClearBoard();

    char            Piece(const ZPoint& grid);
    bool            Empty(const ZPoint& grid);
    bool            Empty(int64_t x, int64_t y) { return Empty(ZPoint(x, y)); }
    static bool     ValidCoord(const ZPoint& l) { return (l.mX >= 0 && l.mX < 8 && l.mY >= 0 && l.mY < 8); }

    bool            MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst, bool bGameMove = true); // if bGameMove is false, this is simply manipulating the board outside of a game
    bool            SetPiece(const ZPoint& gridDst, char c);

    bool            IsBlack(char c) { return  (c == 'q' || c == 'k' || c == 'r' || c == 'n' || c == 'b' || c == 'p'); }
    bool            IsWhite(char c) { return  (c == 'Q' || c == 'K' || c == 'R' || c == 'N' || c == 'B' || c == 'P'); }

    bool            LegalMove(const ZPoint& src, const ZPoint& dst, bool& bCapture);
    bool            SameSide(char c, const ZPoint& grid); // true if the grid coordinate has a piece on the same side (black or white)
    bool            Opponent(char c, const ZPoint& grid); // true if an opponent is on the grid location
    bool            GetMoves(char c, const ZPoint& src, tMoveList& moves, tMoveList& attackSquares);

    bool            IsPromotingMove(const ZPoint& src, const ZPoint& dst);

    void            IsOneOfMoves(const ZPoint& src, const tMoveList& moves, bool& bIncluded, bool& bCapture);
    uint8_t         UnderAttack(bool bByWhite, const ZPoint& grid); // returns number of pieces attacking a square
    bool            IsKingInCheck(bool bWhite, const ZPoint& atLocation);   // returns true if the king is (or would be) in check at a location
    bool            IsKingInCheck(bool bWhite); // is king currently in check at his current position

    bool            IsCheckmate() { return mbCheckMate; }

    bool            WhitesTurn() { return mbWhitesTurn; }
    void            SetWhitesTurn(bool bWhitesTurn) { mbWhitesTurn = bWhitesTurn; }
    ZPoint          GetEnPassantSquare() { return mEnPassantSquare; }
    uint32_t        GetHalfMovesSinceLastCaptureOrPawnAdvance() { return mHalfMovesSinceLastCaptureOrPawnAdvance; }
    uint32_t        GetMoveNumber() { return mFullMoveNumber; }
    uint32_t        GetCastlingFlags() { return mCastlingFlags; }



    std::string     ToPosition(const ZPoint& grid);
    ZPoint          FromPosition(std::string sPosition);

    std::string     ToFEN();   // converts board position to FEN format
    bool            FromFEN(const std::string& sFEN);

protected:
    void            ComputeSquaresUnderAttack();
    void            ComputeCheckAndMate();

    char    mBoard[8][8];
    bool    mbWhitesTurn;
    bool    mbCheck;
    bool    mbCheckMate;
    uint8_t mCastlingFlags;
    ZPoint  mEnPassantSquare;
    uint32_t mHalfMovesSinceLastCaptureOrPawnAdvance;
    uint32_t mFullMoveNumber;

    uint8_t mSquaresUnderAttackByWhite[8][8];
    uint8_t mSquaresUnderAttackByBlack[8][8];

};

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
    ZPoint      mGrid;
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

    // watch panel
    std::string msDebugStatus;
    std::string msFEN;


};
