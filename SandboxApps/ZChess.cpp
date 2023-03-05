#include "ZChess.h"
#include "Resources.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZStringHelpers.h"
#include "helpers/RandHelpers.h"
#include "helpers/LoggingHelpers.h"
#include <filesystem>
#include <fstream>
#include "ZTimer.h"

using namespace std;


char     mResetBoard[8][8] =
{   {'r','n','b','q','k','b','n','r'},
    {'p','p','p','p','p','p','p','p'},
    {0},
    {0},
    {0},
    {0},
    {'P','P','P','P','P','P','P','P'},
    {'R','N','B','Q','K','B','N','R'}
};


bool Contains(const std::string& sText, const std::string& sSubstring)
{
    return sText.find(sSubstring) != string::npos;
}



char ChessBoard::Piece(const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return 0;

    return mBoard[grid.y][grid.x];
}

bool ChessBoard::Empty(const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return false;

    return mBoard[grid.y][grid.x] == 0;
}



bool ChessBoard::MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst, bool bGameMove)
{
    if (!ValidCoord(gridSrc))
    {
        ZDEBUG_OUT("Error: MovePiece src invalid:%d,%d\n", gridSrc.x, gridSrc.y);
        return false;
    }
    if (!ValidCoord(gridDst))
    {
        ZDEBUG_OUT("Error: MovePiece dst invalid:%d,%d\n", gridDst.x, gridDst.y);
        return false;
    }

    if (gridSrc == gridDst)
        return false;


    char c = mBoard[gridSrc.y][gridSrc.x];
    if (c == 0)
    {
        ZDEBUG_OUT("Note: MovePiece src has no piece:%d,%d\n", gridSrc.x, gridSrc.y);
        return false;
    }

    if (bGameMove)
    {
        if (mbWhitesTurn && !IsWhite(c))
        {
            ZWARNING("Error: White's turn.\n");
            return false;
        }

        if (!mbWhitesTurn && IsWhite(c))
        {
            ZWARNING("Error: Black's turn.\n");
            return false;
        }

        if (!LegalMove(gridSrc, gridDst))
        {
            ZWARNING("Error: Illegal move.\n");
            return false;
        }
    }

    bool bIsCapture = Opponent(c, gridDst);


    if (bIsCapture || c == 'p' || c == 'P')
        mHalfMovesSinceLastCaptureOrPawnAdvance = 0;
    else
        mHalfMovesSinceLastCaptureOrPawnAdvance++;

    // castling
    if (c == 'K')
        mCastlingFlags &= ~(kWhiteKingSide | kWhiteQueenSide); // since white king is moving, no more castling

    if (c == 'k')
        mCastlingFlags &= ~(kBlackKingSide | kBlackQueenSide); // since black king is moving, no more castling

    if (c == 'R' && gridSrc == kA1)
        mCastlingFlags &= ~kWhiteQueenSide;     // queenside rook moving, no more castling this side
    if (c == 'R' && gridSrc == kH1)
        mCastlingFlags &= ~kWhiteKingSide;     // queenside rook moving, no more castling this side

    if (c == 'r' && gridSrc == kA8)
        mCastlingFlags &= ~kBlackQueenSide;     // queenside rook moving, no more castling this side
    if (c == 'r' && gridSrc == kH8)
        mCastlingFlags &= ~kBlackKingSide;     // queenside rook moving, no more castling this side

    if (c == 'K' && gridSrc == kWhiteKingHome)   // white castling
    {
        if (gridDst == kC1)
        {
            // move rook
            MovePiece(kA1, kD1, false);
        }
        else if (gridDst == kG1)
        {
            // move rook
            MovePiece(kH1, kF1, false);
        }
    }

    if (c == 'k' && gridSrc == kBlackKingHome)   // white castling
    {
        if (gridDst == kC8)
        {
            // move rook
            MovePiece(kA8, kD8, false);
        }
        else if (gridDst == kG8)
        {
            // move rook
            MovePiece(kH8, kF8, false);
        }
    }



    // En passant capture?
    if (c == 'p' && gridDst == mEnPassantSquare)
        mBoard[mEnPassantSquare.y - 1][mEnPassantSquare.x] = 0;
    if (c == 'P' && gridDst == mEnPassantSquare)
        mBoard[mEnPassantSquare.y + 1][mEnPassantSquare.x] = 0;

    // Creation of en passant square
    mEnPassantSquare.Set(-1, -1);
    // check for pawn moving two spaces for setting en passant
    if (c == 'p' && gridSrc.y == 1 && gridDst.y == 3)
        mEnPassantSquare.Set(gridDst.x, 2);
    if (c == 'P' && gridSrc.y == 6 && gridDst.y == 4)
        mEnPassantSquare.Set(gridDst.x, 5);



    mLastMove.mSrc = gridSrc;
    mLastMove.mDest = gridDst;


    mBoard[gridDst.y][gridDst.x] = c;
    mBoard[gridSrc.y][gridSrc.x] = 0;
    ComputeSquaresUnderAttack();

    if (bGameMove)
    {
        mHalfMoveNumber++;
        if (!mbWhitesTurn)
            mFullMoveNumber++;

        mbWhitesTurn = !mbWhitesTurn;
        ComputeCheckAndMate();
    }

    return true;
}
/*

bool ChessBoard::MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst, bool bGameMove)
{
    if (!ValidCoord(gridSrc))
    {
        ZDEBUG_OUT("Error: MovePiece src invalid:%d,%d\n", gridSrc.x, gridSrc.y);
        return false;
    }
    if (!ValidCoord(gridDst))
    {
        ZDEBUG_OUT("Error: MovePiece dst invalid:%d,%d\n", gridDst.x, gridDst.y);
        return false;
    }

    if (gridSrc == gridDst)
        return false;


    char c = mBoard[gridSrc.y][gridSrc.x];
    if (c == 0)
    {
        ZDEBUG_OUT("Note: MovePiece src has no piece:%d,%d\n", gridSrc.x, gridSrc.y);
        return false;
    }

    if (bGameMove)
    {
        if (mbWhitesTurn && !IsWhite(c))
        {
            ZWARNING("Error: White's turn.\n");
            return false;
        }

        if (!mbWhitesTurn && IsWhite(c))
        {
            ZWARNING("Error: Black's turn.\n");
            return false;
        }

        if (!LegalMove(gridSrc, gridDst))
        {
            ZWARNING("Error: Illegal move.\n");
            return false;
        }

        bool bIsCapture = Opponent(c, gridDst);


        if (bIsCapture || c == 'p' || c == 'P')
            mHalfMovesSinceLastCaptureOrPawnAdvance = 0;
        else
            mHalfMovesSinceLastCaptureOrPawnAdvance++;

        // castling
        if (c == 'K')
            mCastlingFlags &= ~(kWhiteKingSide | kWhiteQueenSide); // since white king is moving, no more castling

        if (c == 'k')
            mCastlingFlags &= ~(kBlackKingSide | kBlackQueenSide); // since black king is moving, no more castling

        if (c == 'R' && gridSrc == kA1)
            mCastlingFlags &= ~kWhiteQueenSide;     // queenside rook moving, no more castling this side
        if (c == 'R' && gridSrc == kH1)
            mCastlingFlags &= ~kWhiteKingSide;     // queenside rook moving, no more castling this side

        if (c == 'r' && gridSrc == kA8)
            mCastlingFlags &= ~kBlackQueenSide;     // queenside rook moving, no more castling this side
        if (c == 'r' && gridSrc == kH8)
            mCastlingFlags &= ~kBlackKingSide;     // queenside rook moving, no more castling this side

        if (c == 'K' && gridSrc == kWhiteKingHome)   // white castling
        {
            if (gridDst == kC1)
            {
                // move rook
                MovePiece(kA1, kD1, false);
            }
            else if (gridDst == kG1)
            {
                // move rook
                MovePiece(kH1, kF1, false);
            }
        }

        if (c == 'k' && gridSrc == kBlackKingHome)   // white castling
        {
            if (gridDst == kC8)
            {
                // move rook
                MovePiece(kA8, kD8, false);
            }
            else if (gridDst == kG8)
            {
                // move rook
                MovePiece(kH8, kF8, false);
            }
        }



        // En passant capture?
        if (c == 'p' && gridDst == mEnPassantSquare)
            mBoard[mEnPassantSquare.y - 1][mEnPassantSquare.x] = 0;
        if (c == 'P' && gridDst == mEnPassantSquare)
            mBoard[mEnPassantSquare.y + 1][mEnPassantSquare.x] = 0;

        // Creation of en passant square
        mEnPassantSquare.Set(-1, -1);
        // check for pawn moving two spaces for setting en passant
        if (c == 'p' && gridSrc.y == 1 && gridDst.y == 3)
            mEnPassantSquare.Set(gridDst.x, 2);
        if (c == 'P' && gridSrc.y == 6 && gridDst.y == 4)
            mEnPassantSquare.Set(gridDst.x, 5);



        mbWhitesTurn = !mbWhitesTurn;

        mLastMove.mSrc = gridSrc;
        mLastMove.mDest = gridDst;

        mHalfMoveNumber++;
        if (mbWhitesTurn)
            mFullMoveNumber++;

        mBoard[gridDst.y][gridDst.x] = c;
        mBoard[gridSrc.y][gridSrc.x] = 0;
        ComputeSquaresUnderAttack();
        ComputeCheckAndMate();
    }
    else
    {
        mBoard[gridDst.y][gridDst.x] = c;
        mBoard[gridSrc.y][gridSrc.x] = 0;
        ComputeSquaresUnderAttack();
        mbCheck = false;
        mbCheckMate = false;
    }


    return true;
}
*/
bool ChessBoard::PromotePiece(const ZPoint& gridSrc, const ZPoint& gridDst, char promotedPiece)
{
    SetPiece(gridSrc, 0);
    SetPiece(gridDst, promotedPiece);
    mLastMove.mSrc = gridSrc;
    mLastMove.mDest = gridDst;
    SetWhitesTurn(!WhitesTurn());

    return true;
}


