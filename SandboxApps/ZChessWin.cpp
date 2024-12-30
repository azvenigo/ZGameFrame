#include "ZChessWin.h"
#include "Resources.h"
#include "ZWinControlPanel.h"
#include "ZWinWatchPanel.h"
#include "ZStringHelpers.h"
#include "ZWinFileDialog.h"
#include "ZWinBtn.H"
#include "helpers/RandHelpers.h"
#include "helpers/LoggingHelpers.h"
#include "helpers/Registry.h"
#include <filesystem>
#include <fstream>
#include "ZTimer.h"
#include "ZAnimator.h"
#include "ZAnimObjects.h"
#include "ZGUIHelpers.h"
#include "ZWinPaletteDialog.h"

using namespace std;
using namespace lunasvg;


ZChoosePGNWin::ZChoosePGNWin()
{
    msWinName = "choosepgnwin";
    mbAcceptsCursorMessages = true;
    mIdleSleepMS = 250;
    mFillColor = gDefaultDialogFill;
}

bool ZChoosePGNWin::Init()
{
    size_t nButtonWidth = gM*4;
    size_t nButtonHeight = gM;

    ZRect rControl(gSpacer, gSpacer, mAreaLocal.Width() - gSpacer, gSpacer + nButtonHeight);

    ZWinLabel* pLabel = new ZWinLabel();
    pLabel->msText = "Filter";
    pLabel->SetArea(rControl);
    pLabel->mStyle.pos = ZGUI::LB;
    ChildAdd(pLabel);

    rControl.OffsetRect(0, rControl.Height());


    ZWinTextEdit* pEdit = new ZWinTextEdit(&msFilter);
    pEdit->SetArea(rControl);
    pEdit->mStyle = gDefaultWinTextEditStyle;
    pEdit->mStyle.fp.nScalePoints = 1000;
    pEdit->mStyle.pos = ZGUI::LC;
    ChildAdd(pEdit);
    pEdit->SetFocus();

    ZWinBtn* pBtn;

    ZRect rButton(0, 0, nButtonWidth, nButtonHeight);
    rButton.OffsetRect(gSpacer, mAreaLocal.Height() - gSpacer - rButton.Height());



    ZGUI::Style btnStyle(gStyleButton);
    btnStyle.fp.nScalePoints = 500;


    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "Random Game";
    pBtn->mCaption.style = btnStyle;
    pBtn->SetArea(rButton);
    //pBtn->SetMessage("cancelpgnselect;target=chesswin");
    pBtn->msButtonMessage = ZMessage("randgame", this);
    ChildAdd(pBtn);





    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "Cancel";
    pBtn->mCaption.style = btnStyle;
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    //pBtn->SetMessage("cancelpgnselect;target=chesswin");
    pBtn->msButtonMessage = ZMessage("cancelpgnselect", mpParentWin);
    ChildAdd(pBtn);

    ZRect rListbox(gSpacer, gSpacer + rControl.bottom, mAreaLocal.Width() - gSpacer, rButton.top - gSpacer);

    mpGamesList = new ZWinFormattedDoc();
    mpGamesList->SetArea(rListbox);
    mpGamesList->mStyle.bgCol = gDefaultTextAreaFill;
    mpGamesList->SetBehavior(ZWinFormattedDoc::kBackgroundFill, true);
    mpGamesList->SetBehavior(ZWinFormattedDoc::kDrawBorder, true);
    ChildAdd(mpGamesList);

    return ZWin::Init();
}

bool ZChoosePGNWin::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface->Fill(mFillColor);

    ZRect rText(mAreaLocal);
    rText.OffsetRect(gSpacer, gSpacer);
    gStyleCaption.Font()->DrawTextParagraph(mpSurface.get(), msCaption, rText);
    return ZWin::Paint();
}

void ZChoosePGNWin::SelectEntry(size_t index)
{
    ZASSERT(index < mPGNEntries.size());

    if (index < mPGNEntries.size())
    {
        auto findIt = mPGNEntries.begin();
        while (index-- > 0)
            findIt++;

        string sPGNContent((*findIt).second);
        gMessageSystem.Post(ZMessage("{setpgn;contents=" + SH::URL_Encode(sPGNContent) + ";target=" + mpParentWin->GetTargetName() + "}"));
    }
}

bool ZChoosePGNWin::Process()
{
    if (msFilter != msLastFiltered)
    {
        msLastFiltered = msFilter;
        RefreshList();
    }
    return ZWin::Process();
}


bool ZChoosePGNWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "selectpgn")
    {
        SelectEntry((size_t) SH::ToInt(message.GetParam("index")));
        return true;
    }
    else if (sType == "randgame")
    {
        SelectEntry(RANDU64(0, mPGNEntries.size()));
        return true;
    }
    return ZWin::HandleMessage(message);
}

