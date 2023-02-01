#include "ChessWin.h"
#include "Resources.h"
#include "ZWinControlPanel.h"

using namespace std;


char     mResetBoard[8][8] =
{ {'R','N','B','Q','K','B','N','R'},
    {'P','P','P','P','P','P','P','P'},
    {0},
    {0},
    {0},
    {0},
    {'p','p','p','p','p','p','p','p'},
    {'r','n','b','q','k','b','n','r'}
};


ZChessWin::ZChessWin()
{
    msWinName = "chesswin";
    mpSymbolicFont = nullptr;
    mbOutline = true;
    mDraggingPiece = 0;
    mDraggingSourceGrid.Set(-1, -1);
    mbCopyKeyEnabled = false;
    mbEditMode = true;
    mbViewWhiteOnBottom = true;
}
   
bool ZChessWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 10000;
    SetFocus();

    mnPieceHeight = mAreaToDrawTo.Height() / 10;

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



    pCP->AddButton("Clear Board", "type=clearboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Reset Board", "type=resetboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
    pCP->AddButton("Rotate Board", "type=rotateboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

    ChildAdd(pCP);




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
    mpSymbolicFont->GenerateSymbolicGlyph('k', 9812);
    mpSymbolicFont->GenerateSymbolicGlyph('q', 9813);
    mpSymbolicFont->GenerateSymbolicGlyph('r', 9814);
    mpSymbolicFont->GenerateSymbolicGlyph('b', 9815);
    mpSymbolicFont->GenerateSymbolicGlyph('n', 9816);
    mpSymbolicFont->GenerateSymbolicGlyph('p', 9817);

    mpSymbolicFont->GenerateSymbolicGlyph('K', 9818);
    mpSymbolicFont->GenerateSymbolicGlyph('Q', 9819);
    mpSymbolicFont->GenerateSymbolicGlyph('R', 9820);
    mpSymbolicFont->GenerateSymbolicGlyph('B', 9821);
    mpSymbolicFont->GenerateSymbolicGlyph('N', 9822);
    mpSymbolicFont->GenerateSymbolicGlyph('P', 9823);

    mPieceData['k'].GenerateImageFromSymbolicFont('k', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['q'].GenerateImageFromSymbolicFont('q', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['r'].GenerateImageFromSymbolicFont('r', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['b'].GenerateImageFromSymbolicFont('b', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['n'].GenerateImageFromSymbolicFont('n', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['p'].GenerateImageFromSymbolicFont('p', mnPieceHeight, mpSymbolicFont, mbOutline);

    mPieceData['K'].GenerateImageFromSymbolicFont('K', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['Q'].GenerateImageFromSymbolicFont('Q', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['R'].GenerateImageFromSymbolicFont('R', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['B'].GenerateImageFromSymbolicFont('B', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['N'].GenerateImageFromSymbolicFont('N', mnPieceHeight, mpSymbolicFont, mbOutline);
    mPieceData['P'].GenerateImageFromSymbolicFont('P', mnPieceHeight, mpSymbolicFont, mbOutline);

    mrBoardArea.SetRect(0, 0, mnPieceHeight * 8, mnPieceHeight * 8);
    mrBoardArea = mrBoardArea.CenterInRect(mAreaToDrawTo);


    mnPalettePieceHeight = mnPieceHeight * 8 / 12;      // 12 possible pieces drawn over 8 squares

    mrPaletteArea.SetRect(mrBoardArea.right, mrBoardArea.top, mrBoardArea.right + mnPalettePieceHeight, mrBoardArea.bottom);

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
                mrDraggingPiece.OffsetRect(x-mrDraggingPiece.Width()/2, y-mrDraggingPiece.Height()/2);
                //            mZoomOffsetAtMouseDown = mZoomOffset;
                return true;
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
                mBoard.MovePiece(mDraggingSourceGrid, dstGrid);    // moving piece from source to dest
            else
                mBoard.SetPiece(mDraggingSourceGrid, 0);           // dragging piece off the board
        }
        else
        {
            mBoard.SetPiece(ScreenToGrid(x, y), mDraggingPiece);   // setting piece from palette
        }

        mpDraggingPiece.reset();
        mDraggingPiece = 0;
        mDraggingSourceGrid.Set(-1, -1);
        ReleaseCapture();
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
    std::list<sMove> legalMoves;
    if (mDraggingPiece)
        mBoard.GetMoves(mDraggingPiece, mDraggingSourceGrid, legalMoves);


    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            ZPoint grid(x, y);

            char c;
            if (mbViewWhiteOnBottom)
            {
                c = mBoard.Piece(grid);
            }
            else
            {
                ZPoint useGrid(7 - x, 7 - y);
                c = mBoard.Piece(useGrid);   // drawing top to bottom using pieces on the rotated board
            }


            uint32_t nSquareColor = SquareColor(grid);
            if (mDraggingPiece)
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







            if (c)
            {
                if (mDraggingPiece && mDraggingSourceGrid == grid)
                    mpTransformTexture->BltAlpha(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), SquareArea(grid), 64);
                else
                    mpTransformTexture->Blt(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), SquareArea(grid));
            }
        }
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
    else if (sType == "rotateboard")
    {
//        RotateBoard();
        mbViewWhiteOnBottom = !mbViewWhiteOnBottom;
        InvalidateChildren();
        return true;
    }

    return ZWin::HandleMessage(message);
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
    if (c == 'Q' || c == 'K' || c == 'R' || c == 'N' || c == 'B' || c == 'P')
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



bool ChessBoard::MovePiece(const ZPoint& gridSrc, const ZPoint& gridDst)
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

    mBoard[gridDst.mY][gridDst.mX] = c;
    mBoard[gridSrc.mY][gridSrc.mX] = 0;
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
    return true;
}


void ChessBoard::ResetBoard()
{
    memcpy(mBoard, mResetBoard, sizeof(mBoard));
}

void ChessBoard::ClearBoard()
{
    memset(mBoard, 0, sizeof(mBoard));
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


    return false;
}


bool ChessBoard::GetMoves(char c, const ZPoint& src, std::list<sMove>& moves)
{
    if (c == 0 || !ValidCoord(src))
        return false;

    if (c == 'p')
    {
        if (src.mY == 6 && Empty(src.mX, 5) && Empty(src.mX, 4))    // pawn hasn't moved
            moves.push_back(ZPoint(src.mX, 4));             // two up
        if (src.mY > 0 && Empty(src.mX, src.mY-1))
            moves.push_back(ZPoint(src.mX, src.mY-1));     // one up if not on the last row
        if (src.mX > 0 && Opponent(c, ZPoint(src.mX -1, src.mY -1)))
            moves.push_back(sMove(src.mX -1, src.mY-1, true));     // capture left
        if (src.mX < 7 && Opponent(c, ZPoint(src.mX+1, src.mY -1)))
            moves.push_back(sMove(src.mX+1, src.mY-1, true));     // capture right
    }
    if (c == 'P')
    {
        if (src.mY == 1 && Empty(src.mX, 2) && Empty(src.mX, 3))    // pawn hasn't moved
            moves.push_back(ZPoint(src.mX, 3));            // two down
        if (src.mY < 7 && Empty(src.mX, src.mY + 1))
            moves.push_back(ZPoint(src.mX, src.mY + 1));     // one down if not on the last row
        if (src.mX > 0 && Opponent(c, ZPoint(src.mX + 1, src.mY + 1)))
            moves.push_back(sMove(src.mX + 1, src.mY + 1, true));     // capture left
        if (src.mX < 7 && Opponent(c, ZPoint(src.mX - 1, src.mY + 1)))
            moves.push_back(sMove(src.mX - 1, src.mY + 1, true));     // capture right
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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
        } while (ValidCoord(checkGrid));

        // run right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
        } while (ValidCoord(checkGrid));

        // run up
        checkGrid = src;
        do
        {
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));

        } while (ValidCoord(checkGrid));

        // run down
        checkGrid = src;
        do
        {
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));

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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
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

        // up up right
        checkGrid = src;
        checkGrid.mY -= 2;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));

        // left left up
        checkGrid = src;
        checkGrid.mY --;
        checkGrid.mX-=2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));

        // right right up
        checkGrid = src;
        checkGrid.mY--;
        checkGrid.mX += 2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));

        // down down right
        checkGrid = src;
        checkGrid.mY += 2;
        checkGrid.mX++;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));

        // down down left
        checkGrid = src;
        checkGrid.mY += 2;
        checkGrid.mX--;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));


        // left left down
        checkGrid = src;
        checkGrid.mY++;
        checkGrid.mX -= 2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));

        // right right down
        checkGrid = src;
        checkGrid.mY++;
        checkGrid.mX += 2;
        if (Opponent(c, checkGrid))
            moves.push_back(sMove(checkGrid, true));
        else if (Empty(checkGrid))
            moves.push_back(sMove(checkGrid));



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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));

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
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
        } while (ValidCoord(checkGrid));

        // run left
        checkGrid = src;
        do
        {
            checkGrid.mX--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
        } while (ValidCoord(checkGrid));

        // run right
        checkGrid = src;
        do
        {
            checkGrid.mX++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
        } while (ValidCoord(checkGrid));

        // run up
        checkGrid = src;
        do
        {
            checkGrid.mY--;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));

        } while (ValidCoord(checkGrid));

        // run down
        checkGrid = src;
        do
        {
            checkGrid.mY++;
            if (Opponent(c, checkGrid))
            {
                moves.push_back(sMove(checkGrid, true));
                break;
            }
            else if (!Empty(checkGrid))
                break;
            moves.push_back(sMove(checkGrid));
        } while (ValidCoord(checkGrid));





    }











    return true;
}

void ChessBoard::IsOneOfMoves(const ZPoint& src, const std::list<sMove>& moves, bool& bIncluded, bool& bCapture)
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


/*void ChessBoard::RotateBoard()
{
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            char c = mBoard[y][x];
            mBoard[y][x] = mBoard[7 - y][7 - x];
            mBoard[7 - y][7 - x] = c;
        }
    }
}*/

