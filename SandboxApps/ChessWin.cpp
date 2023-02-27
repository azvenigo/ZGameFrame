#include "ChessWin.h"
#include "Resources.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "helpers/RandHelpers.h"
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

ZChessWin::ZChessWin()
{
    msWinName = "chesswin";
    mpSymbolicFont = nullptr;
    mDraggingPiece = 0;
    mDraggingSourceGrid.Set(-1, -1);
    mbEditMode = false;
    mbShowAttackCount = true;
    mpPiecePromotionWin = nullptr;
}
   
bool ZChessWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 10000;
    SetFocus();

    mnPieceHeight = mAreaToDrawTo.Height() / 12;

    mLightSquareCol = 0xffeeeed5;
    mDarkSquareCol = 0xff7d945d;


    UpdateSize();


    int64_t panelW = grFullArea.Width() / 10;
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(grFullArea.right - panelW, grFullArea.bottom - panelH, grFullArea.right, grFullArea.bottom);     // upper right for now


    ZWinControlPanel* pCP = new ZWinControlPanel();
    pCP->SetArea(rControlPanel);

    tZFontPtr pBtnFont(gpFontSystem->GetFont(gDefaultButtonFont));

    pCP->Init();

    pCP->AddCaption("Piece Height", gDefaultTitleFont);
    pCP->AddSlider(&mnPieceHeight, 1, 26, 10, "type=updatesize;target=chesswin", true, false, pBtnFont);
    //    pCP->AddSpace(16);
    pCP->AddToggle(&mbEditMode, "Edit Mode", "type=invalidate;target=chesswin", "type=invalidate;target=chesswin", "", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

    pCP->AddButton("Change Turn", "type=changeturn;target=chesswin", pBtnFont, 0xffffffff, 0xff000000, ZFont::kShadowed);

    pCP->AddButton("Clear Board", "type=clearboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Reset Board", "type=resetboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddSpace(panelH/30);

    pCP->AddButton("Load Position", "type=loadboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Save Position", "type=saveboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

    pCP->AddButton("Load Game", "type=loadgame;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Save Game", "type=savegame;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);


    pCP->AddSpace(panelH / 30);
    pCP->AddButton("Random DB Position", "type=randdbboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

    ChildAdd(pCP);

    ZWinWatchPanel* pPanel = new ZWinWatchPanel();
    
    ZRect rWatchPanel(0,0,rControlPanel.Width()*2, rControlPanel.Height()/4);
    rWatchPanel.OffsetRect(rControlPanel.left - rWatchPanel.Width() - 16, rControlPanel.top + rControlPanel.Height() *3 /4);
    pPanel->SetArea(rWatchPanel);
    ChildAdd(pPanel);
    pPanel->AddItem(WatchType::kString, "Debug:", &msDebugStatus, 0xff000000, 0xffffffff);

    msDebugStatus = "starting test";




    return ZWin::Init();
}

bool ZChessWin::Shutdown()
{
    return ZWin::Shutdown();
}

void ZChessWin::UpdateSize()
{
    if (mpSymbolicFont)
        delete mpSymbolicFont;
    mpSymbolicFont = new ZDynamicFont();
    mpSymbolicFont->Init(ZFontParams("MS Gothic", mnPieceHeight, 400, mnPieceHeight / 4, true), false, false);
    mpSymbolicFont->GenerateSymbolicGlyph('K', 9812);
    mpSymbolicFont->GenerateSymbolicGlyph('Q', 9813);
    mpSymbolicFont->GenerateSymbolicGlyph('R', 9814);
    mpSymbolicFont->GenerateSymbolicGlyph('B', 9815);
    mpSymbolicFont->GenerateSymbolicGlyph('N', 9816);
    mpSymbolicFont->GenerateSymbolicGlyph('P', 9817);

    mpSymbolicFont->GenerateSymbolicGlyph('k', 9818);
    mpSymbolicFont->GenerateSymbolicGlyph('q', 9819);
    mpSymbolicFont->GenerateSymbolicGlyph('r', 9820);
    mpSymbolicFont->GenerateSymbolicGlyph('b', 9821);
    mpSymbolicFont->GenerateSymbolicGlyph('n', 9822);
    mpSymbolicFont->GenerateSymbolicGlyph('p', 9823);

    mPieceData['K'].GenerateImageFromSymbolicFont('K', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['Q'].GenerateImageFromSymbolicFont('Q', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['R'].GenerateImageFromSymbolicFont('R', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['B'].GenerateImageFromSymbolicFont('B', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['N'].GenerateImageFromSymbolicFont('N', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['P'].GenerateImageFromSymbolicFont('P', mnPieceHeight, mpSymbolicFont, true);

    mPieceData['k'].GenerateImageFromSymbolicFont('k', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['q'].GenerateImageFromSymbolicFont('q', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['r'].GenerateImageFromSymbolicFont('r', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['b'].GenerateImageFromSymbolicFont('b', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['n'].GenerateImageFromSymbolicFont('n', mnPieceHeight, mpSymbolicFont, true);
    mPieceData['p'].GenerateImageFromSymbolicFont('p', mnPieceHeight, mpSymbolicFont, true);

    mrBoardArea.SetRect(0, 0, mnPieceHeight * 8, mnPieceHeight * 8);
    mrBoardArea = mrBoardArea.CenterInRect(mAreaToDrawTo);


    mnPalettePieceHeight = mnPieceHeight * 8 / 12;      // 12 possible pieces drawn over 8 squares

    mrPaletteArea.SetRect(mrBoardArea.right, mrBoardArea.top, mrBoardArea.right + mnPalettePieceHeight, mrBoardArea.bottom);

    InvalidateChildren();
}


void ZChessWin::UpdateWatchPanel()
{
    msDebugStatus = "Turn: " + StringHelpers::FromInt(mBoard.GetMoveNumber()) + " ";
    if (mBoard.WhitesTurn())
        msDebugStatus += "White's Move";
    else
        msDebugStatus += "Black's Move";

    msDebugStatus += mBoard.ToFEN();
    InvalidateChildren();
}

bool ZChessWin::OnMouseDownL(int64_t x, int64_t y)
{
    ZPoint grid(ScreenToGrid(x, y));

    // In a valid grid coordinate?
    if (mBoard.ValidCoord(grid))
    {
        ZDEBUG_OUT("Square clicked gridX:%d gridY:%d\n", grid.x, grid.y);

        // Is there a piece on this square?
        char c = mBoard.Piece(grid);
        if (c)
        {
            // only pick up a piece if
            // 1) In edit mode
            // 2) In game mode and it's white's turn and picking up white piece
            // 3) same but for black
            bool bPickUpPiece = false;

            if (mbEditMode)
            {
                bPickUpPiece = true;    // pick up piece always in edit mode
            }
            else if (!mBoard.IsCheckmate()) // if game mode, only pick up piece if not checkmate
            {
                if (mBoard.IsWhite(c) && mBoard.WhitesTurn() || (mBoard.IsBlack(c) && !mBoard.WhitesTurn()))    // pick up the white piece on white's turn and vice versa
                    bPickUpPiece = true;
            }


            if (bPickUpPiece)
            {

                mDraggingPiece = c;
                mDraggingSourceGrid = grid;

                ZPoint squareOffset(ScreenToSquareOffset(x, y));
                tZBufferPtr pieceImage(mPieceData[c].mpImage);

                // If we've clicked on an opaque pixel
                if (SetCapture())
                {
                    //            OutputDebugLockless("capture x:%d, y:%d\n", mZoomOffset.x, mZoomOffset.y);
                    SetMouseDownPos(squareOffset.x, squareOffset.x);
                    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
                    mpDraggingPiece = pieceImage;
                    mrDraggingPiece.SetRect(pieceImage->GetArea());
                    mrDraggingPiece.OffsetRect(x - mrDraggingPiece.Width() / 2, y - mrDraggingPiece.Height() / 2);
                    //            mZoomOffsetAtMouseDown = mZoomOffset;
                    return true;
                }
            }

            return true;
        }

    }
    else if (mbEditMode && mrPaletteArea.PtInRect(x, y))
    {
        char c = ScreenToPalettePiece(x, y);
        if (c)
        {
            if (SetCapture())
            {
                mDraggingPiece = c;
                mDraggingSourceGrid.Set(-1, -1);
                const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
                mpDraggingPiece = mPieceData[c].mpImage;
            }
        }
    }

    return ZWin::OnMouseDownL(x, y);
}

bool ZChessWin::OnMouseUpL(int64_t x, int64_t y)
{
    if (AmCapturing())
    {
        const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
        ZPoint dstGrid(ScreenToGrid(x, y));
        if (mbEditMode)
        {
            if (mBoard.ValidCoord(mDraggingSourceGrid))
            {
                if (mBoard.ValidCoord(dstGrid))
                    mBoard.MovePiece(mDraggingSourceGrid, dstGrid, false);    // moving piece from source to dest
                else
                    mBoard.SetPiece(mDraggingSourceGrid, 0);           // dragging piece off the board
            }
            else
                mBoard.SetPiece(ScreenToGrid(x, y), mDraggingPiece);   // setting piece from palette
        }
        else
        {
            // game mode
            if (mBoard.ValidCoord(mDraggingSourceGrid) && mBoard.ValidCoord(dstGrid))
            {
                if (mBoard.IsPromotingMove(mDraggingSourceGrid, dstGrid))
                {
                    ShowPromotingWin(dstGrid);
                    mBoard.SetPiece(mDraggingSourceGrid, 0);
                }
                else
                {
                    mBoard.MovePiece(mDraggingSourceGrid, dstGrid);
                }
            }

        }




        /*

        if (mBoard.ValidCoord(mDraggingSourceGrid) )
        {
            if (mBoard.ValidCoord(dstGrid))
            {
                if (mBoard.IsPromotingMove(mDraggingSourceGrid, dstGrid))
                    ShowPromotingWin(dstGrid);
                else
                    mBoard.MovePiece(mDraggingSourceGrid, dstGrid, !mbEditMode);    // moving piece from source to dest
            }
            else
            {
                if (mbEditMode)
                    mBoard.SetPiece(mDraggingSourceGrid, 0);           // dragging piece off the board
            }
        }
        else
        {
            if (mbEditMode)
                mBoard.SetPiece(ScreenToGrid(x, y), mDraggingPiece);   // setting piece from palette
        }*/

        mpDraggingPiece = nullptr;
        mDraggingPiece = 0;
        mDraggingSourceGrid.Set(-1, -1);
        ReleaseCapture();
        UpdateWatchPanel();
        Invalidate();
    }
    return ZWin::OnMouseUpL(x, y);
}

bool ZChessWin::OnMouseMove(int64_t x, int64_t y)
{
    if (AmCapturing() && mpDraggingPiece)
    {
        mrDraggingPiece.SetRect(mpDraggingPiece->GetArea());
        mrDraggingPiece.OffsetRect(x - mrDraggingPiece.Width() / 2, y - mrDraggingPiece.Height() / 2);
        Invalidate();
    }

    return ZWin::OnMouseMove(x, y);
}

bool ZChessWin::OnMouseDownR(int64_t x, int64_t y)
{
    return ZWin::OnMouseDownR(x, y);
}

bool ZChessWin::OnKeyDown(uint32_t key)
{
    return ZWin::OnKeyDown(key);
}

bool ZChessWin::OnKeyUp(uint32_t key)
{
    return ZWin::OnKeyDown(key);
}


bool ZChessWin::Process()
{
    return ZWin::Process();
}

bool ZChessWin::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

    mpTransformTexture->Fill(mAreaToDrawTo, 0xff0000ff);
    DrawBoard();

    if (mpDraggingPiece)
        mpTransformTexture->Blt(mpDraggingPiece.get(), mpDraggingPiece->GetArea(), mrDraggingPiece);
    else if (mbEditMode)
        DrawPalette();

    return ZWin::Paint();
}

ZRect ZChessWin::SquareArea(const ZPoint& grid)
{
    ZRect r(grid.x * mnPieceHeight, grid.y * mnPieceHeight, (grid.x + 1) * mnPieceHeight, (grid.y + 1) * mnPieceHeight);
    r.OffsetRect(mrBoardArea.left, mrBoardArea.top);
    return r;
}

uint32_t ZChessWin::SquareColor(const ZPoint& grid)
{
    if ((grid.x + grid.y % 2) % 2)
        return mDarkSquareCol;

    return mLightSquareCol;
}

ZPoint ZChessWin::ScreenToSquareOffset(int64_t x, int64_t y)
{
    ZPoint grid(ScreenToGrid(x, y));
    if (mBoard.ValidCoord(grid))
    {
        ZRect rSquare(SquareArea(grid));
        return ZPoint(x - rSquare.left, y - rSquare.top);
    }

    return ZPoint(-1,-1);
}

ZPoint ZChessWin::ScreenToGrid(int64_t x, int64_t y)
{
    ZPoint boardOffset(x - mrBoardArea.left, y - mrBoardArea.top);
    return ZPoint(boardOffset.x / mnPieceHeight, boardOffset.y / mnPieceHeight);
}



char ZChessWin::ScreenToPalettePiece(int64_t x, int64_t y)
{
    if (mrPaletteArea.PtInRect(x, y))
    {
        int64_t nY = y - mrPaletteArea.top;
        int64_t nCell = nY / mnPalettePieceHeight;

        return mPalettePieces[nCell];
    }

    return 0;
}


void ZChessWin::DrawPalette()
{
    mpTransformTexture->Fill(mrPaletteArea, 0xff888888);

    ZRect rPalettePiece(mrPaletteArea);
    rPalettePiece.bottom = mrPaletteArea.top + mnPalettePieceHeight;

    for (int i = 0; i < 12; i++)
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(rPalettePiece, verts);

        gRasterizer.RasterizeWithAlpha(mpTransformTexture.get(), mPieceData[mPalettePieces[i]].mpImage.get(), verts);
        rPalettePiece.OffsetRect(0, mnPalettePieceHeight);
    }
}

void ZChessWin::DrawBoard()
{
    tMoveList legalMoves;
    tMoveList attackSquares;
    if (mDraggingPiece)
        mBoard.GetMoves(mDraggingPiece, mDraggingSourceGrid, legalMoves, attackSquares);

    tZFontPtr defaultFont(gpFontSystem->GetDefaultFont());

    ZPoint enPassantSquare = mBoard.GetEnPassantSquare();

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            ZPoint grid(x, y);

            char c = mBoard.Piece(grid);

            uint32_t nSquareColor = SquareColor(grid);
            if (mDraggingPiece && !mbEditMode)
            {
                bool bIncluded = false;
                mBoard.IsOneOfMoves(grid, legalMoves, bIncluded);
                if (bIncluded)
                {
                    if (mBoard.Opponent(c, grid))
                        nSquareColor = 0xff880000;
                    else
                        nSquareColor = 0xff008800;
                }
            }

            if (grid == enPassantSquare)
                nSquareColor = 0xff8888ff;


            mpTransformTexture->Fill(SquareArea(grid), nSquareColor);

            if (mbShowAttackCount)
            {

                string sCount;
                Sprintf(sCount, "%d", mBoard.UnderAttack(true, grid));
                ZRect rText(SquareArea(grid));
                defaultFont->DrawText(mpTransformTexture.get(), sCount, rText, 0xffffffff, 0xffffffff);

                rText.OffsetRect(0, defaultFont->Height());

                Sprintf(sCount, "%d",mBoard.UnderAttack(false, grid));
                defaultFont->DrawText(mpTransformTexture.get(), sCount, rText, 0xff000000, 0xff000000);
            }




            if (c)
            {
                if (mDraggingPiece && mDraggingSourceGrid == grid)
                    mpTransformTexture->BltAlpha(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), SquareArea(grid), 64);
                else
                    mpTransformTexture->Blt(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), SquareArea(grid));
            }
        }
    }

    ZRect rMoveLabel;
    tZFontPtr pLabelFont = gpFontSystem->GetFont(gDefaultTitleFont);
    uint32_t nLabelPadding = (uint32_t) pLabelFont->Height();
    if (mBoard.WhitesTurn())
    {
        rMoveLabel.SetRect(SquareArea(ZPoint(0, 7)));
        rMoveLabel.OffsetRect(-rMoveLabel.Width()-nLabelPadding*2, 0);
        string sLabel("White's Move");
        if (!mbEditMode && mBoard.IsKingInCheck(true))
        {
            if (mBoard.IsCheckmate())
                sLabel = "Checkmate";
            else
                sLabel = "White is in Check";
        }

        rMoveLabel = pLabelFont->GetOutputRect(rMoveLabel, (uint8_t*)sLabel.data(), sLabel.length(), ZFont::kMiddleCenter);
        rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
        mpTransformTexture->Fill(rMoveLabel, 0xffffffff);
        pLabelFont->DrawTextParagraph(mpTransformTexture.get(), sLabel, rMoveLabel, 0xff000000, 0xff000000, ZFont::kMiddleCenter);
    }
    else
    {
        rMoveLabel.SetRect(SquareArea(ZPoint(0, 0)));
        rMoveLabel.OffsetRect(-rMoveLabel.Width()- nLabelPadding*2, 0);
        string sLabel("Black's Move");
        if (!mbEditMode && mBoard.IsKingInCheck(false))
        {
            if (mBoard.IsCheckmate())
                sLabel = "Checkmate";
            else
                sLabel = "Black is in Check";
        }
        rMoveLabel = pLabelFont->GetOutputRect(rMoveLabel, (uint8_t*)sLabel.data(), sLabel.length(), ZFont::kMiddleCenter);
        rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
        mpTransformTexture->Fill(rMoveLabel, 0xff000000);
        pLabelFont->DrawTextParagraph(mpTransformTexture.get(), sLabel, rMoveLabel, 0xffffffff, 0xffffffff, ZFont::kMiddleCenter);
    }

}


bool ZChessWin::OnChar(char key)
{
#ifdef _WIN64
    switch (key)
    {
    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
    case 'e':
        mbEditMode = !mbEditMode;
        Invalidate();
        break;
    case 't':
        mBoard.SetWhitesTurn(!mBoard.WhitesTurn());
        Invalidate();
        break;
    }

    Invalidate();
#endif
    return ZWin::OnKeyDown(key);
}

bool ZChessWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "updatesize")
    {
        UpdateSize();
        return true;
    }
    else if (sType == "invalidate")
    {
        InvalidateChildren();
        return true;
    }
    else if (sType == "clearboard")
    {
        mBoard.ClearBoard();
        InvalidateChildren();
        return true;
    }
    else if (sType == "resetboard")
    {
        mBoard.ResetBoard();
        InvalidateChildren();
        return true;
    }
    else if (sType == "changeturn")
    {
        if (mbEditMode)
        {
            // Flip who's turn
            mBoard.SetWhitesTurn(!mBoard.WhitesTurn());
            InvalidateChildren();
        }
        return true;
    }
    else if (sType == "loadboard")
    {
        string sFilename;
        if (ZWinFileDialog::ShowLoadDialog("Forsyth-Edwards Notation", "*.FEN", sFilename))
        {
            string sFEN;
            if (ReadStringFromFile(sFilename, sFEN))
            {
                mBoard.FromFEN(sFEN);
                InvalidateChildren();
            }
        }
        return true;
    }
    else if (sType == "saveboard")
    {
        string sFilename;
        if (ZWinFileDialog::ShowSaveDialog("Forsyth-Edwards Notation", "*.FEN", sFilename))
        {
            std::filesystem::path filepath(sFilename);
            if (filepath.extension().empty())
                filepath+=".FEN";
            WriteStringToFile(filepath.string(), mBoard.ToFEN());
        }
        return true;
    }
    else if (sType == "loadgame")
    {
        string sFilename;
        if (ZWinFileDialog::ShowLoadDialog("Portable Game Notation", "*.PGN", sFilename))
        {
            string sPGN;
            if (ReadStringFromFile(sFilename, sPGN))
            {
                ZChessPGN pgn;
                if (pgn.ParsePGN(sPGN))
                    InvalidateChildren();
            }
        }
        return true;
    }
    else if (sType == "savegame")
    {
    }
    else if (sType == "randdbboard")
    {
        LoadRandomPosition();
        return true;
    }
    else if (sType == "promote")
    {
        // type=promote;piece=%c;x=%d;y=%d;target=chesswin
        if (message.HasParam("piece"))
        {
            char c = message.GetParam("piece")[0];
            ZPoint grid;
            grid.x = StringHelpers::ToInt(message.GetParam("x"));
            grid.y = StringHelpers::ToInt(message.GetParam("y"));
            mBoard.SetPiece(grid, c);
            mBoard.SetWhitesTurn(!mBoard.WhitesTurn());
        }

        ChildDelete(mpPiecePromotionWin);
        mpPiecePromotionWin = nullptr;
        InvalidateChildren();
        return true;
    }

    return ZWin::HandleMessage(message);
}

void ZChessWin::LoadRandomPosition()
{
    std::filesystem::path filepath("res/randpositions.fendb");
    size_t nFullSize = std::filesystem::file_size(filepath);
    size_t nRandOffset = RANDI64(0, (int64_t) nFullSize);

    std::ifstream dbFile(filepath);
    if (dbFile)
    {
        dbFile.seekg(nRandOffset);

        char c = 0;
        while (dbFile.read(&c, 1) && c != '\n');    // read forward until reaching a newline
        size_t nStartOffset = dbFile.tellg();

        while (dbFile.read(&c, 1) && c != '\n');    // read forward until reaching a newline
        size_t nEndOffset = dbFile.tellg();

        dbFile.seekg(nStartOffset);
        size_t nFENSize = nEndOffset - nStartOffset-2;

        if (nFENSize < 16 || nFENSize > 128)
        {
            ZDEBUG_OUT("ERROR: unreasonable FEN size:%d", nFENSize);
            return;
        }
       
        char* pBuf = new char[nFENSize];
        dbFile.read(pBuf, nFENSize);
        string sFEN(pBuf, nFENSize);

        mBoard.FromFEN(sFEN);
        delete[] pBuf;
        InvalidateChildren();
    }
}

void ZChessWin::ShowPromotingWin(const ZPoint& grid)
{
    ZASSERT(mpPiecePromotionWin == nullptr);
    if (mpPiecePromotionWin)
        return;

    mpPiecePromotionWin = new ZPiecePromotionWin();

    ZRect rArea(SquareArea(grid));
    rArea.left -= mnPieceHeight * (int64_t)1.5;
    rArea.right = rArea.left + mnPieceHeight * 4;

    mpPiecePromotionWin->SetArea(rArea);
    mpPiecePromotionWin->Init(&mPieceData[0], grid);
    ChildAdd(mpPiecePromotionWin);
}


bool ChessPiece::GenerateImageFromSymbolicFont(char c, int64_t nSize, ZDynamicFont* pFont, bool bOutline)
{
    uint32_t nLightPiece = 0xffffffff;
    uint32_t nDarkPiece = 0xff000000;

    mpImage.reset(new ZBuffer());
    mpImage->Init(nSize, nSize);

    std::string s(&c, 1);

    ZRect rSquare(mpImage->GetArea());

    uint32_t nCol = nLightPiece;
    uint32_t nOutline = nDarkPiece;
    if (c == 'q' || c == 'k' || c == 'r' || c == 'n' || c == 'b' || c == 'p')
    {
        nCol = nDarkPiece;
        nOutline = nLightPiece;
    }

    if (bOutline)
    {
        int64_t nOffset = nSize / 64;
        ZRect rOutline(rSquare);
        rOutline.OffsetRect(-nOffset, -nOffset);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, nOutline, nOutline, ZFont::kMiddleCenter, ZFont::kNormal);
        rOutline.OffsetRect(nOffset * 2, 0);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, nOutline, nOutline, ZFont::kMiddleCenter, ZFont::kNormal);
        rOutline.OffsetRect(0, nOffset * 2);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, nOutline, nOutline, ZFont::kMiddleCenter, ZFont::kNormal);
        rOutline.OffsetRect(-nOffset * 2, 0);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, nOutline, nOutline, ZFont::kMiddleCenter, ZFont::kNormal);
    }

    pFont->DrawTextParagraph(mpImage.get(), s, rSquare, nCol, nCol, ZFont::kMiddleCenter, ZFont::kNormal);
    return true;
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

        bool bIsCapture = Opponent(c, gridDst);


        if (bIsCapture || c == 'p' || c == 'P')
            mHalfMovesSinceLastCaptureOrPawnAdvance = 0;
        else
            mHalfMovesSinceLastCaptureOrPawnAdvance++;

        // castling
        if (c == 'K')
            mCastlingFlags &= ~(kWhiteKingSide|kWhiteQueenSide); // since white king is moving, no more castling

        if (c == 'k')
            mCastlingFlags &= ~(kBlackKingSide|kBlackQueenSide); // since black king is moving, no more castling

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
    mHalfMovesSinceLastCaptureOrPawnAdvance = 0;
    mFullMoveNumber = 1;
    mbCheck = false;
    mbCheckMate = false;
    ComputeSquaresUnderAttack();
}

void ChessBoard::ClearBoard()
{
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
    bool bLegal = false;

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
    if (s == 'K' && dst == kG1)
    {
        if (!(mCastlingFlags & kWhiteKingSide))      // White kingside disabled?
            return false;
        if (IsKingInCheck(kWhite))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kWhite, kF1))       // cannot move across check
            return false;
    }
    if (s == 'K' && dst == kC1)
    {
        if (!(mCastlingFlags & kWhiteQueenSide))      // white queenside disabled?
            return false;
        if (IsKingInCheck(kWhite))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kWhite, kD1) || IsKingInCheck(kWhite, kC1))       // cannot move across check
            return false;
    }


    if (s == 'k' && dst == kG8)
    {
        if (!(mCastlingFlags & kBlackKingSide))      // black kingside disabled?
            return false;
        if (IsKingInCheck(kBlack))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kBlack, kF8))       // cannot move across check
            return false;
    }
    if (s == 'k' && dst == kC8)
    {
        if (!(mCastlingFlags & kBlackQueenSide))      // black queenside disabled?
            return false;
        if (IsKingInCheck(kBlack))                    // cannot castle out of check
            return false;
        if (IsKingInCheck(kBlack, kD8) || IsKingInCheck(kBlack, kC8))       // cannot move across check
            return false;
    }


    IsOneOfMoves(dst, legalMoves, bLegal);

    return bLegal;
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
                            ZOUT("ComputeCheckAndMate time:", nEnd - nStart);
                            return;
                        }
                    }
                }
            }
        }

        mbCheckMate = true;
    }
    int64_t nEnd = gTimer.GetUSSinceEpoch();
    ZOUT("ComputeCheckAndMate time:", nEnd - nStart);
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