bool ZChoosePGNWin::ListGamesFromPGN(string& sFilename, string& sPGNFile)
{
    std::filesystem::path pgnFile(sFilename);
   

    // count number of games in text
    size_t nIndex = 0;
    size_t nCount = 0;
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

            string sCaption(SH::FromInt(nCount + 1) + ". " + sDate + "/" + sEventName + "/" + sSiteName + "/" + sRound);
            string sPGNContent = sPGNFile.substr(nIndex, nEndPGNContent - nIndex);

            mPGNEntries.push_back(tPGNEntry(sCaption, sPGNContent));

            nIndex = nEndPGNContent;    // length of "[Event"
            nCount++;
        }
    } while (nIndex != string::npos);

    RefreshList();
    return true;
}

void ZChoosePGNWin::RefreshList()
{
    mpGamesList->Clear();

    ZFontParams fp = gDefaultTextFont;
    fp.nScalePoints = 500;

    size_t nCount = 0;
    for (auto entry : mPGNEntries)
    {
        //string sListBoxEntry = "<line wrap=0><text color=0xff000000 color2=0xff000000 fontparams=" + SH::URL_Encode(gDefaultTextFont) + " position=lb link={selectpgn;index=" + SH::FromInt(nCount) + ";target=" + GetTargetName() + ">" + entry.first + "</text></line>";
        string sListBoxEntry = "<line wrap=0><text color=0xff000000 color2=0xff000000 fontparams=" + SH::URL_Encode(fp) + " position=lb link={selectpgn;index=" + SH::FromInt(nCount) + ";target=" + GetTargetName() + "}>" + entry.first + "</text></line>";
        nCount++;

        if ( msFilter.empty() ||  SH::Contains(entry.first, msFilter, false) || SH::Contains(entry.second, msFilter, false)) // if empty filter or either caption or pgn contents include filter text (case insensitive)
            mpGamesList->AddLineNode(sListBoxEntry);
    }

    msCaption = "Select from " + SH::FromInt(mPGNEntries.size()) + " games";

    mpGamesList->SetScrollable();
    InvalidateChildren();
}



ZPGNWin::ZPGNWin()
{
    msWinName = "pgnwin";
    mbAcceptsCursorMessages = true;
    mIdleSleepMS = 500;
    mFillColor = gDefaultDialogFill;
}

bool ZPGNWin::Init()
{
    mMoveFont.sFacename = "Verdana";
    mMoveFont.nScalePoints = 1200;
    mMoveFont.nWeight = 200;

    mTagsFont.sFacename = "Verdana";
    mTagsFont.nScalePoints = 600;

    mBoldFont = mMoveFont;
    mBoldFont.nWeight = 800;


    ZRect rGameTags(gSpacer, gSpacer, mAreaLocal.Width() - gSpacer, mAreaLocal.Height() / 2 - gSpacer);

    mpGameTagsWin = new ZWinFormattedDoc();
    mpGameTagsWin->SetArea(rGameTags);
    mpGameTagsWin->mStyle.bgCol = gDefaultTextAreaFill;
    mpGameTagsWin->SetBehavior(ZWinFormattedDoc::kDrawBorder|ZWinFormattedDoc::kBackgroundFill, true);

    ChildAdd(mpGameTagsWin); 



    ZWinBtn* pBtn;

    ZGUI::Style wd = ZGUI::Style(ZFontParams("Wingdings", gStyleButton.fp.nScalePoints, 200, 0, 0, false, true), {}, ZGUI::C);
    ZGUI::Style wd3 = ZGUI::Style(ZFontParams("Wingdings 3", gStyleButton.fp.nScalePoints, 200, 0, 0, false, true), {}, ZGUI::C);

    size_t nButtonSlots = 10;
    size_t nButtonSize = (mAreaLocal.Width() - gSpacer *2) / nButtonSlots;

    ZRect rButton(0, 0, nButtonSize, nButtonSize);
    rButton.OffsetRect(gSpacer, mAreaLocal.Height() - gSpacer - rButton.Height());

    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = ")"; // wingdings 3 to the beggining
    pBtn->mCaption.style = wd3;
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("beginning", this);
    ChildAdd(pBtn);

    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "v"; // back one
    pBtn->mCaption.style = wd3;
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("backone", this);
    ChildAdd(pBtn);

    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "w"; // forward one
    pBtn->mCaption.style = wd3;
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("forwardone", this);
    ChildAdd(pBtn);

    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "*"; // to end
    pBtn->mCaption.style = wd3;
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("end", this);
    ChildAdd(pBtn);

    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "5"; // file cabinet
    pBtn->mCaption.style = wd;
    rButton.OffsetRect(rButton.Width() * 2, 0);
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("chessdb", mpParentWin);
    ChildAdd(pBtn);


    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "1"; // open file
    pBtn->mCaption.style = wd;
    rButton.OffsetRect(rButton.Width() *2, 0);
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("loadgame", mpParentWin);
    ChildAdd(pBtn);


    pBtn = new ZWinBtn();
    pBtn->mCaption.sText = "<"; // save file
    pBtn->mCaption.style = wd;
    rButton.OffsetRect(rButton.Width(), 0);
    pBtn->SetArea(rButton);
    pBtn->msButtonMessage = ZMessage("savegame", mpParentWin);
    ChildAdd(pBtn);

    ZRect rMoves(rGameTags.left, rGameTags.bottom + gSpacer, rGameTags.right, rButton.top - gSpacer * 2);

    mpMovesWin = new ZWinFormattedDoc();
    mpMovesWin->SetArea(rMoves);
    mpMovesWin->mStyle.bgCol = 0xff444444;
    mpMovesWin->SetBehavior(ZWinFormattedDoc::kDrawBorder| ZWinFormattedDoc::kBackgroundFill, true);

    ChildAdd(mpMovesWin);

    mCurrentHalfMoveNumber = 0;

    return ZWin::Init();
}


