#include "ChessWin.h"
#include "Resources.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "helpers/RandHelpers.h"
#include <filesystem>
#include <fstream>

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
    mbOutline = true;
    mDraggingPiece = 0;
    mDraggingSourceGrid.Set(-1, -1);
    mbCopyKeyEnabled = false;
    mbEditMode = false;
    mbShowAttackCount = true;
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
    pCP->AddToggle(&mbOutline, "Outline", "type=invalidate;target=chesswin", "type=invalidate;target=chesswin", "", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddToggle(&mbEditMode, "Edit Mode", "type=invalidate;target=chesswin", "type=invalidate;target=chesswin", "", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

    pCP->AddButton("Change Turn", "type=changeturn;target=chesswin", pBtnFont, 0xffffffff, 0xff000000, ZFont::kShadowed);

    pCP->AddButton("Clear Board", "type=clearboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Reset Board", "type=resetboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddSpace(panelH/30);

    pCP->AddButton("Load Board", "type=loadboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Save Board", "type=saveboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

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

    mPieceData['K'].GenerateImageFromSymbolicFont('K', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['Q'].GenerateImageFromSymbolicFont('Q', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['R'].GenerateImageFromSymbolicFont('R', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['B'].GenerateImageFromSymbolicFont('B', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['N'].GenerateImageFromSymbolicFont('N', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['P'].GenerateImageFromSymbolicFont('P', mnPieceHeight, mpSymbolicFont, mbOutline);

    mPieceData['k'].GenerateImageFromSymbolicFont('k', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['q'].GenerateImageFromSymbolicFont('q', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['r'].GenerateImageFromSymbolicFont('r', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['b'].GenerateImageFromSymbolicFont('b', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['n'].GenerateImageFromSymbolicFont('n', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['p'].GenerateImageFromSymbolicFont('p', mnPieceHeight, mpSymbolicFont, mbOutline);

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
        // Is there a piece on this square?
        char c = mBoard.Piece(grid);
        if (c)
        {
            // only pick up a piece if
            // 1) In edit mode
            // 2) In game mode and it's white's turn and picking up white piece
            // 3) same but for black
            if (mbEditMode ||  (mBoard.IsWhite(c) && mBoard.WhitesTurn()) || (mBoard.IsBlack(c) && !mBoard.WhitesTurn()))
            {

                mDraggingPiece = c;
                mDraggingSourceGrid = grid;

                ZPoint squareOffset(ScreenToSquareOffset(x, y));
                tZBufferPtr pieceImage(mPieceData[c].mpImage);

                // If we've clicked on an opaque pixel
                if (SetCapture())
                {
                    //            OutputDebugLockless("capture x:%d, y:%d\n", mZoomOffset.mX, mZoomOffset.mY);
                    SetMouseDownPos(squareOffset.mX, squareOffset.mX);
                    mpDraggingPiece = pieceImage;
                    mrDraggingPiece.SetRect(pieceImage->GetArea());
                    mrDraggingPiece.OffsetRect(x - mrDraggingPiece.Width() / 2, y - mrDraggingPiece.Height() / 2);
                    //            mZoomOffsetAtMouseDown = mZoomOffset;
                    return true;
                }
            }

            return true;
        }
        ZDEBUG_OUT("Square clicked gridX:%d gridY:%d\n", grid.mX, grid.mY);

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
        if (mBoard.ValidCoord(mDraggingSourceGrid) && !mbCopyKeyEnabled)
        {
            ZPoint dstGrid(ScreenToGrid(x, y));
            if (mBoard.ValidCoord(dstGrid))
                mBoard.MovePiece(mDraggingSourceGrid, dstGrid, !mbEditMode);    // moving piece from source to dest
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
        }

        mpDraggingPiece.reset();
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
    if (key == VK_CONTROL)
    {
        mbCopyKeyEnabled = true;
        return true;
    }

    return ZWin::OnKeyDown(key);
}

bool ZChessWin::OnKeyUp(uint32_t key)
{
    if (key == VK_CONTROL)
    {
        mbCopyKeyEnabled = false;
        return true;
    }

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
    ZRect r(grid.mX * mnPieceHeight, grid.mY * mnPieceHeight, (grid.mX + 1) * mnPieceHeight, (grid.mY + 1) * mnPieceHeight);
    r.OffsetRect(mrBoardArea.left, mrBoardArea.top);
    return r;
}

uint32_t ZChessWin::SquareColor(const ZPoint& grid)
{
    if ((grid.mX + grid.mY % 2) % 2)
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
    return ZPoint(boardOffset.mX / mnPieceHeight, boardOffset.mY / mnPieceHeight);
}



char ZChessWin::ScreenToPalettePiece(int64_t x, int64_t y)
{
    if (mrPaletteArea.PtInRect(x, y))
    {
        int nY = y - mrPaletteArea.top;
        int nCell = nY / mnPalettePieceHeight;

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
                bool bCapture = false;
                mBoard.IsOneOfMoves(grid, legalMoves, bIncluded, bCapture);
                if (bIncluded)
                {
                    if (bCapture)
                        nSquareColor = 0xff880000;
                    else
                        nSquareColor = 0xff008800;
                }
            }

            mpTransformTexture->Fill(SquareArea(grid), nSquareColor);

            if (mbShowAttackCount)
            {

                string sCount;
                Sprintf(sCount, "%d", mBoard.UnderAttack(true, grid));
                ZRect rText(SquareArea(grid));
                defaultFont->DrawText(mpTransformTexture.get(), sCount, rText, 0xffffffff, 0xffffffff);

                rText.OffsetRect(0, defaultFont->FontHeight());

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
    uint32_t nLabelPadding = pLabelFont->GetFontParams().nHeight;
    if (mBoard.WhitesTurn())
    {
        rMoveLabel.SetRect(SquareArea(ZPoint(0, 7)));
        rMoveLabel.OffsetRect(-rMoveLabel.Width()-nLabelPadding*2, 0);
        string sLabel("White's Move");
        if (mBoard.IsKingInCheck(true))
            sLabel = "White is in Check";

        rMoveLabel = pLabelFont->GetOutputRect(rMoveLabel, sLabel.data(), sLabel.length(), ZFont::kMiddleCenter);
        rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
        mpTransformTexture->Fill(rMoveLabel, 0xffffffff);
        pLabelFont->DrawTextParagraph(mpTransformTexture.get(), sLabel, rMoveLabel, 0xff000000, 0xff000000, ZFont::kMiddleCenter);
    }
    else
    {
        rMoveLabel.SetRect(SquareArea(ZPoint(0, 0)));
        rMoveLabel.OffsetRect(-rMoveLabel.Width()- nLabelPadding*2, 0);
        string sLabel("Black's Move");
        if (mBoard.IsKingInCheck(false))
            sLabel = "Black is in Check";
        rMoveLabel = pLabelFont->GetOutputRect(rMoveLabel, sLabel.data(), sLabel.length(), ZFont::kMiddleCenter);
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
    else if (sType == "randdbboard")
    {
        LoadRandomPosition();
        return true;
    }

    return ZWin::HandleMessage(message);
}

void ZChessWin::LoadRandomPosition()
{
    std::filesystem::path filepath("res/position.fendb");
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
        int32_t nOffset = nSize / 64;
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

    return mBoard[grid.mY][grid.mX];
}

bool ChessBoard::Empty(const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return false;

    return mBoard[grid.mY][grid.mX] == 0;
}



bool ChessBoard::MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst, bool bGameMove)
{
    if (!ValidCoord(gridSrc))
    {
        ZDEBUG_OUT("Error: MovePiece src invalid:%d,%d\n", gridSrc.mX, gridSrc.mY);
        return false;
    }
    if (!ValidCoord(gridDst))
    {
        ZDEBUG_OUT("Error: MovePiece dst invalid:%d,%d\n", gridDst.mX, gridDst.mY);
        return false;
    }

    if (gridSrc == gridDst)
        return false;


    char c = mBoard[gridSrc.mY][gridSrc.mX];
    if (c == 0)
    {
        ZDEBUG_OUT("Note: MovePiece src has no piece:%d,%d\n", gridSrc.mX, gridSrc.mY);
        return false;
    }

    if (bGameMove)
    {
        if (mbWhitesTurn && !IsWhite(c))
        {
            ZDEBUG_OUT("Error: White's turn.\n");
            return false;
        }

        if (!mbWhitesTurn && IsWhite(c))
        {
            ZDEBUG_OUT("Error: Black's turn.\n");
            return false;
        }

        bool bIsCapture = false;
        if (!LegalMove(gridSrc, gridDst, bIsCapture))
        {
            ZDEBUG_OUT("Error: Illegal move.\n");
            return false;
        }

        if (bIsCapture || c == 'p' || c == 'P')
            mHalfMovesSinceLastCaptureOrPawnAdvance = 0;
        else
            mHalfMovesSinceLastCaptureOrPawnAdvance++;

        mbWhitesTurn = !mbWhitesTurn;

        if (mbWhitesTurn)
            mFullMoveNumber++;
    }


    mBoard[gridDst.mY][gridDst.mX] = c;
    mBoard[gridSrc.mY][gridSrc.mX] = 0;
    ComputeSquaresUnderAttack();
    return true;
}

bool ChessBoard::SetPiece(const ZPoint& gridDst, char c)
{
    if (!ValidCoord(gridDst))
    {
        ZDEBUG_OUT("Error: SetPiece dst invalid:%d,%d\n", gridDst.mX, gridDst.mY);
        return false;
    }

    mBoard[gridDst.mY][gridDst.mX] = c;
    ComputeSquaresUnderAttack();
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
        return mSquaresUnderAttackByBlack[atLocation.mY][atLocation.mX];
    return mSquaresUnderAttackByWhite[atLocation.mY][atLocation.mX];
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


bool ChessBoard::LegalMove(const ZPoint& src, const ZPoint& dst, bool& bCapture)
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
        return false;





    // white king may not move into check
    if (s == 'K' && IsKingInCheck(true, dst))
        return false;

    // black king may not move into check
    if (s == 'k' && IsKingInCheck(false, dst))
        return false;



    IsOneOfMoves(dst, legalMoves, bLegal, bCapture);

    return bLegal;
}

uint8_t ChessBoard::UnderAttack(bool bByWhyte, const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return false;

    if (bByWhyte)
        return mSquaresUnderAttackByWhite[grid.mY][grid.mX];

    return mSquaresUnderAttackByBlack[grid.mY][grid.mX];
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
                            mSquaresUnderAttackByWhite[square.mGrid.mY][square.mGrid.mX]++;
                    }
                    else
                    {
                        for (auto square : attackSquares)
                            mSquaresUnderAttackByBlack[square.mGrid.mY][square.mGrid.mX]++;
                    }
                }
            }
        }
    }
}


bool ChessBoard::GetMoves(char c, const ZPoint& src, tMoveList& moves, tMoveList& attackSquares)
{
    if (c == 0 || !ValidCoord(src))
        return false;

    moves.clear();
    attackSquares.clear();

    if (c == 'P')
    {
        if (src.mY == 6 && Empty(src.mX, 5) && Empty(src.mX, 4))    // pawn hasn't moved
            moves.push_back(ZPoint(src.mX, 4));             // two up
        if (src.mY > 0 && Empty(src.mX, src.mY-1))
            moves.push_back(ZPoint(src.mX, src.mY-1));     // one up if not on the last row
        if (src.mX > 0)
        {
            attackSquares.push_back(sMove(src.mX - 1, src.mY - 1, true));
            if (Opponent(c, ZPoint(src.mX - 1, src.mY - 1)) || mEnPassantSquare == ZPoint(src.mX - 1, src.mY - 1))
                moves.push_back(sMove(src.mX - 1, src.mY - 1, true));     // capture left
        }
        if (src.mX < 7)
        {
            attackSquares.push_back(sMove(src.mX + 1, src.mY - 1, true));
            if (Opponent(c, ZPoint(src.mX + 1, src.mY - 1)) || mEnPassantSquare == ZPoint(src.mX + 1, src.mY - 1))
                moves.push_back(sMove(src.mX + 1, src.mY - 1, true));     // capture right
        }
    }
    if (c == 'p')
    {
        if (src.mY == 1 && Empty(src.mX, 2) && Empty(src.mX, 3))    // pawn hasn't moved
            moves.push_back(ZPoint(src.mX, 3));            // two down
        if (src.mY < 7 && Empty(src.mX, src.mY + 1))
            moves.push_back(ZPoint(src.mX, src.mY + 1));     // one down if not on the last row
        if (src.mX > 0)
        {
            attackSquares.push_back(sMove(src.mX + 1, src.mY + 1, true));
            if (Opponent(c, ZPoint(src.mX + 1, src.mY + 1)) || mEnPassantSquare == ZPoint(src.mX + 1, src.mY + 1))
                moves.push_back(sMove(src.mX + 1, src.mY + 1, true));     // capture left
        }
        if (src.mX < 7)
        {
            attackSquares.push_back(sMove(src.mX - 1, src.mY + 1, true));
            if (Opponent(c, ZPoint(src.mX - 1, src.mY + 1)) || mEnPassantSquare == ZPoint(src.mX - 1, src.mY + 1))
                moves.push_back(sMove(src.mX - 1, src.mY + 1, true));     // capture right
        }
    }
    else if (c == 'r' || c == 'R')
    {
        ZPoint checkGrid(src);
        // run left
        do
        {
            checkGrid.mX--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));
        } while (ValidCoord(checkGrid));

        // run up
        checkGrid = src;
        do
        {
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run down
        checkGrid = src;
        do
        {
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));
        } while (ValidCoord(checkGrid));
    }
    else if (c == 'b' || c == 'B')
    {
        ZPoint checkGrid(src);
        // run up left
        do
        {
            checkGrid.mX--;
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));
        } while (ValidCoord(checkGrid));

        // run up right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));
        } while (ValidCoord(checkGrid));

        // run down left
        checkGrid = src;
        do
        {
            checkGrid.mX--;
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run down right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));
        } while (ValidCoord(checkGrid));
    }
    else if (c == 'n' || c == 'N')
    {
        ZPoint checkGrid(src);
        // up up left
        checkGrid.mY-=2;
        checkGrid.mX--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // up up right
        checkGrid = src;
        checkGrid.mY -= 2;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));

        // left left up
        checkGrid = src;
        checkGrid.mY --;
        checkGrid.mX-=2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));

        // right right up
        checkGrid = src;
        checkGrid.mY--;
        checkGrid.mX += 2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));

        // down down right
        checkGrid = src;
        checkGrid.mY += 2;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // down down left
        checkGrid = src;
        checkGrid.mY += 2;
        checkGrid.mX--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // left left down
        checkGrid = src;
        checkGrid.mY++;
        checkGrid.mX -= 2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // right right down
        checkGrid = src;
        checkGrid.mY++;
        checkGrid.mX += 2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));
    }
    else if (c == 'q' || c == 'Q')
    {
        ZPoint checkGrid(src);
        // run up left
        do
        {
            checkGrid.mX--;
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run up right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run down left
        checkGrid = src;
        do
        {
            checkGrid.mX--;
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));


        } while (ValidCoord(checkGrid));

        // run down right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run left
        checkGrid = src;
        do
        {
            checkGrid.mX--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run up
        checkGrid = src;
        do
        {
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));

        // run down
        checkGrid = src;
        do
        {
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                attackSquares.push_back(sMove(checkGrid, true));

                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
            attackSquares.push_back(sMove(checkGrid, true));

        } while (ValidCoord(checkGrid));
    }
    else if (c == 'k' || c == 'K')
    {
        ZPoint checkGrid(src);
        // up 
        checkGrid.mY--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // down
        checkGrid = src;
        checkGrid.mY++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // left
        checkGrid = src;
        checkGrid.mX--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // right
        checkGrid = src;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // left up
        checkGrid = src;
        checkGrid.mY--;
        checkGrid.mX--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // right up
        checkGrid = src;
        checkGrid.mY--;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // left down
        checkGrid = src;
        checkGrid.mY++;
        checkGrid.mX--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));


        // right down
        checkGrid = src;
        checkGrid.mY++;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));
        attackSquares.push_back(sMove(checkGrid, true));

    }

    return true;
}