bool ChessBoard::SetPiece(const ZPoint& gridDst, char c)
{
    if (!ValidCoord(gridDst))
    {
        ZDEBUG_OUT("Error: SetPiece dst invalid:%d,%d\n", gridDst.x, gridDst.y);
        return false;
    }

    mBoard[gridDst.y][gridDst.x] = c;
    ComputeSquaresUnderAttack();
    ComputeCheckAndMate();
    return true;
}


void ChessBoard::ResetBoard()
{
    memcpy(mBoard, mResetBoard, sizeof(mBoard));
    mbWhitesTurn = true;
    mCastlingFlags = eCastlingFlags::kAll;
    mEnPassantSquare.Set(-1, -1);
    mLastMove.mSrc.Set(-1, -1);
    mLastMove.mDest.Set(-1, -1);
    mHalfMovesSinceLastCaptureOrPawnAdvance = 0;
    mFullMoveNumber = 1;
    mHalfMoveNumber = 1;
    mbCheck = false;
    mbCheckMate = false;
    ComputeSquaresUnderAttack();
}

void ChessBoard::ClearBoard()
{
    mLastMove.mSrc.Set(-1, -1);
    mLastMove.mDest.Set(-1, -1);
    mEnPassantSquare.Set(-1, -1);
    memset(mBoard, 0, sizeof(mBoard));
    memset(mSquaresUnderAttackByBlack, 0, sizeof(mSquaresUnderAttackByBlack));
    memset(mSquaresUnderAttackByWhite, 0, sizeof(mSquaresUnderAttackByWhite));
}

