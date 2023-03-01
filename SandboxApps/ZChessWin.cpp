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
    pBtn->SetFont(gpFontSystem->GetFont(gDefaultButtonFont));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=cancelpgnselect;target=chesswin");
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
    gpFontSystem->GetFont(mFont)->DrawTextParagraph(mpTransformTexture.get(), msCaption, rText, 0xffffffff, 0xffffffff);
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



            size_t nEndPGNContent = sPGNFile.find("[Event", nEndRound);        // find next event
            if (nEndPGNContent == string::npos)
                nEndPGNContent = sPGNFile.length();     // clip at end

            string sPGNContent = sPGNFile.substr(nIndex, nEndPGNContent - nIndex);



            string sListBoxEntry = "<line wrap=0><text size=0 color=0xff000000 color2=0xff000000 position=MiddleLeft link=type=setpgn;contents="+ StringHelpers::URL_Encode(sPGNContent) +";target=chesswin>" + StringHelpers::FromInt(nCount) + ". " +
                sDate + "/" + sEventName + "/"+ sSiteName + "/" + sRound +"</text></line>";
            mpGamesList->AddTextLine(sListBoxEntry, mFont, 0xffffffff, 0xffffffff, ZFont::kNormal, ZFont::kBottomRight, false);

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
    mFont.sFacename = "Verdana";
    mFont.nHeight = 30;
    mFont.nWeight = 200;

    mBoldFont = mFont;
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
    pBtn->SetFont(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=beginning;target=pgnwin");
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("v"); // back one
    pBtn->SetFont(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=backone;target=pgnwin");
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("w"); // forward one
    pBtn->SetFont(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=forwardone;target=pgnwin");
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("*"); // to end
    pBtn->SetFont(gpFontSystem->GetFont("Wingdings 3", gDefaultButtonFont.nHeight));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=end;target=pgnwin");
    ChildAdd(pBtn);

    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("1"); // open file
    pBtn->SetFont(gpFontSystem->GetFont("Wingdings", gDefaultButtonFont.nHeight));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    rButton.OffsetRect(rButton.Width() *2, 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=loadgame;target=chesswin");
    ChildAdd(pBtn);


    pBtn = new ZWinSizablePushBtn();
    pBtn->SetImages(gStandardButtonUpEdgeImage, gStandardButtonDownEdgeImage, grStandardButtonEdge);
    pBtn->SetCaption("<"); // save file
    pBtn->SetFont(gpFontSystem->GetFont("Wingdings", gDefaultButtonFont.nHeight));
    pBtn->SetColors(0xffffffff, 0xffffffff);
    pBtn->SetStyle(ZFont::kNormal);
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->SetMessage("type=savegame;target=chesswin");
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
    mCurrentHalfMoveNumber = mPGN.GetHalfMoveCount()-1;

    UpdateView();
    InvalidateChildren();

    return true;
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
    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kEventTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kEventTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kSiteTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kSiteTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kDateTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kDateTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kRoundTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kRoundTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kWhiteTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kWhiteTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    if (!mPGN.GetTag(kWhiteELO).empty())
    {
        sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kWhiteELO + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kWhiteELO) + "</text></line>";
        mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);
    }

    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kBlackTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kBlackTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    if (!mPGN.GetTag(kBlackELO).empty())
    {
        sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kBlackELO + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kBlackELO) + "</text></line>";
        mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);
    }



    sTag = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft>" + kResultTag + "</text><text size=0 color=0xff000000 color2=0xff000000 position=middleRight>" + mPGN.GetTag(kResultTag) + "</text></line>";
    mpGameTagsWin->AddTextLine(sTag, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);

    mpGameTagsWin->SetScrollable();

    mpMovesWin->Clear();

    for (int nHalfMove = 2; nHalfMove < mPGN.GetHalfMoveCount(); nHalfMove+=2)   // start at 2 as first two halfmoves are blank entry move
    {
        int nMove = (nHalfMove+1) / 2;
        ZPGNSANEntry& move = mPGN.mMoves[nMove];

        string sMoveLine;
        if (mCurrentHalfMoveNumber > 0 && (mCurrentHalfMoveNumber == nHalfMove || mCurrentHalfMoveNumber == nHalfMove-1))
        {
            ZDEBUG_OUT("UpdateView: mCurrentHalfMoveNumber:", mCurrentHalfMoveNumber, ", nMove:", nMove, "\n");
            if ((mCurrentHalfMoveNumber+1) % 2 == 0)
            {
                // highlight white
                sMoveLine = "<line wrap=0><text size=0 color=0xff0088ff color2=0xff0088ff position=MiddleLeft link=type=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove - 1) + ">" + StringHelpers::FromInt(nMove) + ". [" + move.whiteAction + "]</text>";
                sMoveLine += "<text size=0 color=0xff000000 color2=0xff000000 position=middleRight link=type=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove) + ">" + move.blackAction + "</text></line>";
            }
            else
            {
                // highlight black
                sMoveLine = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft link=type=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove - 1) + ">" + StringHelpers::FromInt(nMove) + ". " + move.whiteAction + "</text>";
                sMoveLine += "<text size=0 color=0xff0088ff color2=0xff0088ff position=middleRight link=type=setmove;target=pgnwin;halfmove=" + StringHelpers::FromInt(nHalfMove) + ">[" + move.blackAction + "]</text></line>";
            }
            mpMovesWin->AddTextLine(sMoveLine, mBoldFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);
        }
        else
        {

            sMoveLine = "<line wrap=0><text size=0 color=0xffffffff color2=0xffffffff position=MiddleLeft link=type=setmove;target=pgnwin;halfmove="+ StringHelpers::FromInt(nHalfMove-1) +">" + StringHelpers::FromInt(nMove) + ". " + move.whiteAction + "</text>";
            sMoveLine += "<text size=0 color=0xff000000 color2=0xff000000 position=middleRight link=type=setmove;target=pgnwin;halfmove="+StringHelpers::FromInt(nHalfMove)+">" + move.blackAction + "</text></line>";
            mpMovesWin->AddTextLine(sMoveLine, mFont, 0xffff0000, 0xffff0000, ZFont::kNormal, ZFont::kBottomLeft, false);
        }

    }
    mpMovesWin->InvalidateChildren();
    mpMovesWin->SetScrollable();
    mpMovesWin->ScrollTo(mFont.nHeight * (2*(mCurrentHalfMoveNumber/2)-10) / 2);

}


