#include "ZChessWin.h"
#include "Resources.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "ZWinBtn.H"
#include "helpers/RandHelpers.h"
#include "helpers/LoggingHelpers.h"
#include <filesystem>
#include <fstream>
#include "ZTimer.h"
#include "ZAnimator.h"
#include "ZAnimObjects.h"
#include "ZGUIHelpers.h"

using namespace std;


ZChoosePGNWin::ZChoosePGNWin()
{
    msWinName = "choosepgnwin";
    mbAcceptsCursorMessages = true;
    mIdleSleepMS = 10000;
    mFillColor = gDefaultDialogFill;
}

bool ZChoosePGNWin::Init()
{
    mFont.sFacename = "Verdana";
    mFont.nHeight = 24;
    mFont.nWeight = 200;


    ZWinSizablePushBtn* pBtn;

    size_t nButtonWidth = (mAreaToDrawTo.Width() - gDefaultSpacer * 2) / 2;
    size_t nButtonHeight = (mFont.nHeight + gDefaultSpacer * 2);

    ZRect rButton(0, 0, nButtonWidth, nButtonHeight);
    rButton.OffsetRect(mAreaToDrawTo.Width()-gDefaultSpacer- nButtonWidth, mAreaToDrawTo.Height() - gDefaultSpacer - rButton.Height());

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("Cancel"); // wingdings 3 to the beggining
    pBtn->SetTextLook(gpFontSystem->GetFont(gDefaultButtonFont), ZTextLook());
    pBtn->SetArea(rButton);
    //pBtn->SetMessage("cancelpgnselect;target=chesswin");
    pBtn->SetMessage(ZMessage("cancelpgnselect", mpParentWin));
    ChildAdd(pBtn);

    ZRect rGameTags(gDefaultSpacer, gDefaultSpacer + mFont.nHeight * 2, mAreaToDrawTo.Width() - gDefaultSpacer, rButton.top - gDefaultSpacer);

    mpGamesList = new ZFormattedTextWin();
    mpGamesList->SetArea(rGameTags);
    mpGamesList->SetFill(gDefaultTextAreaFill);
    mpGamesList->SetDrawBorder();
    ChildAdd(mpGamesList);

    return ZWin::Init();
}

bool ZChoosePGNWin::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

    mpTransformTexture->Fill(mAreaToDrawTo, mFillColor);

    ZRect rText(mAreaToDrawTo);
    rText.OffsetRect(gDefaultSpacer, gDefaultSpacer);
    gpFontSystem->GetFont(mFont)->DrawTextParagraph(mpTransformTexture.get(), msCaption, rText);
    return ZWin::Paint();
}

bool ZChoosePGNWin::HandleMessage(const ZMessage& message)
{
    return ZWin::HandleMessage(message);
}

bool ZChoosePGNWin::ListGamesFromPGN(string& sFilename, string& sPGNFile)
{
    mpGamesList->Clear();

    std::filesystem::path pgnFile(sFilename);
    

    // count number of games in text
    size_t nIndex = 0;
    size_t nCount = 1;
    do
    {
        nIndex = sPGNFile.find("[Event \"", nIndex);
        if (nIndex != string::npos)
        {
            size_t nStartEventName = nIndex+8; // skip the "[Event \""
            size_t nEndEventName = sPGNFile.find("\"", nStartEventName+1);
            string sEventName = sPGNFile.substr(nStartEventName, nEndEventName - nStartEventName);

            size_t nStartSiteName = sPGNFile.find("Site \"", nEndEventName + 1);
            nStartSiteName += 6;  // skip the "Site \""
            size_t nEndSiteName = sPGNFile.find("\"", nStartSiteName);
            string sSiteName = sPGNFile.substr(nStartSiteName, nEndSiteName - nStartSiteName);

            size_t nStartDate = sPGNFile.find("Date \"", nEndSiteName + 1);
            nStartDate += 6;  // skip the "Date \""
            size_t nEndDate = sPGNFile.find("\"", nStartDate);
            string sDate = sPGNFile.substr(nStartDate, nEndDate - nStartDate);

            size_t nStartRound = sPGNFile.find("Round \"", nEndDate + 1);
            nStartRound += 7;  // skip the "Round \""
            size_t nEndRound = sPGNFile.find("\"", nStartRound);
            string sRound = sPGNFile.substr(nStartRound, nEndRound - nStartRound);



            size_t nEndPGNContent = sPGNFile.find("[Event ", nEndRound);        // find next event
            if (nEndPGNContent == string::npos)
                nEndPGNContent = sPGNFile.length();     // clip at end

            string sPGNContent = sPGNFile.substr(nIndex, nEndPGNContent - nIndex);



            string sListBoxEntry = "<line wrap=0><text color=0xff000000 color2=0xff000000 fontparams=" + StringHelpers::URL_Encode(mFont) + " position=lb link=setpgn;contents="+ StringHelpers::URL_Encode(sPGNContent) +";target=" + mpParentWin->GetTargetName() + ">" + StringHelpers::FromInt(nCount) + ". " +
                sDate + "/" + sEventName + "/"+ sSiteName + "/" + sRound +"</text></line>";
            mpGamesList->AddLineNode(sListBoxEntry);

            nIndex = nEndPGNContent;    // length of "[Event"
            nCount++;
        }
    } while (nIndex != string::npos);

    msCaption = "Select from " + StringHelpers::FromInt(nCount) + " games found in \"" + pgnFile.filename().string() + "\"";

    mpGamesList->SetScrollable();
    InvalidateChildren();
    return true;
}