bool ChessBoard::SameSide(char c, const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return false;

    bool bCIsWhite = IsWhite(c);
    bool bSquarePieceIsWhite = IsWhite(Piece(grid));

    return (bCIsWhite && bSquarePieceIsWhite) || (!bCIsWhite && !bSquarePieceIsWhite);
}

bool ChessBoard::Opponent(char c, const ZPoint& grid)
{
    // need to check for valid coord first before returning opposite of SameSide or !SameSide would be true for invalid coord
    if (!ValidCoord(grid))
        return false;

    if (c == 0 || Piece(grid) == 0)
        return false;

    return !SameSide(c, grid);
}

bool ChessBoard::IsKingInCheck(bool bWhite, const ZPoint& atLocation)
{
    if (bWhite)
        return mSquaresUnderAttackByBlack[atLocation.y][atLocation.x];
    return mSquaresUnderAttackByWhite[atLocation.y][atLocation.x];
}

bool ChessBoard::IsKingInCheck(bool bWhite)
{
    // Find the king in question
    ZPoint grid;
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            if ((mBoard[y][x] == 'K' && bWhite) || (mBoard[y][x] == 'k' && !bWhite))
            {
                grid.Set(x, y);
                break;
            }
        }
    }

    return IsKingInCheck(bWhite, grid); // Is the king in check currently at his location?
}



bool ChessBoard::LegalMove(const ZPoint& src, const ZPoint& dst)
{
    if (!ValidCoord(src))
        return false;
    if (!ValidCoord(dst))
        return false;

    char s = Piece(src);
    char d = Piece(dst);

    if (s == 0)
        return false;

    tMoveList legalMoves;
    tMoveList attackMoves;
    GetMoves(s, src, legalMoves, attackMoves);

    // test whether the king is in check after the move
    ChessBoard afterMove(*this);    // copy the board
    afterMove.MovePiece(src, dst, false);   
    if (afterMove.IsKingInCheck(mbWhitesTurn))
    {
        ZWARNING("Illegal move. King would be in check");
        return false;
    }


    // white king may not move into check
    if (s == 'K' && IsKingInCheck(true, dst))
        return false;

    // black king may not move into check
    if (s == 'k' && IsKingInCheck(false, dst))
        return false;

    // Castling
    if (s == 'K' && src == kE1 && dst == kG1)
    {
        if (!(mCastlingFlags & kWhiteKingSide))      // White kingside disabled?
            return false;
        if (IsKingInCheck(kWhite))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kWhite, kF1))       // cannot move across check
            return false;
    }
    if (s == 'K' && src == kE1 && dst == kC1)
    {
        if (!(mCastlingFlags & kWhiteQueenSide))      // white queenside disabled?
            return false;
        if (IsKingInCheck(kWhite))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kWhite, kD1) || IsKingInCheck(kWhite, kC1))       // cannot move across check
            return false;
    }


    if (s == 'k' && src == kE8 && dst == kG8)
    {
        if (!(mCastlingFlags & kBlackKingSide))      // black kingside disabled?
            return false;
        if (IsKingInCheck(kBlack))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kBlack, kF8))       // cannot move across check
            return false;
    }
    if (s == 'k' && src == kE8 && dst == kC8)
    {
        if (!(mCastlingFlags & kBlackQueenSide))      // black queenside disabled?
            return false;
        if (IsKingInCheck(kBlack))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kBlack, kD8) || IsKingInCheck(kBlack, kC8))       // cannot move across check
            return false;
    }

    return IsOneOfMoves(dst, legalMoves);
}

bool ChessBoard::IsPromotingMove(const ZPoint& src, const ZPoint& dst)
{
    char s = Piece(src);

    // white pawn on 8th rank?
    if (s == 'P')
        return dst.y == 0;

    // black pawn on 1st rank?
    return (s == 'p' && dst.y == 7);
}


uint8_t ChessBoard::UnderAttack(bool bByWhyte, const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return false;

    if (bByWhyte)
        return mSquaresUnderAttackByWhite[grid.y][grid.x];

    return mSquaresUnderAttackByBlack[grid.y][grid.x];
}



void ChessBoard::ComputeSquaresUnderAttack()
{
    memset(mSquaresUnderAttackByBlack, 0, sizeof(mSquaresUnderAttackByBlack));
    memset(mSquaresUnderAttackByWhite, 0, sizeof(mSquaresUnderAttackByWhite));

    tMoveList moves;
    tMoveList attackSquares;
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            ZPoint grid(x, y);
            char c = Piece(grid);
            if (c)
            {
                if (GetMoves(c, grid, moves, attackSquares))
                {
                    if (IsWhite(c))
                    {
                        for (auto square : attackSquares)
                            mSquaresUnderAttackByWhite[square.mDest.y][square.mDest.x]++;
                    }
                    else
                    {
                        for (auto square : attackSquares)
                            mSquaresUnderAttackByBlack[square.mDest.y][square.mDest.x]++;
                    }
                }
            }
        }
    }
}

void ChessBoard::ComputeCheckAndMate()
{
    int64_t nStart = gTimer.GetUSSinceEpoch();
    mbCheck = false;
    mbCheckMate = false;
    if (IsKingInCheck(mbWhitesTurn))
    {
        mbCheck = true;

        // look for any possible moves
        ZPoint grid;
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                char c = mBoard[y][x];
                if (IsWhite(c) == mbWhitesTurn)
                {
                    tMoveList legalMoves;
                    tMoveList attackMoves;
                    grid.Set(x, y);
                    GetMoves(c, grid, legalMoves, attackMoves);
                    for (auto move : legalMoves)
                    {
                        ChessBoard afterMove(*this);
                        afterMove.MovePiece(grid, move.mDest, false);
                        if (!afterMove.IsKingInCheck(mbWhitesTurn))       // if king is no longer in check after the move, we're done
                        {
                            int64_t nEnd = gTimer.GetUSSinceEpoch();
                            ZDEBUG_OUT("ComputeCheckAndMate time:", nEnd - nStart);
                            return;
                        }
                    }
                }
            }
        }

        mbCheckMate = true;
        if (mbWhitesTurn)
            msResult = "1-0";
        else
            msResult = "0-1";
    }
    int64_t nEnd = gTimer.GetUSSinceEpoch();
    ZDEBUG_OUT("ComputeCheckAndMate time:", nEnd - nStart);
}