bool ZPGNWin::SetHalfMove(int64_t nHalfMove)
{
    if (nHalfMove >= 0 && nHalfMove < mPGN.GetMoveCount() * 2)
    {
        mCurrentHalfMoveNumber = nHalfMove;
        string sMessage;
        Sprintf(sMessage, "type=sethistoryindex;target=chesswin;halfmove=%d", mCurrentHalfMoveNumber);
        gMessageSystem.Post(sMessage);
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
        SetHalfMove(mPGN.GetHalfMoveCount()); 
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


ZChessWin::ZChessWin()
{
    msWinName = "chesswin";
    mpSymbolicFont = nullptr;
    mDraggingPiece = 0;
    mDraggingSourceGrid.Set(-1, -1);
    mbEditMode = false;
    mbShowAttackCount = true;
    mpPiecePromotionWin = nullptr;
    mpChoosePGNWin = nullptr;
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

    pCP->AddSpace(panelH / 30);
    pCP->AddButton("Random DB Position", "type=randdbboard;target=chesswin", pBtnFont, 0xff737373, 0xff73ff73, ZFont::kEmbossed);

    ChildAdd(pCP);


/*    ZWinWatchPanel* pPanel = new ZWinWatchPanel();
    
    ZRect rWatchPanel(0,0,rControlPanel.Width()*2, rControlPanel.Height()/4);
    rWatchPanel.OffsetRect(rControlPanel.left - rWatchPanel.Width() - 16, rControlPanel.top + rControlPanel.Height() *3 /4);
    pPanel->SetArea(rWatchPanel);
    ChildAdd(pPanel);
    pPanel->AddItem(WatchType::kString, "Debug:", &msDebugStatus, 0xff000000, 0xffffffff);

    msDebugStatus = "starting test";
    */

    mpPGNWin = new ZPGNWin();
    ZRect rPGNPanel(0, 0, rControlPanel.Width() * 1.5, rControlPanel.Height());
    rPGNPanel.OffsetRect(rControlPanel.left - rPGNPanel.Width() - gDefaultSpacer, rControlPanel.top );
    mpPGNWin->SetArea(rPGNPanel);
    ChildAdd(mpPGNWin);




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
            LoadPGN(sFilename);
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
    }
    else if (sType == "savegame")
    {
        return true;
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
            mBoard.PromotePiece(mDraggingSourceGrid, grid, c);
        }

        ChildDelete(mpPiecePromotionWin);
        mpPiecePromotionWin = nullptr;
        InvalidateChildren();
        return true;
    }
    else if (sType == "sethistoryindex")
    {
        int64_t nHalfMove = StringHelpers::ToInt(message.GetParam("halfmove"));
        if (nHalfMove >= 0 && nHalfMove < mHistory.size())
        {
            mBoard = mHistory[nHalfMove];
            InvalidateChildren();
        }
        else
        {
            ZERROR("ERROR: Couldn't load board from history halfmove:", nHalfMove,  " entries:" , mHistory.size());
        }
        return true;
    }

    return ZWin::HandleMessage(message);
}