ZPGNWin::ZPGNWin()
{
    msWinName = "pgnwin";
    mbAcceptsCursorMessages = true;
    mIdleSleepMS = 10000;
    mFillColor = gDefaultDialogFill;
}

bool ZPGNWin::Init()
{
    mMoveFont.sFacename = "Verdana";
    mMoveFont.nHeight = 40;
    mMoveFont.nWeight = 200;

    mTagsFont.sFacename = "Verdana";
    mTagsFont.nHeight = 20;

    mBoldFont = mMoveFont;
    mBoldFont.nWeight = 800;


    ZRect rGameTags(gDefaultSpacer, gDefaultSpacer, mAreaToDrawTo.Width() - gDefaultSpacer, mAreaToDrawTo.Height() / 3 - gDefaultSpacer);

    mpGameTagsWin = new ZFormattedTextWin();
    mpGameTagsWin->SetArea(rGameTags);
    mpGameTagsWin->SetFill(gDefaultTextAreaFill);
    mpGameTagsWin->SetDrawBorder();

    ChildAdd(mpGameTagsWin); 



    ZWinSizablePushBtn* pBtn;

    size_t nButtonSlots = 10;
    size_t nButtonSize = (mAreaToDrawTo.Width() - gDefaultSpacer *2) / nButtonSlots;

    ZRect rButton(0, 0, nButtonSize, nButtonSize);
    rButton.OffsetRect(gDefaultSpacer, mAreaToDrawTo.Height() - gDefaultSpacer - rButton.Height()); 

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption(")"); // wingdings 3 to the beggining
    pBtn->SetTextLook(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight), ZTextLook());
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("beginning", this));
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("v"); // back one
    pBtn->SetTextLook(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight), ZTextLook());
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("backone", this));
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("w"); // forward one
    pBtn->SetTextLook(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight), ZTextLook());
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("forwardone", this));
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("*"); // to end
    pBtn->SetTextLook(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight), ZTextLook());
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("end", this));
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("1"); // open file
    pBtn->SetTextLook(gpFontSystem->GetFont("Wingdings", gDefaultButtonFont.nHeight), ZTextLook());
    rButton.OffsetRect(rButton.Width() *2, 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("loadgame", mpParentWin));
    ChildAdd(pBtn);


    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("<"); // save file
    pBtn->SetTextLook(gpFontSystem->GetFont("Wingdings", gDefaultButtonFont.nHeight), ZTextLook());
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage(ZMessage("savegame", mpParentWin));
    ChildAdd(pBtn);

    ZRect rMoves(rGameTags.left, rGameTags.bottom + gDefaultSpacer, rGameTags.right, rButton.top - gDefaultSpacer * 2);

    mpMovesWin = new ZFormattedTextWin();
    mpMovesWin->SetArea(rMoves);
    mpMovesWin->SetFill(0xff444444);
    mpMovesWin->SetDrawBorder();

    ChildAdd(mpMovesWin);

    mCurrentHalfMoveNumber = 0;

    return ZWin::Init();
}


bool ZPGNWin::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

    mpTransformTexture->Fill(mAreaToDrawTo, mFillColor);


    return ZWin::Paint();
}

bool ZPGNWin::FromPGN(ZChessPGN& pgn)
{
    mPGN = pgn;
    mCurrentHalfMoveNumber = 0;

    UpdateView();
    InvalidateChildren();

    return true;
}

string ZPGNWin::GetPGN()
{
    return mPGN.ToString();
}

bool ZPGNWin::AddAction(const std::string& sAction)
{
    if (mPGN.AddMove(sAction))
    {
        mCurrentHalfMoveNumber = mPGN.GetHalfMoveCount() - 1;
        UpdateView();
        InvalidateChildren();
        return true;
    }

    return false;
}


bool ZPGNWin::Clear()
{
    if (mpGameTagsWin)
        mpGameTagsWin->Clear();
    if (mpMovesWin)
        mpMovesWin->Clear();
    InvalidateChildren();
    return true;
}