bool ZPGNWin::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface->Fill(mFillColor);


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

bool ZPGNWin::AddAction(const std::string& sAction, size_t nHalfMoveNumber)
{
    mPGN.TrimToHalfMove(nHalfMoveNumber);
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
    mPGN.Reset();
    InvalidateChildren();
    return true;
}


void ZPGNWin::UpdateView()
{
    mpGameTagsWin->Clear();

    string sTag;

    // first add all of the known tags in order
    const int knTags = sizeof(pgnTags)/sizeof(string);
    for (int nTagIndex = 0; nTagIndex < knTags; nTagIndex++)
    {
        string sValue = mPGN.GetTag(pgnTags[nTagIndex]);
        
        if (!sValue.empty())
        {
            sTag = "<line wrap=0><text color=0xffffffff color2=0xffffffff fontparams=" + SH::URL_Encode(mTagsFont) + " position=lb>" + pgnTags[nTagIndex] + "</text><text fontparams=" + SH::URL_Encode(mTagsFont) + " color=0xff000000 color2=0xff000000 position=rb>" + sValue + "</text></line>";
            mpGameTagsWin->AddLineNode(sTag);
        }
    }

    // now add the unknown tags
    for (auto tagpair : mPGN.mTags)
    {
        if (!mPGN.IsKnownTag(tagpair.first))
        {
            sTag = "<line wrap=0><text color=0xffbbbbbb color2=0xffbbbbbb fontparams=" + SH::URL_Encode(mTagsFont) + " position=lb>" + tagpair.first + "</text><text fontparams=" + SH::URL_Encode(mTagsFont) + " color=0xff333333 color2=0xff333333 position=rb>" + tagpair.second + "</text></line>";
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
                sMoveLine = "<line wrap=0><text fontparams=" + SH::URL_Encode(mBoldFont) + " color=0xff0088ff color2=0xff0088ff position=lb link={setmove;target=pgnwin;halfmove=" + SH::FromInt(nHalfMove - 1) + "}>" + SH::FromInt(nMove) + ". [" + move.whiteAction + "]</text>";
                sMoveLine += "<text fontparams=" + SH::URL_Encode(mBoldFont) + " color=0xff000000 color2=0xff000000 position=rb link={setmove;target=pgnwin;halfmove=" + SH::FromInt(nHalfMove) + "}>" + move.blackAction + "</text></line>";

                if (!move.whiteComment.empty())
                    gMessageSystem.Post(ZMessage("statusmessage", mpParentWin, "message", SH::FromInt(nMove) + ". [" + move.whiteAction + "] " + move.whiteComment, "col", 0xff0066aa));

            }
            else
            {
                // highlight black
                sMoveLine = "<line wrap=0><text fontparams=" + SH::URL_Encode(mBoldFont) + " color=0xffffffff color2=0xffffffff position=lb link={setmove;target=pgnwin;halfmove=" + SH::FromInt(nHalfMove - 1) + "}>" + SH::FromInt(nMove) + ". " + move.whiteAction + "</text>";
                sMoveLine += "<text fontparams=" + SH::URL_Encode(mBoldFont) + " color=0xff0088ff color2=0xff0088ff position=rb link={setmove;target=pgnwin;halfmove=" + SH::FromInt(nHalfMove) + "}>[" + move.blackAction + "]</text></line>";

                if (!move.blackComment.empty())
                    gMessageSystem.Post(ZMessage("statusmessage", mpParentWin, "message", SH::FromInt(nMove) + "... [" + move.blackAction + "] " + move.blackComment, "col", 0xff0066aa));

            }
            mpMovesWin->AddLineNode(sMoveLine);
        }
        else
        {

            sMoveLine = "<line wrap=0><text fontparams=" + SH::URL_Encode(mMoveFont) + " color=0xffffffff color2=0xffffffff position=lb link={setmove;target=pgnwin;halfmove="+ SH::FromInt(nHalfMove-1) +"}>" + SH::FromInt(nMove) + ". " + move.whiteAction + "</text>";
            sMoveLine += "<text fontparams=" + SH::URL_Encode(mMoveFont) + " color=0xff000000 color2=0xff000000 position=rb link={setmove;target=pgnwin;halfmove="+SH::FromInt(nHalfMove)+"}>" + move.blackAction + "</text></line>";
            mpMovesWin->AddLineNode(sMoveLine);
        }

        nHalfMove += 2;
    }