bool ZChessWin::LoadPGN(string sFilename)
{
    ZOUT("Loading PGN file:", sFilename);
    string sPGN;
    if (ReadStringFromFile(sFilename, sPGN))
    {
        size_t nEventIndex = sPGN.find("[Event", 0);
        if (nEventIndex == string::npos)
            return false;

        size_t nSecondEventIndex = sPGN.find("[Event", nEventIndex + 6);
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
                return true;
            }
        }

    }
    return false;
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




bool ZChessWin::FromPGN(ZChessPGN& pgn)
{
    mHistory.clear();
    mBoard.ResetBoard();

    mHistory.push_back(mBoard); // initial board
    
    mpPGNWin->FromPGN(pgn);

    size_t nMove = 1;
    bool bDone = false;
    while (!bDone && nMove < pgn.GetMoveCount())
    {
        ZPGNSANEntry& move = pgn.mMoves[nMove];

        if (move.IsGameResult(kWhite))
        {
            cout << "Final result:" << pgn.GetTag(kResultTag) << "\n";
            bDone = true;
            break;
        }
        else
        {
            //cout << "Before move - w:[" << move.whiteAction << "] b:" << move.blackAction << "\n";
            //mBoard.DebugDump();

            ZPoint dst = move.DestFromAction(kWhite);
            ZASSERT(mBoard.ValidCoord(dst));

            ZPoint src = move.ResolveSourceFromAction(kWhite, mBoard);
            ZASSERT(mBoard.ValidCoord(src));

            bool bRet;

            char promotion = move.IsPromotion(kWhite);
            if (promotion)
                bRet = mBoard.PromotePiece(src, dst, promotion);
            else
                bRet = mBoard.MovePiece(src, dst, true);
            ZASSERT(bRet);

            mHistory.push_back(mBoard);

            if (move.IsGameResult(kBlack))
            {
                cout << "Final result:" << pgn.GetTag(kResultTag) << "\n";
                bDone = true;
                break;
            }
            else
            {

                //cout << "Before move - w:" << move.whiteAction << " b:[" << move.blackAction << "]\n";
                //mBoard.DebugDump();

                ZPoint dst = move.DestFromAction(kBlack);
                ZASSERT(mBoard.ValidCoord(dst));

                ZPoint src = move.ResolveSourceFromAction(kBlack, mBoard);
                ZASSERT(mBoard.ValidCoord(src));

                char promotion = move.IsPromotion(kBlack);
                if (promotion)
                    bRet = mBoard.PromotePiece(src, dst, promotion);
                else
                    bRet = mBoard.MovePiece(src, dst, true);
                ZASSERT(bRet);

                mHistory.push_back(mBoard);
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