void ZPGNWin::UpdateView()
{
    mpGameTagsWin->Clear();

    string sTag;

    const int knTags = sizeof(pgnTags)/sizeof(string);
    for (int nTagIndex = 0; nTagIndex < knTags; nTagIndex++)
    {
        string sValue = mPGN.GetTag(pgnTags[nTagIndex]);
        
        if (!sValue.empty())
        {
            sTag = "<line wrap=0><text color=0xffffffff color2=0xffffffff fontparams=" + StringHelpers::URL_Encode(mTagsFont) + " position=lb>" + pgnTags[nTagIndex] + "</text><text fontparams=" + StringHelpers::URL_Encode(mTagsFont) + " color=0xff000000 color2=0xff000000 position=rb>" + sValue + "</text></line>";
            mpGameTagsWin->AddLineNode(sTag);
        }
    }

    mpGameTagsWin->SetScrollable();

    mpMovesWin->Clear();

    int nHalfMove = 2;
    for (int nMove = 1; nMove < mPGN.mMoves.size(); nMove++)   // start at 2 as first two halfmoves are blank entry move
    {
        ZPGNSANEntry& move = mPGN.mMoves[nMove];

        string sMoveLine;
        if (mCurrentHalfMoveNumber > 0 && (mCurrentHalfMoveNumber == nHalfMove || mCurrentHalfMoveNumber == nHalfMove-1))
        {
            ZDEBUG_OUT("UpdateView: mCurrentHalfMoveNumber:", mCurrentHalfMoveNumber, ", nMove:", nMove, "\n");
            if ((mCurrentHalfMoveNumber+1) % 2 == 0)
            {
                // highlight white
                sMoveLine = "<line wrap=0><text fontparams=" + StringHelpers::URL_Encode(mBoldFont) + " color=0xff0088ff color2=0xff0088ff position=lb link=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove - 1) + ">" + StringHelpers::FromInt(nMove) + ". [" + move.whiteAction + "]</text>";
                sMoveLine += "<text fontparams=" + StringHelpers::URL_Encode(mBoldFont) + " color=0xff000000 color2=0xff000000 position=rb link=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove) + ">" + move.blackAction + "</text></line>";
            }
            else
            {
                // highlight black
                sMoveLine = "<line wrap=0><text fontparams=" + StringHelpers::URL_Encode(mBoldFont) + " color=0xffffffff color2=0xffffffff position=lb link=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove - 1) + ">" + StringHelpers::FromInt(nMove) + ". " + move.whiteAction + "</text>";
                sMoveLine += "<text fontparams=" + StringHelpers::URL_Encode(mBoldFont) + " color=0xff0088ff color2=0xff0088ff position=rb link=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove) + ">[" + move.blackAction + "]</text></line>";
            }
            mpMovesWin->AddLineNode(sMoveLine);
        }
        else
        {

            sMoveLine = "<line wrap=0><text fontparams=" + StringHelpers::URL_Encode(mMoveFont) + " color=0xffffffff color2=0xffffffff position=lb link=setmove;target=pgnwin;halfmove="+ StringHelpers::FromInt(nHalfMove-1) +">" + StringHelpers::FromInt(nMove) + ". " + move.whiteAction + "</text>";
            sMoveLine += "<text fontparams=" + StringHelpers::URL_Encode(mMoveFont) + " color=0xff000000 color2=0xff000000 position=rb link=setmove;target=pgnwin;halfmove="+StringHelpers::FromInt(nHalfMove)+">" + move.blackAction + "</text></line>";
            mpMovesWin->AddLineNode(sMoveLine);
        }

        nHalfMove += 2;
    }
    mpMovesWin->InvalidateChildren();
    mpMovesWin->SetScrollable();
    mpMovesWin->SetUnderlineLinks(false);
    mpMovesWin->ScrollTo(mMoveFont.nHeight * (2*(mCurrentHalfMoveNumber/2)-10) / 2);

}


bool ZPGNWin::SetHalfMove(int64_t nHalfMove)
{
    if (nHalfMove >= 0 && nHalfMove < mPGN.GetHalfMoveCount())
    {
        mCurrentHalfMoveNumber = nHalfMove;
//        string sMessage;
//        Sprintf(sMessage, "sethistoryindex;target=chesswin;halfmove=%d", mCurrentHalfMoveNumber);
//        gMessageSystem.Post(sMessage);
        gMessageSystem.Post("sethistoryindex", mpParentWin, "halfmove", mCurrentHalfMoveNumber);
        UpdateView();
        return true;
    }    

    return false;
}


bool ZPGNWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "end")
    {
        SetHalfMove(mPGN.GetHalfMoveCount()-1); 
        return true;
    }
    else if (sType == "backone")
    {
        SetHalfMove(mCurrentHalfMoveNumber - 1);
        return true;
    }
    else if (sType == "forwardone")
    {
        SetHalfMove(mCurrentHalfMoveNumber + 1);
        return true;
    }
    else if (sType == "beginning")
    {
        SetHalfMove(0);
        return true;
    }
    else if (sType == "setmove")
    {
        SetHalfMove(StringHelpers::ToInt(message.GetParam("halfmove")));
        return true;
    }


    return ZWin::HandleMessage(message);
}


const int64_t kAutoplayMaxMSBetweenMoves = 2000;
const int64_t kAutoplayMinMSBetweenMoves = 200;