//    mpMovesWin->InvalidateChildren();
    mpMovesWin->SetScrollable();
    mpMovesWin->SetBehavior(ZWinFormattedDoc::kUnderlineLinks, false);
    mpMovesWin->ScrollTo(mMoveFont.Height() * (2*(mCurrentHalfMoveNumber/2)-10) / 2);

    InvalidateChildren();
}


bool ZPGNWin::SetHalfMove(int64_t nHalfMove)
{
    if (nHalfMove >= 0 && nHalfMove < (int64_t)mPGN.GetHalfMoveCount())
    {
        mCurrentHalfMoveNumber = nHalfMove;
//        string sMessage;
//        Sprintf(sMessage, "sethistoryindex;target=chesswin;halfmove=%d", mCurrentHalfMoveNumber);
//        gMessageSystem.Post(sMessage);
        gMessageSystem.Post(ZMessage("sethistoryindex", mpParentWin, "halfmove", mCurrentHalfMoveNumber));

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
        SetHalfMove(SH::ToInt(message.GetParam("halfmove")));
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
    mpEditBoardWin = nullptr;
    mpStatusWin = nullptr;
    mbDemoMode = false;
    mnDemoModeNextMoveTimeStamp = 0;
    mAutoplayMSBetweenMoves = (kAutoplayMaxMSBetweenMoves+kAutoplayMinMSBetweenMoves)/2;
}
   

const int kLightSqCol = 0;
const int kDarkSqCol = 1;
const int kWhiteCol = 2;
const int kBlackCol = 3;

bool ZChessWin::Init()
{
    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;
    mIdleSleepMS = 250;
    SetFocus();

    ClearHistory();

    mnPieceHeight = mAreaLocal.Height() / 12;

    if (gRegistry.Contains("chess", "palette"))
    {
        mPalette = ZGUI::Palette(gRegistry.GetValue("chess", "palette"));
    }
    else
    {
        mPalette.mColorMap = ZGUI::tColorMap{   { "light_sq", 0xff99c4dc },     // kLightSqCol
                                                { "dark_sq", 0xff4e5262 },      // kDarkSqCol
                                                { "white_col", 0xffd0d2eb },    // kWhiteCol
                                                { "black_col", 0xff12142f } };  // kBlackCol
        gRegistry.SetDefault("chess", "palette", mPalette);
    }


/*    mpLightSquareCol = &ZGUI::gDefaultPalette.colors[4]; // index 4 is light square....tbd, index or by name?
    mpDarkSquareCol = &ZGUI::gDefaultPalette.colors[5]; 
    mpWhiteCol = &ZGUI::gDefaultPalette.colors[6]; 
    mpBlackCol = &ZGUI::gDefaultPalette.colors[7];
    */

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

        string invalidateMsg(ZMessage("invalidate", this));

        pCP->Init();

        ZGUI::ZTextLook buttonLook(ZGUI::ZTextLook::kEmbossed, 0xff737373, 0xff737373);
        ZGUI::ZTextLook checkedButtonLook(ZGUI::ZTextLook::kEmbossed, 0xff737373, 0xff73ff73);

        ZWin* pEditButton = pCP->Toggle("editmode", &mbEditMode, "Edit Mode", ZMessage("toggleeditmode", this), ZMessage("toggleeditmode", this));


        pCP->AddSpace(panelH / 30);
        pCP->Toggle("demomode", &mbDemoMode, "Demo Mode", invalidateMsg, invalidateMsg);

        pCP->Caption("anim_speed", "Animation Speed");
        pCP->Slider("autoplay", &mAutoplayMSBetweenMoves, kAutoplayMinMSBetweenMoves, kAutoplayMaxMSBetweenMoves, 1);

        ChildAdd(pCP);


        ZRect rEditBoard(rControlPanel);
        rEditBoard.OffsetRect(0, pEditButton->GetArea().bottom + gSpacer);
        mpEditBoardWin = new ZWinControlPanel();
        mpEditBoardWin->SetArea(rEditBoard);
        mpEditBoardWin->Init();

        mpEditBoardWin->mStyle.bgCol = 0xff737373;

        mpEditBoardWin->AddSpace(panelH / 30);

        mpEditBoardWin->Button("clearboard", "Clear", ZMessage("clearboard", this));
        mpEditBoardWin->Button("resetboard", "Reset", ZMessage("resetboard", this));

        mpEditBoardWin->AddSpace(panelH / 30);
        mpEditBoardWin->Button("changeturn", "Change Turn", ZMessage("changeturn", this));

        mpEditBoardWin->AddSpace(panelH / 30);
        mpEditBoardWin->Button("showpalette", "Colors", ZMessage("showpalette", this));

        mpEditBoardWin->AddSpace(panelH / 30);
        mpEditBoardWin->Caption("size", "Size");
        mpEditBoardWin->Slider("pieceheight", &mnPieceHeight, 1, 26, 10, 0.2, ZMessage("updatesize", this), true, false);

        ChildAdd(mpEditBoardWin, false);




        mpPGNWin = new ZPGNWin();
        ZRect rPGNPanel((int64_t)(rControlPanel.Width() * 1.5), (int64_t)(rControlPanel.Height()));
        rPGNPanel.OffsetRect(rControlPanel.left - rPGNPanel.Width() - gSpacer, rControlPanel.top);
        mpPGNWin->SetArea(rPGNPanel);
        ChildAdd(mpPGNWin);

        ZRect rStatusPanel(0, 0, mrBoardArea.Width(), mnPieceHeight);

        //mpStatusWin = new ZWinLabel();
        mpStatusWin = new ZWinTextEdit(&msStatus);
        //mpStatusWin->msText = "Welcome to ZChess";
        msStatus = "Welcome to ZChess";
        mpStatusWin->mStyle = ZGUI::Style(ZFontParams("Ariel", ZFontParams::ScalePoints(mAreaLocal.Height()/28), 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, 0xff555555, 0xffffffff), ZGUI::C, ZGUI::Padding((int32_t)gSpacer, (int32_t)gSpacer), gDefaultTextAreaFill, false);
        mpStatusWin->SetArea(ZGUI::Arrange(rStatusPanel, mrBoardArea, ZGUI::ICOB, (int32_t)gSpacer, (int32_t)gSpacer));
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
    mpSymbolicFont->Init(ZFontParams("MS Gothic", ZFontParams::ScalePoints(mnPieceHeight), 400, mnPieceHeight / 4, true), false, false);
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

    uint32_t whiteCol = mPalette.mColorMap[kWhiteCol].col;
    uint32_t blackCol = mPalette.mColorMap[kBlackCol].col;

//    if (whiteCol == 0xffffffff)
    {
        mPieceData['K'].LoadImage("res/chess/king_b.svg", mnPieceHeight, whiteCol);
        mPieceData['Q'].LoadImage("res/chess/queen_b.svg", mnPieceHeight, whiteCol);
        mPieceData['R'].LoadImage("res/chess/rook_b.svg", mnPieceHeight, whiteCol);
        mPieceData['B'].LoadImage("res/chess/bishop_b.svg", mnPieceHeight, whiteCol);
        mPieceData['N'].LoadImage("res/chess/knight_b.svg", mnPieceHeight, whiteCol);
        mPieceData['P'].LoadImage("res/chess/pawn_b.svg", mnPieceHeight, whiteCol);
    }
/*    else
    {
        // load black piece images (as they are filled in and can be colorized)
        mPieceData['K'].LoadImageA("res/chess/king_b.svg", mnPieceHeight, whiteCol);
        mPieceData['Q'].LoadImageA("res/chess/queen_b.svg", mnPieceHeight, whiteCol);
        mPieceData['R'].LoadImageA("res/chess/rook_b.svg", mnPieceHeight, whiteCol);
        mPieceData['B'].LoadImageA("res/chess/bishop_b.svg", mnPieceHeight, whiteCol);
        mPieceData['N'].LoadImageA("res/chess/knight_b.svg", mnPieceHeight, whiteCol);
        mPieceData['P'].LoadImageA("res/chess/pawn_b.svg", mnPieceHeight, whiteCol);
    }*/
    
    mPieceData['k'].LoadImage("res/chess/king_b.svg", mnPieceHeight, blackCol);
    mPieceData['q'].LoadImage("res/chess/queen_b.svg", mnPieceHeight, blackCol);
    mPieceData['r'].LoadImage("res/chess/rook_b.svg", mnPieceHeight, blackCol);
    mPieceData['b'].LoadImage("res/chess/bishop_b.svg", mnPieceHeight, blackCol);
    mPieceData['n'].LoadImage("res/chess/knight_b.svg", mnPieceHeight, blackCol);
    mPieceData['p'].LoadImage("res/chess/pawn_b.svg", mnPieceHeight, blackCol);

    mrBoardArea.SetRect(0, 0, mnPieceHeight * 8, mnPieceHeight * 8);
    mrBoardArea = mrBoardArea.CenterInRect(mAreaLocal);


    mnPalettePieceHeight = mnPieceHeight * 8 / 12;      // 12 possible pieces drawn over 8 squares

    mrPaletteArea.SetRect(mrBoardArea.right, mrBoardArea.top, mrBoardArea.right + mnPalettePieceHeight, mrBoardArea.bottom);

    if (mpStatusWin)
    {
        ZRect rStatusPanel(0, 0, mrBoardArea.Width(), mnPieceHeight);
        mpStatusWin->SetArea(ZGUI::Arrange(rStatusPanel, mrBoardArea, ZGUI::ICOB, (int32_t)gSpacer, (int32_t)gSpacer));
        mpStatusWin->mStyle = ZGUI::Style(ZFontParams("Ariel", ZFontParams::ScalePoints(mnPieceHeight / 2), 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed), ZGUI::LT, ZGUI::Padding((int32_t)gSpacer, (int32_t)gSpacer), gDefaultTextAreaFill, true);
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
                    mMouseDownOffset.Set(squareOffset.x, squareOffset.x);
                    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
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
                const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
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
        const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());
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
                            mpPGNWin->AddAction(sAction, mBoard.GetHalfMoveNumber());

                        UpdateStatus(sAction);

                        const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
                        mHistory.resize(mBoard.GetHalfMoveNumber());
                        mHistory.push_back(mBoard);
                    }
                }
            }

        }


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
                gMessageSystem.Post(ZMessage("sethistoryindex", this, "halfmove", mBoard.GetHalfMoveNumber()));
            }
        }
    }


    return ZWin::Process();
}