void AddMove(tMoveList& moves, const sMove& move)
{
//    ZASSERT(ChessBoard::ValidCoord(move.mGrid));

    if (ChessBoard::ValidCoord(move.mDest))
        moves.push_back(move);
}


bool ChessBoard::GetMoves(char c, const ZPoint& src, tMoveList& moves, tMoveList& attackSquares)
{
    if (c == 0 || !ValidCoord(src))
        return false;

    moves.clear();
    attackSquares.clear();

    if (c == 'P')
    {
        if (src.y == 6 && Empty(src.x, 5) && Empty(src.x, 4))    // pawn hasn't moved
            AddMove(moves, sMove(src, ZPoint(src.x, 4)));             // two up
        if (src.y > 0 && Empty(src.x, src.y-1))
            AddMove(moves, sMove(src, ZPoint(src.x, src.y-1)));     // one up if not on the last row
        if (src.x > 0)
        {
            AddMove(attackSquares, sMove(src, ZPoint(src.x - 1, src.y - 1)));
            if (Opponent(c, ZPoint(src.x - 1, src.y - 1)) || mEnPassantSquare == ZPoint(src.x - 1, src.y - 1))
                AddMove(moves, sMove(src, ZPoint(src.x - 1, src.y - 1)));     // capture left
        }
        if (src.x < 7)
        {
            AddMove(attackSquares, sMove(src, ZPoint(src.x + 1, src.y - 1)));
            if (Opponent(c, ZPoint(src.x + 1, src.y - 1)) || mEnPassantSquare == ZPoint(src.x + 1, src.y - 1))
                AddMove(moves, sMove(src, ZPoint(src.x + 1, src.y - 1)));     // capture right
        }
    }
    if (c == 'p')
    {
        if (src.y == 1 && Empty(src.x, 2) && Empty(src.x, 3))    // pawn hasn't moved
            AddMove(moves, sMove(src, ZPoint(src.x, 3)));            // two down
        if (src.y < 7 && Empty(src.x, src.y + 1))
            AddMove(moves, sMove(src, ZPoint(src.x, src.y + 1)));     // one down if not on the last row
        if (src.x > 0)
        {
            AddMove(attackSquares, sMove(src, ZPoint(src.x - 1, src.y + 1)));
            if (Opponent(c, ZPoint(src.x - 1, src.y + 1)) || mEnPassantSquare == ZPoint(src.x - 1, src.y + 1))
                AddMove(moves, sMove(src, ZPoint(src.x - 1, src.y + 1)));     // capture left
        }
        if (src.x < 7)
        {
            AddMove(attackSquares, sMove(src, ZPoint(src.x + 1, src.y + 1)));
            if (Opponent(c, ZPoint(src.x + 1, src.y + 1)) || mEnPassantSquare == ZPoint(src.x + 1, src.y + 1))
                AddMove(moves, sMove(src, ZPoint(src.x + 1, src.y + 1)));     // capture right
        }
    }
    else if (c == 'r' || c == 'R')
    {
        ZPoint checkGrid(src);
        // run left
        do
        {
            checkGrid.x--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run right
        checkGrid = src;
        do
        {
            checkGrid.x++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));
        } while (ValidCoord(checkGrid));

        // run up
        checkGrid = src;
        do
        {
            checkGrid.y--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run down
        checkGrid = src;
        do
        {
            checkGrid.y++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));
        } while (ValidCoord(checkGrid));
    }
    else if (c == 'b' || c == 'B')
    {
        ZPoint checkGrid(src);
        // run up left
        do
        {
            checkGrid.x--;
            checkGrid.y--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));
        } while (ValidCoord(checkGrid));

        // run up right
        checkGrid = src;
        do
        {
            checkGrid.x++;
            checkGrid.y--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));
        } while (ValidCoord(checkGrid));

        // run down left
        checkGrid = src;
        do
        {
            checkGrid.x--;
            checkGrid.y++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run down right
        checkGrid = src;
        do
        {
            checkGrid.x++;
            checkGrid.y++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));
        } while (ValidCoord(checkGrid));
    }
    else if (c == 'n' || c == 'N')
    {
        ZPoint checkGrid(src);
        // up up left
        checkGrid.y-=2;
        checkGrid.x--;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // up up right
        checkGrid = src;
        checkGrid.y -= 2;
        checkGrid.x++;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));

        // left left up
        checkGrid = src;
        checkGrid.y --;
        checkGrid.x-=2;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));

        // right right up
        checkGrid = src;
        checkGrid.y--;
        checkGrid.x += 2;
        if (Opponent(c, checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        else if (Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));

        // down down right
        checkGrid = src;
        checkGrid.y += 2;
        checkGrid.x++;
        if (Opponent(c, checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        else if (Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // down down left
        checkGrid = src;
        checkGrid.y += 2;
        checkGrid.x--;
        if (Opponent(c, checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        else if (Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // left left down
        checkGrid = src;
        checkGrid.y++;
        checkGrid.x -= 2;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // right right down
        checkGrid = src;
        checkGrid.y++;
        checkGrid.x += 2;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));
    }
    else if (c == 'q' || c == 'Q')
    {
        ZPoint checkGrid(src);
        // run up left
        do
        {
            checkGrid.x--;
            checkGrid.y--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run up right
        checkGrid = src;
        do
        {
            checkGrid.x++;
            checkGrid.y--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run down left
        checkGrid = src;
        do
        {
            checkGrid.x--;
            checkGrid.y++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));


        } while (ValidCoord(checkGrid));

        // run down right
        checkGrid = src;
        do
        {
            checkGrid.x++;
            checkGrid.y++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run left
        checkGrid = src;
        do
        {
            checkGrid.x--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run right
        checkGrid = src;
        do
        {
            checkGrid.x++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run up
        checkGrid = src;
        do
        {
            checkGrid.y--;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));

        // run down
        checkGrid = src;
        do
        {
            checkGrid.y++;
            if (Opponent(c, checkGrid))
            {
                AddMove(moves, sMove(src, checkGrid));
                AddMove(attackSquares, sMove(src, checkGrid));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            AddMove(moves, sMove(src, checkGrid));
            AddMove(attackSquares, sMove(src, checkGrid));

        } while (ValidCoord(checkGrid));
    }
    else if (c == 'k' || c == 'K')
    {
        ZPoint checkGrid(src);
        // up 
        checkGrid.y--;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // down
        checkGrid = src;
        checkGrid.y++;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // left
        checkGrid = src;
        checkGrid.x--;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // right
        checkGrid = src;
        checkGrid.x++;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // left up
        checkGrid = src;
        checkGrid.y--;
        checkGrid.x--;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // right up
        checkGrid = src;
        checkGrid.y--;
        checkGrid.x++;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // left down
        checkGrid = src;
        checkGrid.y++;
        checkGrid.x--;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));


        // right down
        checkGrid = src;
        checkGrid.y++;
        checkGrid.x++;
        if (Opponent(c, checkGrid) || Empty(checkGrid))
            AddMove(moves, sMove(src, checkGrid));
        AddMove(attackSquares, sMove(src, checkGrid));

        // White Castling
        if (src == kWhiteKingHome && Empty(kF1) && Empty(kG1))
        {
            AddMove(moves, sMove(kWhiteKingHome, kG1));  // castling kingside
        }

        if (src == kWhiteKingHome && Empty(kB1) && Empty(kC1) && Empty(kD1))
        {
            AddMove(moves, sMove(kWhiteKingHome, kC1));  // castling queenside
        }

        // Black Castling
        if (src == kBlackKingHome && Empty(kF8) && Empty(kG8))
        {
            AddMove(moves, sMove(kBlackKingHome, kG8));  // castling kingside
        }

        if (src == kBlackKingHome && Empty(kB8) && Empty(kC8) && Empty(kD8))
        {
            AddMove(moves, sMove(kBlackKingHome, kC8));  // castling queenside
        }



    }

    return true;
}

bool ChessBoard::GetMovesThatSatisfy(char piece, bool bAttack, const ZPoint& endSquare, tMoveList& moves)
{
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8;x++)
        {
            char c = mBoard[y][x];
            if (c == piece)
            {
                tMoveList pieceMoves;
                tMoveList pieceAttacks;
                ZPoint src(x, y);
                GetMoves(c, src, pieceMoves, pieceAttacks);

                if (bAttack && IsOneOfMoves(endSquare, pieceAttacks) && LegalMove(src, endSquare))
                {
                    moves.push_back(sMove(src, endSquare));
                }

                if (!bAttack && IsOneOfMoves(endSquare, pieceMoves) && LegalMove(src, endSquare))
                {
                    moves.push_back(sMove(src, endSquare));
                }
            }
        }
    }

    return !moves.empty();
}


bool ChessBoard::IsOneOfMoves(const ZPoint& dst, const tMoveList& moves)
{
    for (auto m : moves)
    {
        if (m.mDest == dst)
        {
            return true;
        }
    }

    return false;
}

string ChessBoard::ToPosition(const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return "";

    return string(grid.x + 'a', 1) + string('1' + (7 - grid.y), 1);
}

ZPoint ChessBoard::FromPosition(string sPosition)
{
    StringHelpers::makelower(sPosition);

    ZPoint grid(-1, -1);
    if (sPosition.length() == 2)
    {
        char col = sPosition[0];
        char row = sPosition[1];

        if (col >= 'a' && col <= 'h')
        {
            if (row >= '1' && row <= '8')
            {
                grid.x = col - 'a';
                grid.y = row - '1';
            }

        }
    }

    return grid;
}



string ChessBoard::ToFEN()
{
    string sFEN;
    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8;)
        {
            char c = mBoard[y][x];
            if (c > 0)
            {
                sFEN += string(&c, 1);
                x++;
            }
            else
            {
                uint32_t nBlanks = 0;
                while (c == 0 && x < 8)
                {
                    nBlanks++;
                    x++;
                    c = mBoard[y][x];
                }
                if (nBlanks > 0)
                {
                    string sBlanks;
                    Sprintf(sBlanks, "%d", nBlanks);
                    sFEN += sBlanks;
                }

            }
        }
        if (y < 7)
            sFEN += "/";
    }

    if (mbWhitesTurn)
        sFEN += " w ";
    else
        sFEN += " b ";

    if (mCastlingFlags == kNone)
    {
        sFEN += "-";
    }
    else
    {
        if (mCastlingFlags | eCastlingFlags::kWhiteKingSide)
            sFEN += "K";
        if (mCastlingFlags | eCastlingFlags::kWhiteQueenSide)
            sFEN += "Q";
        if (mCastlingFlags | eCastlingFlags::kBlackKingSide)
            sFEN += "k";
        if (mCastlingFlags | eCastlingFlags::kBlackQueenSide)
            sFEN += "q";
    }

    if (ValidCoord(mEnPassantSquare))
        sFEN += " " + ToPosition(mEnPassantSquare) + " ";
    else
        sFEN += " - ";

    sFEN += StringHelpers::FromInt(mHalfMovesSinceLastCaptureOrPawnAdvance) + " ";
    sFEN += StringHelpers::FromInt(mFullMoveNumber);

    return sFEN;
}