ZChessWin::ZChessWin()
{
    msWinName = "chesswin" + gMessageSystem.GenerateUniqueTargetName();

    mpSymbolicFont = nullptr;
    mDraggingPiece = 0;
    mDraggingSourceGrid.Set(-1, -1);
    mHiddenSquare.Set(-1, -1);
    mbEditMode = false;
    mbShowAttackCount = false;
    mpPiecePromotionWin = nullptr;
    mpPGNWin = nullptr;
    mpChoosePGNWin = nullptr;
    mpSwitchSidesButton = nullptr;
    mpStatusWin = nullptr;
    mbDemoMode = false;
    mnDemoModeNextMoveTimeStamp = 0;
    mAutoplayMSBetweenMoves = (kAutoplayMaxMSBetweenMoves+kAutoplayMinMSBetweenMoves)/2;
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


    if (mbDemoMode)
        LoadRandomGame();

    if (!mbDemoMode)
    {

        ZWinControlPanel* pCP = new ZWinControlPanel();
        pCP->SetArea(rControlPanel);

        tZFontPtr pBtnFont(gpFontSystem->GetFont(gDefaultButtonFont));

        string invalidateMsg(ZMessage("invalidate", this));

        pCP->Init();

        ZTextLook buttonLook(ZTextLook::kEmbossed, 0xff737373, 0xff737373);
        ZTextLook checkedButtonLook(ZTextLook::kEmbossed, 0xff737373, 0xff73ff73);

        pCP->AddCaption("Piece Height", gDefaultCaptionFont, ZTextLook(), ZGUI::Center, gDefaultDialogFill);
        pCP->AddSlider(&mnPieceHeight, 1, 26, 10, ZMessage("updatesize", this), true, false, pBtnFont);
        //    pCP->AddSpace(16);
        pCP->AddToggle(&mbEditMode, "Edit Mode", ZMessage("toggleeditmode", this), ZMessage("toggleeditmode", this), "", pBtnFont, checkedButtonLook, buttonLook);

        pCP->AddButton("Clear Board", ZMessage("clearboard", this), pBtnFont, buttonLook);
        pCP->AddButton("Reset Board", ZMessage("resetboard", this), pBtnFont, buttonLook);
        pCP->AddSpace(panelH / 30);

/*        pCP->AddButton("Load Position", "loadboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);
        pCP->AddButton("Save Position", "saveboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);*/

        pCP->AddSpace(panelH / 30);
        pCP->AddButton("Load Random Game", ZMessage("randgame", this), pBtnFont, buttonLook);
        pCP->AddToggle(&mbDemoMode, "Demo Mode", invalidateMsg, invalidateMsg, "", pBtnFont, checkedButtonLook, buttonLook);

        pCP->AddCaption("Playback Speed", gDefaultCaptionFont, ZTextLook(), ZGUI::Center, gDefaultDialogFill);
        pCP->AddSlider(&mAutoplayMSBetweenMoves, kAutoplayMinMSBetweenMoves, kAutoplayMaxMSBetweenMoves, 1);

        ChildAdd(pCP);


        mpPGNWin = new ZPGNWin();
        ZRect rPGNPanel(0, 0, rControlPanel.Width() * 1.5, rControlPanel.Height());
        rPGNPanel.OffsetRect(rControlPanel.left - rPGNPanel.Width() - gDefaultSpacer, rControlPanel.top);
        mpPGNWin->SetArea(rPGNPanel);
        ChildAdd(mpPGNWin);

        mpSwitchSidesButton = new ZWinSizablePushBtn();
        mpSwitchSidesButton->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
        mpSwitchSidesButton->SetArea(ZRect(0, 0, mnPieceHeight*1.5, mnPieceHeight / 1.5));
        mpSwitchSidesButton->SetCaption("Change Turn");
        mpSwitchSidesButton->SetMessage(ZMessage("changeturn", this));
        mpSwitchSidesButton->SetTextLook(pBtnFont, ZTextLook(ZTextLook::kEmbossed, 0xffffffff, 0xff000000));
        ChildAdd(mpSwitchSidesButton, false);


        ZRect rStatusPanel(0, 0, mrBoardArea.Width(), mnPieceHeight);

        mpStatusWin = new ZWinLabel();
        mpStatusWin->SetLook(gpFontSystem->GetFont(ZFontParams("Ariel", 80, 600)), ZTextLook(ZTextLook::kShadowed), ZGUI::Center, gDefaultTextAreaFill);
        mpStatusWin->SetArea(ZGUI::Arrange(rStatusPanel, mrBoardArea, ZGUI::ICOB, gDefaultSpacer, gDefaultSpacer));
        mpStatusWin->SetText("Welcome to ZChess");
        ChildAdd(mpStatusWin);
    }


    mpAnimator = new ZAnimator();


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

    if (mpSwitchSidesButton)
        mpSwitchSidesButton->Arrange(ZGUI::OLIC, mrBoardArea, gDefaultSpacer, gDefaultSpacer);

    if (mpStatusWin)
    {
        ZRect rStatusPanel(0, 0, mrBoardArea.Width(), mnPieceHeight);
        mpStatusWin->SetArea(ZGUI::Arrange(rStatusPanel, mrBoardArea, ZGUI::ICOB, gDefaultSpacer, gDefaultSpacer));
    }


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
                    string sAction = ZPGNSANEntry::ActionFromMove(sMove(mDraggingSourceGrid, dstGrid), mBoard);
                    ZOUT("Action: ", sAction);

                    if (mBoard.MovePiece(mDraggingSourceGrid, dstGrid))
                    {
                        if (mpPGNWin)
                            mpPGNWin->AddAction(sAction);

                        if (mpStatusWin)
                            mpStatusWin->SetText(sAction);

                        const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
                        if (mHistory.empty())
                        {
                            ChessBoard board;
                            mHistory.push_back(board); // initial board
                        }
                        mHistory.push_back(mBoard);
                    }
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
    if (mpAnimator)
    {
        if (mpAnimator->HasActiveObjects())
            Invalidate();

        if (mbDemoMode && gTimer.GetUSSinceEpoch() > mnDemoModeNextMoveTimeStamp)
        {
            const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
            if (mHistory.size() > 0 && mHistory[mHistory.size() - 1].GetHalfMoveNumber() == mBoard.GetHalfMoveNumber())
            {
                LoadRandomGame();
            }
            else
            {
                ZDEBUG_OUT("Setting halfmove:", mBoard.GetHalfMoveNumber() + 1);
                mnDemoModeNextMoveTimeStamp = gTimer.GetUSSinceEpoch() + ((kAutoplayMaxMSBetweenMoves-mAutoplayMSBetweenMoves)*1000);
                //string sMessage;
                //Sprintf(sMessage, "sethistoryindex;target=chesswin;halfmove=%d", mBoard.GetHalfMoveNumber());
                //gMessageSystem.Post(sMessage);
                gMessageSystem.Post("sethistoryindex", this, "halfmove", mBoard.GetHalfMoveNumber());
            }
        }
    }


    return ZWin::Process();
}