bool ZChessWin::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

    mpSurface->Fill(0xff444444);
    DrawBoard();

    // Draw labels if not in demo mode
    if (!mbDemoMode)
    {
        ZRect rMoveLabel(0,0,mnPieceHeight,mnPieceHeight/2);
        tZFontPtr pLabelFont = gpFontSystem->GetFont(gDefaultTitleFont);
        uint32_t nLabelPadding = (uint32_t)pLabelFont->Height();
        if (mBoard.WhitesTurn())
        {
            string sLabel("White's Move");
            if (!mbEditMode && mBoard.IsKingInCheck(true))
            {
                if (mBoard.IsCheckmate())
                    sLabel = "Checkmate";
                else
                    sLabel = "White is in Check";
            }

            rMoveLabel = pLabelFont->Arrange(rMoveLabel, sLabel, ZGUI::Center);
            rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
            rMoveLabel = ZGUI::Arrange(rMoveLabel, SquareArea(kA1), ZGUI::OLIC, gSpacer, gSpacer);

            mpSurface->Fill(0xffffffff, &rMoveLabel);
            ZGUI::Style style(gDefaultTitleFont, ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xff000000, 0xff000000), ZGUI::Center);
            pLabelFont->DrawTextParagraph(mpSurface.get(), sLabel, rMoveLabel, &style);
        }
        else
        {
            string sLabel("Black's Move");
            if (!mbEditMode && mBoard.IsKingInCheck(false))
            {
                if (mBoard.IsCheckmate())
                    sLabel = "Checkmate";
                else
                    sLabel = "Black is in Check";
            }
            rMoveLabel = pLabelFont->Arrange(rMoveLabel, sLabel, ZGUI::Center);
            rMoveLabel.InflateRect(nLabelPadding, nLabelPadding);
            rMoveLabel = ZGUI::Arrange(rMoveLabel, SquareArea(kA8), ZGUI::OLIC, gSpacer, gSpacer);
            mpSurface->Fill(0xff000000, &rMoveLabel);
            ZGUI::Style style(gDefaultTitleFont, ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xffffffff, 0xffffffff), ZGUI::Center);
            pLabelFont->DrawTextParagraph(mpSurface.get(), sLabel, rMoveLabel, &style);
        }
    }


    if (mpDraggingPiece)
        mpSurface->BltAlpha(mpDraggingPiece.get(), mpDraggingPiece->GetArea(), mrDraggingPiece, 255);
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
        return mPalette.mColorMap[kDarkSqCol].col;

    return mPalette.mColorMap[kLightSqCol].col;
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
    mpSurface->Fill(gDefaultTextAreaFill, &mrPaletteArea);

    ZRect rPalettePiece(mrPaletteArea);
    rPalettePiece.bottom = mrPaletteArea.top + mnPalettePieceHeight;

    for (int i = 0; i < 12; i++)
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(rPalettePiece, verts);

        gRasterizer.RasterizeWithAlpha(mpSurface.get(), mPieceData[mPalettePieces[i]].mpImage.get(), verts);
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
            sMove lastMove = mBoard.GetLastMove();
            if (grid == lastMove.mSrc || grid == lastMove.mDest)
                nSquareColor = COL::AlphaBlend_BlendAlpha(nSquareColor, 0xff0088ff, 128);


            ZRect r(SquareArea(grid));
            mpSurface->Fill(nSquareColor, &r);

            if (mbShowAttackCount)
            {

                string sCount;
                Sprintf(sCount, "%d", mBoard.UnderAttack(true, grid));
                ZRect rText(SquareArea(grid));
                defaultFont->DrawText(mpSurface.get(), sCount, rText);

                rText.OffsetRect(0, defaultFont->Height());

                Sprintf(sCount, "%d",mBoard.UnderAttack(false, grid));
                defaultFont->DrawText(mpSurface.get(), sCount, rText);
            }



            if (grid == mHiddenSquare)  // do not draw piece while animation playing
                continue;


            if (c)
            {
                ZRect rSquare(SquareArea(grid));
                if (mDraggingPiece && mDraggingSourceGrid == grid)
                    mpSurface->BltAlpha(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), rSquare, 64);
                else
                    mpSurface->BltAlpha(mPieceData[c].mpImage.get(), mPieceData[c].mpImage->GetArea(), rSquare, 255);
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
        gMessageSystem.Post("{quit_app_confirmed}");
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
        //mpStatusWin->msText = sText;
        msStatus = sText;
        mpStatusWin->mStyle = ZGUI::Style(ZFontParams("Ariel", ZFontParams::ScalePoints(mnPieceHeight/2), 600), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed), ZGUI::LT, ZGUI::Padding(), gDefaultTextAreaFill, true);
        mpStatusWin->Invalidate();
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
            UpdateStatus(message.GetParam("message"), (uint32_t)SH::ToInt(message.GetParam("col")));
        else
            UpdateStatus(message.GetParam("message"));
        Invalidate();
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
        msStatus.clear();
        InvalidateChildren();
        return true;
    }
    else if (sType == "resetboard")
    {
        mBoard.ResetBoard();
        ClearHistory();
        if (mpPGNWin)
            mpPGNWin->Clear();
        msStatus.clear();
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
    else if (sType == "showpalette")
    {
        ZWinPaletteDialog::ShowPaletteDialog("Board Colors", &mPalette.mColorMap, ZMessage("palette_ok", this), 2);
        return true;
    }
    else if (sType == "palette_ok")
    {
        gRegistry.Set("chess", "palette", mPalette);
        UpdateSize();   // re-renders
        return true;
    }
    else if (sType == "chessdb")
    {
        LoadPGN("res/twic1152.pgn");
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
        string sPGN = SH::URL_Decode(message.GetParam("contents"));
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
/*    else if (sType == "randgame")
    {
        LoadRandomGame();
        return true;
    }*/
    else if (sType == "toggleeditmode")
    {
        mpEditBoardWin->SetVisible(mbEditMode);
        mpPGNWin->SetVisible(!mbEditMode);
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
            grid.x = SH::ToInt(message.GetParam("x"));
            grid.y = SH::ToInt(message.GetParam("y"));

            string sAction = ZPGNSANEntry::ActionFromPromotion(sMove(mDraggingSourceGrid, grid), c, mBoard);
            ZOUT("Action: ", sAction);

            if (mBoard.PromotePiece(mDraggingSourceGrid, grid, c))
            {
                if (mpPGNWin)
                    mpPGNWin->AddAction(sAction, mBoard.GetHalfMoveNumber());
                UpdateStatus(sAction);

                const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
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

        int64_t nHalfMove = 1+SH::ToInt(message.GetParam("halfmove"));   // add 1 since first two history boards are placeholder
        const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
        if (nHalfMove >= 0)
        {
            if (nHalfMove >= (int64_t)mHistory.size())
                nHalfMove = (int64_t)mHistory.size() - 1;


            // remember previous board for animation purposes
            ChessBoard prevBoard = mBoard;
            mBoard = mHistory[nHalfMove];

            sMove move = mBoard.GetLastMove();

            if (mBoard.ValidCoord(move.mSrc) && mBoard.ValidCoord(move.mDest) && (kAutoplayMaxMSBetweenMoves - mAutoplayMSBetweenMoves) > 250)
            {
                int64_t nTransformTime = (kAutoplayMaxMSBetweenMoves-mAutoplayMSBetweenMoves) / 2;
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
                        pImage = new ZAnimObject_TransformingImage(mPieceData[prevPiece].mpImage);
                        pImage->StartTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top)));
                        pImage->AddTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top), 1.0, 0.0, 255, ""), nTransformTime);
                        pImage->SetDestination(mpSurface);
                        mpAnimator->AddObject(pImage);
                    }
                }


                pImage = new ZAnimObject_TransformingImage(mPieceData[mBoard.Piece(move.mDest)].mpImage);
                pImage->StartTransformation(ZTransformation(ZPoint(rSrcSquareArea.left, rSrcSquareArea.top)));
                pImage->AddTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top), 1.0, 0.0, 255, ZMessage("hidesquare", this, "x", -1, "y", -1)), nTransformTime);
                pImage->AddTransformation(ZTransformation(ZPoint(rDstSquareArea.left, rDstSquareArea.top)), 33);    // 33ms to at least cover 30fps
                pImage->SetDestination(mpSurface);
                mHiddenSquare = move.mDest;
                mpAnimator->AddObject(pImage);
            }

            InvalidateChildren();
            return true;
        }
        else
        {
            ZERROR("ERROR: Couldn't load board from history halfmove:", nHalfMove,  " entries:" , mHistory.size());
        }
        return true;
    }
    else if (sType == "hidesquare")
    {
        mHiddenSquare.Set(SH::ToInt(message.GetParam("x")), SH::ToInt(message.GetParam("y")));
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

            ZRect rChoosePGNWin((int64_t)(rControlPanel.Width() * 1.5), rControlPanel.Height());
            rChoosePGNWin.OffsetRect(rControlPanel.left - rChoosePGNWin.Width() - gSpacer, rControlPanel.top);
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


bool ChessPiece::LoadImage(std::string path, int64_t nSize, uint32_t col)
{
    mpImage.reset(new ZBuffer());
    mpImage->Init(nSize, nSize);


    auto doc = lunasvg::Document::loadFromFile(path);

    string sCol(std::format("#{:02X}{:02X}{:02X}", ARGB_R(col), ARGB_G(col), ARGB_B(col)));
    auto element = doc->getElementById("piece");
    element.setAttribute("fill", sCol);

    element = doc->getElementById("base");
    if (!element.isNull())
        element.setAttribute("fill", sCol);

    doc->updateLayout();

    if (!doc)
        return false;
    auto svgbitmap = doc->renderToBitmap((uint32_t)nSize, (uint32_t)nSize); // if the buffer is already initialized, this will render at the same dimensions. If not these will be 0x0 & it will render at the SVG embedded dimensions

    int64_t w = (int64_t)svgbitmap.width();
    int64_t h = (int64_t)svgbitmap.height();
    uint32_t s = svgbitmap.stride();

    for (int64_t y = 0; y < h; y++)
    {
        memcpy(mpImage->mpPixels + y * w, svgbitmap.data() + y * s, w * 4);
    }

    mpImage->mbHasAlphaPixels = true;

    return true;
}

/*
bool ChessPiece::GenerateImageFromSymbolicFont(char c, int64_t nSize, ZDynamicFont* pFont, bool bOutline, uint32_t whiteCol, uint32_t blackCol)
{
    mpImage.reset(new ZBuffer());
    mpImage->Init(nSize, nSize);
    mpImage->mbHasAlphaPixels = true;

    std::string s(&c, 1);

    ZRect rSquare(mpImage->GetArea());

    uint32_t nCol = whiteCol;
    uint32_t nOutline = 0xff000000;
    if (c == 'q' || c == 'k' || c == 'r' || c == 'n' || c == 'b' || c == 'p')
    {
        nCol = blackCol;
        nOutline = 0xffffffff;
    }

    ZGUI::Style style;
    style.look = ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, nCol, nCol);
    style.pos = ZGUI::Center;

    if (bOutline)
    {
        ZGUI::Style outlineStyle(style);
        outlineStyle.look = ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, nOutline, nOutline);


        int64_t nOffset = nSize / 64;
        ZRect rOutline(rSquare);
        rOutline.OffsetRect(-nOffset, -nOffset);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, &outlineStyle);
        rOutline.OffsetRect(nOffset * 2, 0);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, &outlineStyle);
        rOutline.OffsetRect(0, nOffset * 2);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, &outlineStyle);
        rOutline.OffsetRect(-nOffset * 2, 0);
        pFont->DrawTextParagraph(mpImage.get(), s, rOutline, &outlineStyle);
    }

    pFont->DrawTextParagraph(mpImage.get(), s, rSquare, &style);
    return true;
}
*/

void ZChessWin::ClearHistory()
{
    const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);
    mHistory.clear();

    ChessBoard board;
    mHistory.push_back(board); // initial board
    mHistory.push_back(board); // initial board
}