void ChessBoard::IsOneOfMoves(const ZPoint& src, const tMoveList& moves, bool& bIncluded, bool& bCapture)
{
    for (auto m : moves)
    {
        if (m.mGrid == src)
        {
            bIncluded = true;
            bCapture = m.bCapture;
            return;
        }
    }

    bIncluded = false;
}

string ChessBoard::ToPosition(const ZPoint& grid)
{
    if (!ValidCoord(grid))
        return "";

    return string(grid.mX + 'a', 1) + string('1' + (7 - grid.mY), 1);
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
                grid.mX = col - 'a';
                grid.mY = row - '1';
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
                if (c == '/')
                {
                    int stophere = 5;
                }
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

    nIndex += sCastling.length()+1;

    // en passant field
    nNextSpace = sFEN.find(' ', nIndex);
    string sEnPassant(sFEN.substr(nIndex, nNextSpace - nIndex));

    mEnPassantSquare = FromPosition(sEnPassant);
    nIndex += sEnPassant.length()+1;

    // half move since last capture or pawn move
    nNextSpace = sFEN.find(' ', nIndex);
    string sHalfMoves(sFEN.substr(nIndex, nNextSpace - nIndex));

    mHalfMovesSinceLastCaptureOrPawnAdvance = StringHelpers::ToInt(sHalfMoves);
    nIndex += sHalfMoves.length()+1;

    mFullMoveNumber = StringHelpers::ToInt(sFEN.substr(nIndex));      // last value

    ComputeSquaresUnderAttack();

    return false;
}