bool ChessBoard::FromFEN(const string& sFEN)
{
    ClearBoard();
    int nIndex = 0;
    for (int y = 0; y < 8; y++)
    {
        int x = 0;
        while (x < 8)
        {
            char c = sFEN[nIndex];
            if (c >= '1' && c <= '8')
                x += (c - '0');
            else
            {
                mBoard[y][x] = c;
                ZASSERT(c == 'q' || c == 'Q' || c == 'k' || c == 'K' || c == 'n' || c == 'N' || c == 'R' || c == 'r' || c == 'b' || c == 'B' || c == 'p' || c == 'P');
                x++;
            }

            nIndex++;
        }

        // skip the '/' (or space after the final row)
        nIndex++;
    }

    // who's turn field
    char w = sFEN[nIndex];
    if (w == 'w')
        mbWhitesTurn = true;
    else
        mbWhitesTurn = false;

    nIndex += 2;    // after the w/b and space

    // castleing field
    size_t nNextSpace = sFEN.find(' ', nIndex);
    string sCastling = sFEN.substr(nIndex, nNextSpace - nIndex);

    if (sCastling == "-")
    {
        mCastlingFlags = eCastlingFlags::kNone;
    }
    else
    {
        for (int nCastlingIndex = 0; nCastlingIndex < sCastling.length(); nCastlingIndex++)
        {
            char c = sCastling[nCastlingIndex];
            if (c == 'K')
                mCastlingFlags |= eCastlingFlags::kWhiteKingSide;
            if (c == 'Q')
                mCastlingFlags |= eCastlingFlags::kWhiteQueenSide;
            if (c == 'k')
                mCastlingFlags |= eCastlingFlags::kBlackKingSide;
            if (c == 'q')
                mCastlingFlags |= eCastlingFlags::kBlackQueenSide;
        }
    }

    nIndex += (int)sCastling.length()+1;

    // en passant field
    nNextSpace = sFEN.find(' ', nIndex);
    string sEnPassant(sFEN.substr(nIndex, nNextSpace - nIndex));

    mEnPassantSquare = FromPosition(sEnPassant);
    nIndex += (int)sEnPassant.length()+1;

    // half move since last capture or pawn move
    nNextSpace = sFEN.find(' ', nIndex);
    string sHalfMoves(sFEN.substr(nIndex, nNextSpace - nIndex));

    mHalfMovesSinceLastCaptureOrPawnAdvance = (uint32_t)StringHelpers::ToInt(sHalfMoves);
    nIndex += (int)sHalfMoves.length()+1;

    mFullMoveNumber = (uint32_t)StringHelpers::ToInt(sFEN.substr(nIndex));      // last value
    mHalfMoveNumber = mFullMoveNumber + (int)!mbWhitesTurn;  // +1 if it's black's turn

    ComputeSquaresUnderAttack();
    ComputeCheckAndMate();

    return false;
}

