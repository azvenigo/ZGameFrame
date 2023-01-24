#include "ChessWin.h"
#include "Resources.h"
#include "ZWinControlPanel.h"

using namespace std;

ZChessWin::ZChessWin()
{
    msWinName = "chesswin";
    mpSymbolicFont = nullptr;
    mbOutline = true;

}
   
bool ZChessWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 10000;
    SetFocus();

    mnPieceHeight = mAreaToDrawTo.Height() / 10;


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

    InvalidateChildren();
}

bool ZChessWin::OnMouseDownL(int64_t x, int64_t y)
{
    return ZWin::OnMouseDownL(x, y);
}

bool ZChessWin::OnMouseUpL(int64_t x, int64_t y)
{
    return ZWin::OnMouseUpL(x, y);
}

bool ZChessWin::OnMouseMove(int64_t x, int64_t y)
{
    return ZWin::OnMouseMove(x, y);
}

bool ZChessWin::OnMouseDownR(int64_t x, int64_t y)
{
    return ZWin::OnMouseDownR(x, y);
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

    return ZWin::Paint();
}

void ZChessWin::DrawBoard()
{
    ZRect rFullBoard(0, 0, mnPieceHeight * 8, mnPieceHeight * 8);
    rFullBoard = rFullBoard.CenterInRect(mAreaToDrawTo);

    uint32_t nLightSquare = 0xffeeeed5;
    uint32_t nDarkSquare = 0xff7d945d;

    uint32_t nLightPiece = 0xffffffff;
    uint32_t nDarkPiece = 0xff000000;

    for (int y = 0; y < 8; y++)
    {
        for (int x = 0; x < 8; x++)
        {
            ZRect rSquare(x*mnPieceHeight, y*mnPieceHeight, (x+1)*mnPieceHeight, (y+1)*mnPieceHeight);
            rSquare.OffsetRect(rFullBoard.left, rFullBoard.top);
            if ((x + y % 2) % 2)
                mpTransformTexture->Fill(rSquare, nLightSquare);
            else
                mpTransformTexture->Fill(rSquare, nDarkSquare);

            char c = mBoard[y][x];
            std::string s(&c, 1);

            if (c == 'Q' || c == 'K' || c == 'R' || c == 'N' || c == 'B' || c == 'P')
            {
                if (mbOutline)
                {
                    int32_t nOffset = mnPieceHeight / 64;
                    ZRect rOutline(rSquare);
                    rOutline.OffsetRect(-nOffset, -nOffset);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nLightPiece, nLightPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                    rOutline.OffsetRect(nOffset * 2, 0);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nLightPiece, nLightPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                    rOutline.OffsetRect(0, nOffset * 2);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nLightPiece, nLightPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                    rOutline.OffsetRect(-nOffset * 2, 0);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nLightPiece, nLightPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                }

                mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rSquare, nDarkPiece, nDarkPiece, ZFont::kMiddleCenter, ZFont::kNormal);
            }
            else
            {
                if (mbOutline)
                {
                    int32_t nOffset = mnPieceHeight / 64;
                    ZRect rOutline(rSquare);
                    rOutline.OffsetRect(-nOffset, -nOffset);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nDarkPiece, nDarkPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                    rOutline.OffsetRect(nOffset * 2, 0);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nDarkPiece, nDarkPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                    rOutline.OffsetRect(0, nOffset * 2);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nDarkPiece, nDarkPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                    rOutline.OffsetRect(-nOffset * 2, 0);
                    mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rOutline, nDarkPiece, nDarkPiece, ZFont::kMiddleCenter, ZFont::kNormal);
                }


                mpSymbolicFont->DrawTextParagraph(mpTransformTexture.get(), s, rSquare, nLightPiece, nLightPiece, ZFont::kMiddleCenter, ZFont::kNormal);
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
    return ZWin::HandleMessage(message);
}
