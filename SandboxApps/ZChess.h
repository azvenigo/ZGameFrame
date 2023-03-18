#pragma once

#include "ZTypes.h"
#include "ZFont.h"
#include "ZFormattedTextWin.h"
#include <list>

/////////////////////////////////////////////////////////////////////////
// 
struct sMove
{
    sMove(ZPoint src = {}, ZPoint dest = {})
    {
        mSrc = src;
        mDest = dest;
    }

    sMove(int64_t x, int64_t y)
    {
        mDest.Set(x, y);
    }

    ZPoint mSrc;
    ZPoint mDest;
};

typedef std::list<sMove> tMoveList;



enum eCastlingFlags : uint8_t
{
    kNone = 0,
    kWhiteKingSide = 1,
    kWhiteQueenSide = 2,
    kBlackKingSide = 4,
    kBlackQueenSide = 8,
    kAll = kWhiteKingSide | kWhiteQueenSide | kBlackKingSide | kBlackQueenSide
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


class ChessBoard
{
public:
    ChessBoard() { ResetBoard(); }
    void            ResetBoard();
    void            ClearBoard();

    char            Piece(const ZPoint& grid);
    bool            Empty(const ZPoint& grid);
    bool            Empty(int64_t x, int64_t y) { return Empty(ZPoint(x, y)); }
    static bool     ValidCoord(const ZPoint& l) { return (l.x >= 0 && l.x < 8 && l.y >= 0 && l.y < 8); }

    bool            MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst, bool bGameMove = true); // if bGameMove is false, this is simply manipulating the board outside of a game
    bool            PromotePiece(const ZPoint& gridSrc, const ZPoint& gridDst, char promotedPiece);
    bool            SetPiece(const ZPoint& gridDst, char c);

    sMove           GetLastMove() { return mLastMove; }

    bool            IsBlack(char c) { return  (c == 'q' || c == 'k' || c == 'r' || c == 'n' || c == 'b' || c == 'p'); }
    bool            IsWhite(char c) { return  (c == 'Q' || c == 'K' || c == 'R' || c == 'N' || c == 'B' || c == 'P'); }

    bool            LegalMove(const ZPoint& src, const ZPoint& dst);
    bool            SameSide(char c, const ZPoint& grid); // true if the grid coordinate has a piece on the same side (black or white)
    bool            Opponent(char c, const ZPoint& grid); // true if an opponent is on the grid location
    bool            GetMoves(char c, const ZPoint& src, tMoveList& moves, tMoveList& attackSquares);

    bool            GetMovesThatSatisfy(char piece, bool bAttack, const ZPoint& endsquare, tMoveList& moves);       // given a piece and a target square, find all moves that satisfy the condition
    bool            GetMovesThatSatisfy(char piece, const ZPoint& endsquare, tMoveList& moves);                    // find all moves for one side that end at a destination
    size_t          CountPieceOnRank(char piece, int64_t nRank);
    size_t          CountPieceOnFile(char piece, int64_t nFile);

    bool            IsPromotingMove(const ZPoint& src, const ZPoint& dst);

    bool            IsOneOfMoves(const ZPoint& endSquare, const tMoveList& moves);
    uint8_t         UnderAttack(bool bByWhite, const ZPoint& grid); // returns number of pieces attacking a square
    bool            IsKingInCheck(bool bWhite, const ZPoint& atLocation);   // returns true if the king is (or would be) in check at a location
    bool            IsKingInCheck(bool bWhite); // is king currently in check at his current position

    bool            IsCheckmate() { return mbCheckMate; }

    bool            WhitesTurn() { return mbWhitesTurn; }
    void            SetWhitesTurn(bool bWhitesTurn) { mbWhitesTurn = bWhitesTurn; }
    ZPoint          GetEnPassantSquare() { return mEnPassantSquare; }
    uint32_t        GetHalfMovesSinceLastCaptureOrPawnAdvance() { return mHalfMovesSinceLastCaptureOrPawnAdvance; }
    uint32_t        GetMoveNumber() { return mFullMoveNumber; }
    uint32_t        GetHalfMoveNumber() { return mHalfMoveNumber; }
    uint32_t        GetCastlingFlags() { return mCastlingFlags; }


    std::string     GetResult() { return msResult; }
    void            SetResult(std::string sResult) { msResult = sResult; }