bool ZChessWin::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

    mpTransformTexture->Fill(mAreaToDrawTo, 0xff444444);
    DrawBoard();

    if (mpDraggingPiece)
        mpTransformTexture->Blt(mpDraggingPiece.get(), mpDraggingPiece->GetArea(), mrDraggingPiece);
    else if (mbEditMode)
        DrawPalette();

    if (mpAnimator && mpAnimator->HasActiveObjects())
        mpAnimator->Paint();

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
    mpTransformTexture->Fill(mrPaletteArea, gDefaultTextAreaFill);

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
/*            if (mDraggingPiece && !mbEditMode)
            {
                if (mBoard.IsOneOfMoves(grid, legalMoves))
                {
                    if (mBoard.Opponent(c, grid))
                        nSquareColor = 0xff880000;
                    else
                        nSquareColor = 0xff008800;
                }
            }

            if (grid == enPassantSquare)
                nSquareColor = 0xff8888ff;*/

            sMove lastMove = mBoard.GetLastMove();
            if (grid == lastMove.mSrc || grid == lastMove.mDest)
                nSquareColor = 0xff0088ff;


            mpTransformTexture->Fill(SquareArea(grid), nSquareColor);

            if (mbShowAttackCount)
            {

                string sCount;
                Sprintf(sCount, "%d", mBoard.UnderAttack(true, grid));
                ZRect rText(SquareArea(grid));
                defaultFont->DrawText(mpTransformTexture.get(), sCount, rText);

                rText.OffsetRect(0, defaultFont->Height());

                Sprintf(sCount, "%d",mBoard.UnderAttack(false, grid));
                defaultFont->DrawText(mpTransformTexture.get(), sCount, rText);
            }



            if (grid == mHiddenSquare)  // do not draw piece while animation playing
                continue;


            if (c)
            {
                if (mDraggingPiece && mDraggingSourceGrid == grid)
                    mpTransformTexture->BltAlpha(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), SquareArea(grid), 64);
                else
                    mpTransformTexture->Blt(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), SquareArea(grid));
            }
        }
    }



    // Draw labels if not in demo mode
    if (!mbDemoMode)
    {
        ZRect rMoveLabel;
        tZFontPtr pLabelFont = gpFontSystem->GetFont(gDefaultTitleFont);
        uint32_t nLabelPadding = (uint32_t)pLabelFont->Height();
        if (mBoard.WhitesTurn())
        {
            rMoveLabel.SetRect(SquareArea(ZPoint(0, 7)));
            rMoveLabel.OffsetRect(-rMoveLabel.Width() - nLabelPadding * 2, 0);
            string sLabel("White's Move");
            if (!mbEditMode && mBoard.IsKingInCheck(true))
            {
                if (mBoard.IsCheckmate())
                    sLabel = "Checkmate";
                else
                    sLabel = "White is in Check";
            }

            rMoveLabel = pLabelFont->GetOutputRect(rMoveLabel, (uint8_t*)sLabel.data(), sLabel.length(), ZGUI::Center);
            rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
            mpTransformTexture->Fill(rMoveLabel, 0xffffffff);
            pLabelFont->DrawTextParagraph(mpTransformTexture.get(), sLabel, rMoveLabel,ZTextLook(ZTextLook::kNormal, 0xff000000, 0xff000000), ZGUI::Center);
        }
        else
        {
            rMoveLabel.SetRect(SquareArea(ZPoint(0, 0)));
            rMoveLabel.OffsetRect(-rMoveLabel.Width() - nLabelPadding * 2, 0);
            string sLabel("Black's Move");
            if (!mbEditMode && mBoard.IsKingInCheck(false))
            {
                if (mBoard.IsCheckmate())
                    sLabel = "Checkmate";
                else
                    sLabel = "Black is in Check";
            }
            rMoveLabel = pLabelFont->GetOutputRect(rMoveLabel, (uint8_t*)sLabel.data(), sLabel.length(), ZGUI::Center);
            rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
            mpTransformTexture->Fill(rMoveLabel, 0xff000000);
            pLabelFont->DrawTextParagraph(mpTransformTexture.get(), sLabel, rMoveLabel, ZTextLook(), ZGUI::Center);
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

void ZChessWin::UpdateStatus(const std::string& sText, uint32_t col)
{
    if (mpStatusWin)
    {
        mpStatusWin->SetText(sText);
        mpStatusWin->UpdateLook(ZTextLook(ZTextLook::kShadowed, col, col));
    }
}

bool ZChessWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();

    if (sType == "updatesize")
    {
        UpdateSize();
        return true;
    }
    else if (sType == "statusmessage")
    {
        if (message.HasParam("col"))
            UpdateStatus(message.GetParam("message"), StringHelpers::ToInt(message.GetParam("col")));
        else
            UpdateStatus(message.GetParam("message"));
        Invalidate();
    }
    else if (sType == "invalidate")
    {
        InvalidateChildren();
        return true;
    }
    else if (sType == "clearboard")
    {
        mBoard.ClearBoard();
        if (mpPGNWin)
            mpPGNWin->Clear();
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
/*    else if (sType == "loadboard")
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
    }*/
    else if (sType == "loadgame")
    {
        string sFilename;
        if (ZWinFileDialog::ShowLoadDialog("Portable Game Notation", "*.PGN", sFilename))
        {
            LoadPGN(sFilename);
        }
        return true;
    }
    else if (sType == "savegame")
    {
        string sFilename;
        if (ZWinFileDialog::ShowSaveDialog("Portable Game Notation", "*.PGN", sFilename))
        {
            SavePGN(sFilename);
        }
        return true;
    }
    else if (sType == "setpgn")
    {
        string sPGN = StringHelpers::URL_Decode(message.GetParam("contents"));
        ZChessPGN pgn;
        if (pgn.ParsePGN(sPGN))
        {
            FromPGN(pgn);
            InvalidateChildren();

            ChildDelete(mpChoosePGNWin);
            mpChoosePGNWin = nullptr;
        }

        return true;
    }
    else if (sType == "cancelpgnselect")
    {
        ChildDelete(mpChoosePGNWin);
        mpChoosePGNWin = nullptr;
        InvalidateChildren();
        return true;
    }
    else if (sType == "randgame")
    {
        LoadRandomGame();
        return true;
    }
    else if (sType == "toggleeditmode")
    {
/*        ZRect r(mpSwitchSidesButton->GetArea());
        r.MoveRect(mrBoardArea.left-r.Width(), (mrBoardArea.bottom + mrBoardArea.top)/2 - r.Height()/2);
        mpSwitchSidesButton->SetArea(r);*/
        mpSwitchSidesButton->Arrange(ZGUI::OLIC, mrBoardArea, gDefaultSpacer, gDefaultSpacer);
        mpSwitchSidesButton->SetVisible(mbEditMode);
        Invalidate();
        return true;
    }
    else if (sType == "promote")
    {
        // promote;piece=%c;x=%d;y=%d;target=chesswin
        if (message.HasParam("piece"))
        {
            char c = message.GetParam("piece")[0];
            ZPoint grid;
            grid.x = StringHelpers::ToInt(message.GetParam("x"));
            grid.y = StringHelpers::ToInt(message.GetParam("y"));

            string sAction = ZPGNSANEntry::ActionFromPromotion(sMove(mDraggingSourceGrid, grid), c, mBoard);
            ZOUT("Action: ", sAction);

            if (mBoard.PromotePiece(mDraggingSourceGrid, grid, c))
            {
                if (mpPGNWin)
                    mpPGNWin->AddAction(sAction);
                if (mpStatusWin)
                    mpStatusWin->SetText(sAction);

                const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
                if (mHistory.empty())
                {
                    ChessBoard board;
                    mHistory.push_back(board); // initial board
                }
                mHistory.push_back(mBoard);
            }
        }

        ChildDelete(mpPiecePromotionWin);
        mpPiecePromotionWin = nullptr;
        InvalidateChildren();
        return true;
    }
    else if (sType == "sethistoryindex")
    {
        UpdateStatus("");
        if (mHistory.empty())
        {
            ZWARNING("No game history");
            return true;
        }

        int64_t nHalfMove = StringHelpers::ToInt(message.GetParam("halfmove"));
        const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
        if (nHalfMove >= 0)
        {
            if (nHalfMove >= mHistory.size())
                nHalfMove = mHistory.size() - 1;


            // remember previous board for animation purposes
            ChessBoard prevBoard = mBoard;
            mBoard = mHistory[nHalfMove];

            sMove move = mBoard.GetLastMove();

            if (mBoard.ValidCoord(move.mSrc) && mBoard.ValidCoord(move.mDest) && (kAutoplayMaxMSBetweenMoves - mAutoplayMSBetweenMoves) > 250)
            {
                int nTransformTime = (kAutoplayMaxMSBetweenMoves-mAutoplayMSBetweenMoves) / 2;
                if (nTransformTime > 500)
                    nTransformTime = 500;

                ZRect rSrcSquareArea = SquareArea(move.mSrc);
                ZRect rDstSquareArea = SquareArea(move.mDest);


                // If there was 
                ZAnimObject_TransformingImage* pImage;
                if (prevBoard.GetHalfMoveNumber() == mBoard.GetHalfMoveNumber() - 1)
                {
                    char prevPiece = prevBoard.Piece(move.mDest);
                    if (prevPiece)
                    {
                        pImage = new ZAnimObject_TransformingImage(mPieceData[prevPiece].mpImage.get());
                        pImage->StartTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top)));
                        pImage->AddTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top), 1.0, 0.0, 255, ""), nTransformTime);
                        pImage->SetDestination(mpTransformTexture);
                        mpAnimator->AddObject(pImage);
                    }
                }


                pImage = new ZAnimObject_TransformingImage(mPieceData[mBoard.Piece(move.mDest)].mpImage.get());
                pImage->StartTransformation(ZTransformation(ZPoint(rSrcSquareArea.left, rSrcSquareArea.top)));
                pImage->AddTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top), 1.0, 0.0, 255, ZMessage("hidesquare;x=-1;y=-1", this)), nTransformTime);
                pImage->AddTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top)), 33);    // 33ms to at least cover 30fps
                pImage->SetDestination(mpTransformTexture);
                mHiddenSquare = move.mDest;
                mpAnimator->AddObject(pImage);
            }

            InvalidateChildren();
        }
        else
        {
            ZERROR("ERROR: Couldn't load board from history halfmove:", nHalfMove,  " entries:" , mHistory.size());
        }
        return true;
    }
    else if (sType == "hidesquare")
    {
        mHiddenSquare.Set(StringHelpers::ToInt(message.GetParam("x")), StringHelpers::ToInt(message.GetParam("y")));
        Invalidate();
        return true;
    }

    return ZWin::HandleMessage(message);
}