void ChessBoard::IsOneOfMoves(const ZPoint& dst, const tMoveList& moves, bool& bIncluded)
{
    for (auto m : moves)
    {
        if (m.mDest == dst)
        {
            bIncluded = true;
            return;
        }
    }

    bIncluded = false;
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
    int n = sizeof(ChessBoard);

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

    ComputeSquaresUnderAttack();
    ComputeCheckAndMate();

    return false;
}

bool ZPiecePromotionWin::Init(ChessPiece* pPieceData, ZPoint grid)
{
    mpPieceData = pPieceData;
    mDest = grid;

    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 10000;
    SetFocus();

    SetCapture();

    return ZWin::Init();
}

bool ZPiecePromotionWin::OnMouseDownL(int64_t x, int64_t y)
{
    int64_t nPieceWidth = mAreaToDrawTo.Width() / 4;
    int64_t nPiece = x/nPieceWidth;
    if (!mDest.y == 0) // promotion on 0 rank is by black
        nPiece += 4;

    string sMessage;
    Sprintf(sMessage, "type=promote;piece=%c;x=%d;y=%d;target=chesswin", mPromotionPieces[nPiece], mDest.x, mDest.y);

    gMessageSystem.Post(sMessage);
    return true;
}


bool ZPiecePromotionWin::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

    mpTransformTexture->Fill(mAreaToDrawTo, 0xff4444ff);

    ZRect rPalettePiece(mAreaToDrawTo);
    int32_t nPieceHeight = (int32_t)mAreaToDrawTo.Width() / 4;
    rPalettePiece.right = mAreaToDrawTo.left + nPieceHeight;

    int nBasePieceIndex = 0;
    if (!mDest.y == 0)
        nBasePieceIndex = 4;

    for (int i = nBasePieceIndex; i < nBasePieceIndex+4; i++)
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(rPalettePiece, verts);

        gRasterizer.RasterizeWithAlpha(mpTransformTexture.get(), mpPieceData[mPromotionPieces[i]].mpImage.get(), verts);
        rPalettePiece.OffsetRect(nPieceHeight, 0);
    }

    return ZWin::Paint();
};

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