    std::string     ToPosition(const ZPoint& grid);
    ZPoint          FromPosition(std::string sPosition);

    std::string     ToFEN();   // converts board position to FEN format
    bool            FromFEN(const std::string& sFEN);

    void            DebugDump();

protected:
    void            ComputeSquaresUnderAttack();
    void            ComputeCheckAndMate();

    char            mBoard[8][8];
    bool            mbWhitesTurn;
    bool            mbCheck;
    bool            mbCheckMate;
    uint8_t         mCastlingFlags;
    ZPoint          mEnPassantSquare;
    uint32_t        mHalfMovesSinceLastCaptureOrPawnAdvance;
    uint32_t        mFullMoveNumber;
    uint32_t        mHalfMoveNumber;

    sMove           mLastMove;
    std::string     msResult;

    uint8_t         mSquaresUnderAttackByWhite[8][8];
    uint8_t         mSquaresUnderAttackByBlack[8][8];

};


class ZPGNSANEntry
{
public:
    ZPGNSANEntry(uint32_t _movenumber = 0, const std::string _whiteaction = "", const std::string& _blackaction = "", const std::string& _whitecomment = "", const std::string& _blackcomment = "")
    {
        movenumber  = _movenumber;
        whiteAction = _whiteaction;
        blackAction = _blackaction;
        whiteComment = _whitecomment;
        blackComment = _blackcomment;
    }

    bool            ParseLine(std::string sSANLine);
    std::string     ToString();

    char            PieceFromAction(bool bWhite);
    std::string     DisambiguationChars(bool bWhite);

    ZPoint          ResolveSourceFromAction(bool bWhite, ChessBoard& board);      // may require disambiguation
    ZPoint          DestFromAction(bool bWhite);

    ZPoint          LocationFromText(const std::string& sSquare);

    static std::string  ActionFromMove(const sMove& move, ChessBoard board);   // returns a SAN action string (ex "Nb6+") from a move
    static std::string  ActionFromPromotion(const sMove& move, char promotedPiece, ChessBoard board);


    bool            IsGameResult(bool bWhite);
    bool            IsCheck(bool bWhite);
    eCastlingFlags  IsCastle(bool bWhite);
    char            IsPromotion(bool bWhite);
    bool            GetMove(bool bWhite, const ChessBoard& board, ZPoint& src, ZPoint& dst);

    uint32_t        movenumber;
    std::string     whiteAction;
    std::string     blackAction;

    std::string     whiteComment;
    std::string     blackComment;

private:
};


/*const std::string kEventTag = { "Event" };
const std::string kSiteTag = { "Site" };
const std::string kDateTag = { "Date" };
const std::string kRoundTag = { "Round" };
const std::string kWhiteTag = { "White" };
const std::string kBlackTag = { "Black" };
const std::string kWhiteELO = { "WhiteElo" };
const std::string kBlackELO = { "BlackElo" };
const std::string kResultTag = { "Result" };
const std::string kECOTag = { "ECO" };
const std::string kOpeningTag = { "Opening" };
const std::string kTimeControlTag = { "TimeControl" };
const std::string kWhiteClockTag = { "WhiteClock" };
const std::string kBlackClockTag = { "BlackClock" };
const std::string kClockTag = { "Clock" };*/

const std::string pgnTags[] = 
{
    "Event",
    "Site",
    "Date",
    "Round"
    "White",
    "WhiteElo",
    "Black",
    "BlackElo",
    "Result",
    "ECO",
    "Opening",
    "WhiteFideId",
    "BlackFideId",
    "EventDate",
    "EventType"
};

class ZChessPGN
{
public:
    ZChessPGN() {}

    bool ParsePGN(const std::string& sPGN);

    std::string ToString();

    // for live games
    bool AddMove(const std::string& sAction);


    bool IsDraw() { return GetTag("Result") == "1/2-1/2"; }
    bool WhiteWins() { return GetTag("Result") == "1-0"; }
    bool BlackWins() { return GetTag("Result") == "0-1"; }

    void ResetTags();
    std::string GetTag(const std::string& sTag);


    size_t GetMoveCount() { return mMoves.size(); }
    size_t GetHalfMoveCount();

    std::unordered_map<std::string, std::string> mTags;
    std::vector<ZPGNSANEntry> mMoves;

    bool IsStandardTag(const std::string& sTag);
};