bool ZChessWin::LoadPGN(string sFilename)
{
    if (std::filesystem::file_size(sFilename) > 10 * 1024 * 1024)
    {
        ZERROR("ERROR: File ", sFilename, " exceeds maximum allowable 10MiB");
        return false;
    }

    ZOUT("Loading PGN file:", sFilename);
    string sPGN;
    if (ReadStringFromFile(sFilename, sPGN))
    {
        size_t nEventIndex = sPGN.find("[Event ", 0);
        if (nEventIndex == string::npos)
            return false;

        size_t nSecondEventIndex = sPGN.find("[Event ", nEventIndex + 7);
        if (nSecondEventIndex != string::npos)
        {
            // show choose pgn window
            if (mpChoosePGNWin)
                ChildDelete(mpChoosePGNWin);

            mpChoosePGNWin = new ZChoosePGNWin();
            int64_t panelW = grFullArea.Width() / 10;
            int64_t panelH = grFullArea.Height() / 2;
            ZRect rControlPanel(grFullArea.right - panelW, grFullArea.bottom - panelH, grFullArea.right, grFullArea.bottom);     // upper right for now

            ZRect rChoosePGNWin(0, 0, rControlPanel.Width() * 1.5, rControlPanel.Height());
            rChoosePGNWin.OffsetRect(rControlPanel.left - rChoosePGNWin.Width() - gDefaultSpacer, rControlPanel.top);
            mpChoosePGNWin->SetArea(rChoosePGNWin);
            ChildAdd(mpChoosePGNWin);
            mpChoosePGNWin->ListGamesFromPGN(sFilename, sPGN);
            UpdateStatus("Select game from list...");
            return true;
        }
        else
        {
            // only one pgn game in file.....parse it
            ZChessPGN pgn;
            if (pgn.ParsePGN(sPGN))
            {
                FromPGN(pgn);
                InvalidateChildren();
                UpdateStatus("Game loaded");
                return true;
            }
        }

    }
    return false;
}