bool ZChessPGN::ParsePGN(const string& sPGN)
{
    bool bDone = false;
    size_t nMoveNumber = 1;

    mMoves.clear();
    mMoves.push_back(ZPGNSANEntry());       // dummy entry so that move numbers line up

    mTags.clear();

    // extract tags
    size_t nTagIndex = 0;
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
            ret.y = c - '1';
    }
    else if (sSquare.length() == 2)
    {
        // both rank and file provided
        char c = sSquare[0];
        ZASSERT(c >= 'a' && c <= 'h');
        ret.x = c - 'a';

        c = sSquare[1];
        ZASSERT(c >= '1' && c <= '8');
        ret.y = c - '1';
        ZASSERT(ret.x >= 0 && ret.x <= 7 && ret.y >= 0 && ret.y <= 7);
    }
    else 
    {
        ZERROR("ZPGNSANEntry::LocationFromText() - Couldn't parse:", sSquare);
    }

    return ret;
}

bool Contains(const std::string& sText, const std::string& sSubstring)
{
    return sText.find(sSubstring) != string::npos;
}


bool ZPGNSANEntry::ParseLine(const std::string& sSANLine)
{
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

bool ZPGNSANEntry::GetMove(bool bWhite, ZPoint& src, ZPoint& dst)
{
    if (bWhite)
    {
    }

    return true;
}