bool ZChessWin::FromPGN(ZChessPGN& pgn)
{
    ClearHistory();

    const std::lock_guard<std::recursive_mutex> lock(mHistoryMutex);

    if (mpPGNWin)
        mpPGNWin->FromPGN(pgn);

    ChessBoard board;
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
                ZASSERT(ChessBoard::ValidCoord(dst));

                ZPoint src = move.ResolveSourceFromAction(kBlack, board);
                ZASSERT(ChessBoard::ValidCoord(src));

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
    int64_t nPieceWidth = mAreaLocal.Width() / 4;
    int64_t nPiece = x/nPieceWidth;
    if (!mDest.y == 0) // promotion on 0 rank is by black
        nPiece += 4;
    gMessageSystem.Post(ZMessage("promote", mpParentWin, "piece", mPromotionPieces[nPiece], "x", mDest.x, "y", mDest.y));
    return true;
}


bool ZPiecePromotionWin::Paint()
{
    if (!PrePaintCheck())
        return false;
    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

//    mpTransformTexture->Fill(mAreaToDrawTo, 0xff4444ff);

    ZRect rPalettePiece(mAreaLocal);
    int32_t nPieceHeight = (int32_t)mAreaLocal.Width() / 4;
    rPalettePiece.right = mAreaLocal.left + nPieceHeight;

    int nBasePieceIndex = 0;
    if (!mDest.y == 0)
        nBasePieceIndex = 4;

    for (int i = nBasePieceIndex; i < nBasePieceIndex+4; i++)
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(rPalettePiece, verts);

        gRasterizer.RasterizeWithAlpha(mpSurface.get(), mpPieceData[mPromotionPieces[i]].mpImage.get(), verts);
        rPalettePiece.OffsetRect(nPieceHeight, 0);
    }

    return ZWin::Paint();
};