bool ZChessWin::SavePGN(string sFilename)
{
    if (mpPGNWin)
    {
        string sPGN(mpPGNWin->GetPGN());
        UpdateStatus("Game saved");
        return WriteStringToFile(sFilename, sPGN);
    }

    return false;
}



void ZChessWin::LoadRandomGame()
{
    std::filesystem::path filepath("res/twic1241.pgn");
    size_t nFullSize = std::filesystem::file_size(filepath);
    size_t nRandOffset = RANDI64(0, (int64_t) nFullSize - 1024);

    int64_t nStartOffset = -1;
    int64_t nEndOffset = -1;

    std::ifstream dbFile(filepath, ios::binary);
    if (dbFile)
    {
        dbFile.seekg(nRandOffset);
        string sLine;
        while (std::getline(dbFile, sLine) && (nStartOffset == -1 || nEndOffset == -1))
        {
            size_t nFind = sLine.find("[Event ");
            if (nFind != string::npos)
            {
                if (nStartOffset == -1)
                    nStartOffset = ((int64_t)dbFile.tellg() - (sLine.length() + 1));
                else if (nEndOffset == -1)
                    nEndOffset = ((int64_t)dbFile.tellg() - (sLine.length() + 1));
            }
        }

        size_t nPGNSize = nEndOffset - nStartOffset;
        dbFile.seekg(nStartOffset);
        char* pBuf = new char[nPGNSize];
        dbFile.read(pBuf, nPGNSize);
        string sPGN(pBuf, nPGNSize);

        ZChessPGN pgn;
        if (pgn.ParsePGN(sPGN))
        {
            FromPGN(pgn);
            InvalidateChildren();
        }

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
    UpdateStatus("Select piece");
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
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, ZTextLook(ZTextLook::kNormal, nOutline, nOutline), ZGUI::Center);
        rOutline.OffsetRect(nOffset * 2, 0);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, ZTextLook(ZTextLook::kNormal, nOutline, nOutline), ZGUI::Center);
        rOutline.OffsetRect(0, nOffset * 2);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, ZTextLook(ZTextLook::kNormal, nOutline, nOutline), ZGUI::Center);
        rOutline.OffsetRect(-nOffset * 2, 0);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, ZTextLook(ZTextLook::kNormal, nOutline, nOutline), ZGUI::Center);
    }

    pFont->DrawTextParagraph(mpImage.get(), s, rSquare, ZTextLook(ZTextLook::kNormal, nCol, nCol), ZGUI::Center);
    return true;
}