void ChessBoard::DebugDump()
{
    TableOutput table;
    table.SetSeparator(' ', 1);

    table.AddRow(' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h');
    for (int y = 0; y < 8; y++)
    {
        int nFile = 8 - y;

        char c[8];
        for (int x = 0; x < 8; x++)
        {
            if (mBoard[y][x] == 0)
                c[x] = ' ';
            else
                c[x] = mBoard[y][x];
        }


        table.AddRow(nFile, c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]);
    }

    cout << "move:" << mFullMoveNumber << "\n";
    cout << "halfmove:" << mHalfMoveNumber << "\n";
    cout << table << std::flush;
}


size_t SkipWhitespace(const string& sText, size_t nStart = 0)
{
    uint8_t* pText = (uint8_t*)sText.c_str() + nStart;

    while (*pText && std::isspace(*pText))
    {
        pText++;
    }
    if (!*pText)
        return string::npos;

    return pText - (uint8_t*)sText.c_str();
}

size_t FindNextWhitespace(const string& sText, size_t nStart = 0)
{
    uint8_t* pText = (uint8_t*)sText.c_str() + nStart;

    while (*pText && !std::isspace(*pText))
    {
        pText++;
    }
    if (!*pText)
        return string::npos;

    return pText - (uint8_t*)sText.c_str();
}


size_t ZChessPGN::GetHalfMoveCount() 
{ 
    if (mMoves.empty())
        return 0;

    size_t nHalfMoves = mMoves.size() * 2-1; 
    string sResult = GetTag("Result");
    if (mMoves[mMoves.size() - 1].whiteAction == sResult || (mMoves[mMoves.size() - 1].blackAction == sResult))     // if last entry on last move is a result, there's one less half move
        nHalfMoves--;

    return nHalfMoves;
}


