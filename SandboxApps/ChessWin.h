#pragma once

#include "ZTypes.h"
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

    const bool kBlack = false;
    const bool kWhite = true;


    // Named squares by position
    const ZPoint kA1 = { 0, 7 }; const ZPoint kA2 = { 0, 6 }; const ZPoint kA3 = { 0, 5 }; const ZPoint kA4 = { 0, 4 }; const ZPoint kA5 = { 0, 3 }; const ZPoint kA6 = { 0, 2 }; const ZPoint kA7 = { 0, 1 }; const ZPoint kA8 = { 0, 0 };
    const ZPoint kB1 = { 1, 7 }; const ZPoint kB2 = { 1, 6 }; const ZPoint kB3 = { 1, 5 }; const ZPoint kB4 = { 1, 4 }; const ZPoint kB5 = { 1, 3 }; const ZPoint kB6 = { 1, 2 }; const ZPoint kB7 = { 1, 1 }; const ZPoint kB8 = { 1, 0 };
    const ZPoint kC1 = { 2, 7 }; const ZPoint kC2 = { 2, 6 }; const ZPoint kC3 = { 2, 5 }; const ZPoint kC4 = { 2, 4 }; const ZPoint kC5 = { 2, 3 }; const ZPoint kC6 = { 2, 2 }; const ZPoint kC7 = { 2, 1 }; const ZPoint kC8 = { 2, 0 };
    const ZPoint kD1 = { 3, 7 }; const ZPoint kD2 = { 3, 6 }; const ZPoint kD3 = { 3, 5 }; const ZPoint kD4 = { 3, 4 }; const ZPoint kD5 = { 3, 3 }; const ZPoint kD6 = { 3, 2 }; const ZPoint kD7 = { 3, 1 }; const ZPoint kD8 = { 3, 0 };
    const ZPoint kE1 = { 4, 7 }; const ZPoint kE2 = { 4, 6 }; const ZPoint kE3 = { 4, 5 }; const ZPoint kE4 = { 4, 4 }; const ZPoint kE5 = { 4, 3 }; const ZPoint kE6 = { 4, 2 }; const ZPoint kE7 = { 4, 1 }; const ZPoint kE8 = { 4, 0 };
    const ZPoint kF1 = { 5, 7 }; const ZPoint kF2 = { 5, 6 }; const ZPoint kF3 = { 5, 5 }; const ZPoint kF4 = { 5, 4 }; const ZPoint kF5 = { 5, 3 }; const ZPoint kF6 = { 5, 2 }; const ZPoint kF7 = { 5, 1 }; const ZPoint kF8 = { 5, 0 };
    const ZPoint kG1 = { 6, 7 }; const ZPoint kG2 = { 6, 6 }; const ZPoint kG3 = { 6, 5 }; const ZPoint kG4 = { 6, 4 }; const ZPoint kG5 = { 6, 3 }; const ZPoint kG6 = { 6, 2 }; const ZPoint kG7 = { 6, 1 }; const ZPoint kG8 = { 6, 0 };
    const ZPoint kH1 = { 7, 7 }; const ZPoint kH2 = { 7, 6 }; const ZPoint kH3 = { 7, 5 }; const ZPoint kH4 = { 7, 4 }; const ZPoint kH5 = { 7, 3 }; const ZPoint kH6 = { 7, 2 }; const ZPoint kH7 = { 7, 1 }; const ZPoint kH8 = { 7, 0 };

    const ZPoint kWhiteKingHome = { kE1 };
    const ZPoint kWhiteQueenHome = { kD1 };

    const ZPoint kBlackQueenHome = { kD8 };
    const ZPoint kBlackKingHome = { kE8 };



    ChessBoard() { ResetBoard(); }
    void            ResetBoard();
    void            ClearBoard();

    char            Piece(const ZPoint& grid);
    bool            Empty(const ZPoint& grid);
    bool            Empty(int64_t x, int64_t y) { return Empty(ZPoint(x, y)); }
    static bool     ValidCoord(const ZPoint& l) { return (l.x >= 0 && l.x < 8 && l.y >= 0 && l.y < 8); }

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