bool ZChessWin::FromPGN(ZChessPGN& pgn)
{
    const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
    mHistory.clear();

    ChessBoard board;
    mHistory.push_back(board); // initial board
    
    if (mpPGNWin)
        mpPGNWin->FromPGN(pgn);

    size_t nMove = 1;
    bool bDone = false;
    while (!bDone && nMove < pgn.GetMoveCount())
    {
        ZPGNSANEntry& move = pgn.mMoves[nMove];

        if (move.IsGameResult(kWhite))
        {
            cout << "Final result:" << pgn.GetTag("Result") << "\n";
            bDone = true;
            break;
        }
        else
        {
//            cout << "Before move - w:[" << move.whiteAction << "] b:" << move.blackAction << "\n";
//            board.DebugDump();

            ZPoint dst = move.DestFromAction(kWhite);
            ZASSERT(board.ValidCoord(dst));

            ZPoint src = move.ResolveSourceFromAction(kWhite, board);
            ZASSERT(board.ValidCoord(src));

            bool bRet;

            char promotion = move.IsPromotion(kWhite);
            if (promotion)
                bRet = board.PromotePiece(src, dst, promotion);
            else
                bRet = board.MovePiece(src, dst, true);
            ZASSERT(bRet);

            mHistory.push_back(board);

            if (move.IsGameResult(kBlack))
            {
                mHistory[mHistory.size() - 1].SetResult(pgn.GetTag("Result"));

                cout << "Final result:" << pgn.GetTag("Result") << "\n";
                if (mpPGNWin)
                    mpPGNWin->SetHalfMove(0);

                bDone = true;
                break;
            }
            else
            {

                //cout << "Before move - w:" << move.whiteAction << " b:[" << move.blackAction << "]\n";
                //board.DebugDump();

                ZPoint dst = move.DestFromAction(kBlack);
                ZASSERT(board.ValidCoord(dst));

                ZPoint src = move.ResolveSourceFromAction(kBlack, board);
                ZASSERT(board.ValidCoord(src));

                char promotion = move.IsPromotion(kBlack);
                if (promotion)
                    bRet = board.PromotePiece(src, dst, promotion);
                else
                    bRet = board.MovePiece(src, dst, true);
                ZASSERT(bRet);

                mHistory.push_back(board);
            }
        }

        nMove++;
    }

    return true;
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

//    string sMessage;
//    Sprintf(sMessage, "promote;piece=%c;x=%d;y=%d;target=chesswin", mPromotionPieces[nPiece], mDest.x, mDest.y);
//    Sprintf(sMessage, "piece=%c;x=%d;y=%d", mPromotionPieces[nPiece], mDest.x, mDest.y);

    gMessageSystem.Post("promote", mpParentWin, "piece", mPromotionPieces[nPiece], "x", mDest.x, "y", mDest.y);
    return true;
}


bool ZPiecePromotionWin::Paint()
{
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpTransformTexture.get()->GetMutex());
    if (!mbInvalid)
        return true;

//    mpTransformTexture->Fill(mAreaToDrawTo, 0xff4444ff);

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