bool ZChessPGN::ParsePGN(const string& sPGN)
{
    size_t nTagIndex = 0;

    bool bDone = false;
    size_t nMoveNumber = 1;

    mMoves.clear();
    mMoves.push_back(ZPGNSANEntry());       // dummy entry so that move numbers line up

    mTags.clear();




    size_t nLastTag = 0;
    do
    {
        nTagIndex = sPGN.find("[", nTagIndex);
        if (nTagIndex != string::npos)
        {
            nTagIndex++;
            nTagIndex = SkipWhitespace(sPGN, nTagIndex);
            size_t nEndTag = sPGN.find("]", nTagIndex);

            size_t nLabelEnd = FindNextWhitespace(sPGN, nTagIndex);
            string sLabel = sPGN.substr(nTagIndex, nLabelEnd - nTagIndex);

            size_t nValueStart = sPGN.find("\"", nLabelEnd);
            if (nValueStart != string::npos)
            {
                nValueStart++;
                size_t nValueEnd = sPGN.find("\"", nValueStart);
                if (nValueEnd != string::npos)
                {
                    string sValue = sPGN.substr(nValueStart, nEndTag - nValueStart-1);
                    mTags[sLabel] = sValue;
                    ZOUT("PGN Tag - [", sLabel, " -> \"", sValue, "\"]");
                }
            }

            nTagIndex = nEndTag;
            nLastTag = nEndTag;
        }


    } while (nTagIndex != string::npos);

    size_t nParsingIndex = nLastTag;



    do
    {
        string sMoveNumberLabel;
        Sprintf(sMoveNumberLabel, "%d.", nMoveNumber);
        size_t nMoveStartIndex = sPGN.find(sMoveNumberLabel, nParsingIndex);
        if (nMoveStartIndex != string::npos)
        {
            string sNextMoveLabel;
            Sprintf(sNextMoveLabel, "%d.", nMoveNumber + 1);
            size_t nNextMoveStartIndex = sPGN.find(sNextMoveLabel, nMoveStartIndex);
            
            // if there is no next move, then this is the last move, use up the rest of the text
            size_t nMoveEndIndex = nNextMoveStartIndex;
            if (nMoveEndIndex == string::npos)
                nMoveEndIndex = sPGN.length();

            string sMoveText = sPGN.substr(nMoveStartIndex, nMoveEndIndex - nMoveStartIndex);

            ZDEBUG_OUT("Move:", nMoveNumber, " raw:", sMoveText);

            ZPGNSANEntry entry;
            

            if (!entry.ParseLine(sMoveText))
            {
                ZERROR("Failed to parse line:", sMoveText);
                return false;
            }

            mMoves.push_back(entry);
            ZASSERT(mMoves[nMoveNumber].movenumber = nMoveNumber);  // just a verification

            nParsingIndex = nMoveEndIndex;
            nMoveNumber++;
        }
        else
            bDone = true;


    } while (!bDone);


    return true;
}

string ZChessPGN::GetTag(const std::string& sTag)
{
    auto findIt = mTags.find(sTag);
    if (findIt == mTags.end())
        return "";

    return (*findIt).second;
}


// parses file and/or rank. -1 in a field if not included
ZPoint ZPGNSANEntry::LocationFromText(const std::string& sSquare)
{
    ZPoint ret(-1, -1);
    if (sSquare.length() == 1)
    {
        // only rank or file provided
        char c = sSquare[0];
        if (c >= 'a' && c <= 'h')
            ret.x = c - 'a';
        else if (c >= '1' && c <= '8')
            ret.y = 7-(c - '1');
    }
    else if (sSquare.length() == 2)
    {
        // both rank and file provided
        char c = sSquare[0];
        ZASSERT(c >= 'a' && c <= 'h');
        ret.x = c - 'a';

        c = sSquare[1];
        ZASSERT(c >= '1' && c <= '8');
        ret.y = 7-(c - '1');            // point coordinates are top to bottom.....tbd, align with board rank instead
        ZASSERT(ret.x >= 0 && ret.x <= 7 && ret.y >= 0 && ret.y <= 7);
    }
    else 
    {
        ZERROR("ZPGNSANEntry::LocationFromText() - Couldn't parse:", sSquare);
    }

    return ret;
}


ZPoint ZPGNSANEntry::ResolveSourceFromAction(bool bWhite, ChessBoard& board)
{
    string action;

    // castling actions
    if (bWhite)
    {
        action = whiteAction;
        if (action == "O-O" || action == "O-O-O")
            return kE1;
    }
    else
    {
        action = blackAction;
        if (action == "O-O" || action == "O-O-O")
            return kE8;
    }

    bool bAttack = Contains(action, "x");

    char c = PieceFromAction(bWhite);
    ZASSERT(c == 'q' || c == 'Q' || c == 'k' || c == 'K' || c == 'n' || c == 'N' || c == 'R' || c == 'r' || c == 'b' || c == 'B' || c == 'p' || c == 'P');


    ZPoint dest = DestFromAction(bWhite);

    tMoveList possibleMoves;

    bool bRet = board.GetMovesThatSatisfy(c, bAttack, dest, possibleMoves);

    ZASSERT(bRet);

    if (possibleMoves.size() == 1)
    {
        ZASSERT((*possibleMoves.begin()).mDest == dest);
        return (*possibleMoves.begin()).mSrc;
    }

    // more than one requires disambiguation

    string sDisambiguation = DisambiguationChars(bWhite);
    ZASSERT(sDisambiguation.length() == 2);
    char c1 = sDisambiguation[0];
    char c2 = sDisambiguation[1];

    if (c1 >= 'a' && c1 <= 'h') // rule one, disambiguation by file
    {
        int nFileNumber = c1 - 'a';
        for (auto move : possibleMoves)
        {
            if (move.mSrc.x == nFileNumber)
                return move.mSrc;
        }
    }
    else if (c1 >= '1' && c1 <= '8') // rule two, disambiguation by rank
    {
        int nRankNumber = 7 - (c1 - '1');
        for (auto move : possibleMoves)
        {
            if (move.mSrc.y == nRankNumber)
                return move.mSrc;
        }
    }
    else // rule three, disambiguate by square name
    {
        ZPoint square = LocationFromText(sDisambiguation);
        for (auto move : possibleMoves)
        {
            if (move.mSrc == square)
                return move.mSrc;
        }
    }

    int stophere = 5;
    return ZPoint(-1, -1);
}

ZPoint ZPGNSANEntry::DestFromAction(bool bWhite)
{
    if (bWhite)
    {
        // castling actions
        if (whiteAction == "O-O")
            return kG1;
        if (whiteAction == "O-O-O")
            return kC1;

        // promotion
        size_t nEqualsIndex = whiteAction.find("=");
        if (nEqualsIndex != string::npos)
            return LocationFromText(whiteAction.substr(nEqualsIndex - 2, 2));   // in a promotion the last two squares before equals is the location

        // check
        if (whiteAction[whiteAction.length() - 1] == '+')
            return LocationFromText(whiteAction.substr(whiteAction.length() - 3, 2));     // destination will be last two chars before +

        // checkmate
        if (whiteAction[whiteAction.length() - 1] == '#')
            return LocationFromText(whiteAction.substr(whiteAction.length() - 3, 2));     // destination will be last two chars before #


        return LocationFromText(whiteAction.substr(whiteAction.length() - 2, 2)); // destination will be last two chars

    }

    // black

    // castling options
    if (blackAction == "O-O")
        return kG8;
    if (blackAction == "O-O-O")
        return kC8;

    // promotion
    size_t nEqualsIndex = blackAction.find("=");
    if (nEqualsIndex != string::npos)
        return LocationFromText(blackAction.substr(nEqualsIndex - 2, 2));   // in a promotion the last two squares before equals is the location

    // check
    if (blackAction[blackAction.length() - 1] == '+')
        return LocationFromText(blackAction.substr(blackAction.length() - 3, 2));     // destination will be last two chars before +

    // checkmate
    if (blackAction[blackAction.length() - 1] == '#')
        return LocationFromText(blackAction.substr(blackAction.length() - 3, 2));     // destination will be last two chars before #


    return LocationFromText(blackAction.substr(blackAction.length() - 2, 2)); // destination will be last two chars
}


bool ZPGNSANEntry::ParseLine(std::string sSANLine)
{
    // extract commentary if it exists
    size_t nStartComment = sSANLine.find("{");
    if (nStartComment != string::npos)
    {
        size_t nEndComment = sSANLine.find("}", nStartComment + 1);

        annotation = sSANLine.substr(nStartComment+1, nEndComment - nStartComment);

        sSANLine.erase(nStartComment, nEndComment - nStartComment+1);

    }


    size_t nMoveEndIndex = sSANLine.find(".", 0);
    if (nMoveEndIndex == string::npos)
    {
        ZERROR("ZPGNSANEntry::ParseLine - Couldn't extract move number.");
        return false;
    }

    movenumber = StringHelpers::ToInt(sSANLine.substr(0, nMoveEndIndex));

    nMoveEndIndex++;

    size_t nStartOfWhiteText = SkipWhitespace(sSANLine, nMoveEndIndex);
    size_t nEndOfWhiteText = FindNextWhitespace(sSANLine, nStartOfWhiteText);

    size_t nStartOfBlackText = SkipWhitespace(sSANLine, nEndOfWhiteText);
    size_t nEndOfBlackText = FindNextWhitespace(sSANLine, nStartOfBlackText);

    whiteAction = sSANLine.substr(nStartOfWhiteText, nEndOfWhiteText - nStartOfWhiteText);
    blackAction = sSANLine.substr(nStartOfBlackText, nEndOfBlackText - nStartOfBlackText);

    return true;
}


char ZPGNSANEntry::IsPromotion(bool bWhite)
{
    if (bWhite)
    {
        size_t nEqualsIndex = whiteAction.find('=');
        if (nEqualsIndex == string::npos)
            return 0;
        return whiteAction[nEqualsIndex + 1];
    }
    else
    {
        size_t nEqualsIndex = blackAction.find('=');
        if (nEqualsIndex == string::npos)
            return 0;

        return blackAction[nEqualsIndex + 1] - 'Q' + 'q';   // to lowercase
    }

    return 0;
}


bool ZPGNSANEntry::IsCheck(bool bWhite)
{
    if (bWhite)
        return Contains(whiteAction, "+");

    return Contains(blackAction, "+");
}

eCastlingFlags ZPGNSANEntry::IsCastle(bool bWhite)
{
    if (bWhite)
    {
        if (whiteAction == "O-O")
            return eCastlingFlags::kWhiteKingSide;
        else if (whiteAction == "O-O-O")
            return eCastlingFlags::kWhiteQueenSide;
    }
    else
    {
        if (blackAction == "O-O")
            return eCastlingFlags::kBlackKingSide;
        else if (blackAction == "O-O-O")
            return eCastlingFlags::kBlackQueenSide;
    }           

    return eCastlingFlags::kNone;
}

bool ZPGNSANEntry::IsGameResult(bool bWhite)
{
    string action;

    if (bWhite)
        action = whiteAction;
    else
        action = blackAction;

    return action == "1-0" || action == "0-1" || action == "1/2-1/2";
}


char ZPGNSANEntry::PieceFromAction(bool bWhite)
{
    string action;

    if (bWhite)
        action = whiteAction;
    else
        action = blackAction;

    if (action.empty())
        return 0;

    char c = action[0];
    if (c == 'Q' || c == 'K' || c == 'N' || c == 'R' || c == 'B' || c == 'P')
    {
        if (bWhite)
            return c;
        return c - 'A' + 'a';       // convert to lowercase for black
    }


    ZASSERT(c >= 'a' && c <= 'h');  // better be a file
    if (bWhite)
        return 'P';

    return 'p';
}

string ZPGNSANEntry::DisambiguationChars(bool bWhite)
{
    string action;

    if (bWhite)
        action = whiteAction;
    else
        action = blackAction;

    if (action.empty())
        return "";

    char c = action[0];
    if (c == 'Q' || c == 'K' || c == 'N' || c == 'R' || c == 'B' || c == 'P'/* ||
        c == 'q' || c == 'k' || c == 'n' || c == 'r' || c == 'b' || c == 'p'*/)
    {
        return action.substr(1, 2);     // two characters following the piece name
    }

    return action.substr(0, 2);     // if no piece specified pawn assumed
}
