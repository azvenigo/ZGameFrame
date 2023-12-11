#include "ZWinFolderSelector.h"
#include "ZGUIStyle.h"
#include "helpers/Registry.h"
#include "Resources.h"
#include "ZWinFormattedText.h"

using namespace std;

ZWinFolderSelector::ZWinFolderSelector()
{
    mStyle = gDefaultDialogStyle;
    mpOpenFolderBtn = nullptr;
    mpFolderList = nullptr;
    mState = kCollapsed;
    mbAcceptsCursorMessages = true;
}

bool ZWinFolderSelector::Init()
{
    if (!mbInitted)
    {
        string sAppPath = gRegistry["apppath"];

        mpOpenFolderBtn = new ZWinSizablePushBtn();
        mpOpenFolderBtn->mSVGImage.Load(sAppPath + "/res/openfolder.svg");
        mpOpenFolderBtn->SetMessage(ZMessage("openfolder", this));
        mpOpenFolderBtn->msTooltip = "Open Folder";
        ChildAdd(mpOpenFolderBtn);

        if (!mpFolderList)
        {
            mpFolderList = new ZWinFormattedDoc();
            mpFolderList->mStyle = mStyle;
            ChildAdd(mpFolderList, false);
            mpFolderList->Set(ZWinFormattedDoc::kBackgroundFromParent, true);
        }
    }

    return ZWin::Init();
}

bool ZWinFolderSelector::Paint()
{
    if (!mpSurface)
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    if (!mbVisible || !mbInvalid)
        return false;

    if (ARGB_A(mStyle.bgCol) > 0x0f)
    {
        mpSurface->Fill(mStyle.bgCol);
    }
    else
    {
        mpSurface->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mAreaLocal, ZBuffer::kEdgeBltMiddle_Stretch| ZBuffer::kEdgeBltSides_Stretch);
    }

    ZRect r = PathArea();
    mStyle.Font()->DrawText(mpSurface.get(), VisiblePath(), r);

    if ((mState == kExpanded || mState == kFullPath) && mrMouseOver.Area() > 0)
    {
        mpSurface->FillAlpha(0x88FFFFFF, &mrMouseOver);
        mStyle.Font()->DrawText(mpSurface.get(), mMouseOverSubpath.string(), r, &ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, gDefaultHighlight, gDefaultHighlight));
//        mpSurface->DrawRectAlpha(0x88ffffff, mrMouseOver);
    }

    return ZWin::Paint();
}

void ZWinFolderSelector::SetArea(const ZRect& newArea)
{
    ZRect r(newArea);
    r.DeflateRect(mStyle.paddingH, mStyle.paddingV);

    if (mpFolderList && mpFolderList->mbVisible)
    {
        ZRect rList(PathArea());
        rList.OffsetRect(gM*2, rList.Height() + gSpacer);
        rList.bottom = grFullArea.Height()/2;
        mpFolderList->SetArea(rList);
    }

    ZWin::SetArea(newArea);
}


bool ZWinFolderSelector::OnMouseIn()
{
    if (mState == kCollapsed)
        SetState(kFullPath);

    mIdleSleepMS = 100;
    mWorkToDoCV.notify_one();

    return ZWin::OnMouseIn();
}

bool ZWinFolderSelector::Process()
{
    if (mState == kExpanded || mState == kFullPath)
    {
        if (!mAreaAbsolute.PtInRect(gInput.lastMouseMove))
        {
            ZDEBUG_OUT("Out of area\n");
            mIdleSleepMS = 1000;
            SetState(kCollapsed);

            Invalidate();
        }
    }

    return ZWin::Process();
}



bool ZWinFolderSelector::OnMouseDownL(int64_t x, int64_t y)
{
    if (!mMouseOverSubpath.empty())
    {
        ScanFolder(mMouseOverSubpath);
    }
    return ZWin::OnMouseDownL(x, y);
}

bool ZWinFolderSelector::OnMouseWheel(int64_t x, int64_t y, int64_t nDelta)
{
    return ZWin::OnMouseWheel(x, y, nDelta);
}

bool ZWinFolderSelector::OnMouseMove(int64_t x, int64_t y)
{
    if (mState == kFullPath || mState == kExpanded)
    {
        ZRect rPathArea(PathArea());

        std::filesystem::path subPath;
        MouseOver(x- rPathArea.left, subPath, mrMouseOver);
        if (mMouseOverSubpath != subPath)
        {
            ZDEBUG_OUT("subPath:", subPath, "\n");
            mMouseOverSubpath = subPath;
            Invalidate();
        }
    }

    return ZWin::OnMouseMove(x, y);
}

std::string ZWinFolderSelector::VisiblePath()
{
    if (mState == kFullPath || mState == kExpanded)
        return mCurPath.string();

    return mCurPath.root_path().string() + "...\\" + mCurPath.filename().string();
}

ZRect ZWinFolderSelector::VisibleArea()
{
    tZFontPtr pFont = mStyle.Font();

    ZRect rArea;
    int64_t w = pFont->StringWidth(VisiblePath()) + mStyle.paddingH * 2;
    rArea.SetRect(mAreaLocal);

    rArea.right = rArea.left + w;

    if (mState == kExpanded)
    {
        if (mpFolderList)
        {
            rArea.bottom = rArea.top + mpFolderList->GetFullDocumentHeight() + gM * 2;
            if (rArea.bottom > grFullArea.bottom)
                rArea.bottom = grFullArea.bottom;
            if (rArea.Width() < grFullArea.Width() / 3)
                rArea.right = rArea.left + grFullArea.Width() / 3;
        }
        else
            rArea.bottom = rArea.top + grFullArea.Height() / 2;
    }
    else
        rArea.bottom = rArea.top + gM;


    return rArea;
}

ZRect ZWinFolderSelector::PathArea()
{
    ZRect r(mAreaLocal.Width(), mStyle.fp.nHeight);
//    r.DeflateRect(mStyle.paddingH, mStyle.paddingV);
    return r;
}

bool ZWinFolderSelector::ScanFolder(const std::filesystem::path& folder)
{
    SetState(kExpanded);
    mrMouseOver.SetRect(0, 0, 0, 0);
    mCurPath = folder;

    mpFolderList->Clear();

    string sLine;

#ifdef _WIN64

    if (folder.string().length() <= 3)
    {
        DWORD drives = GetLogicalDrives();

        sLine = "<line><text>Drives:</text>";

        for (char letter = 'A'; letter <= 'Z'; ++letter)
        {
            if (drives & 1)
            {
//                std::cout << "Drive: " << letter << ":\\" << std::endl;
                sLine += "<text link=" + ZMessage("scan;folder=" + string(1, letter) + ":\\", this).ToString() + "> [" + string(1, letter) + ":]</text>";
                //mpFolderList->AddMultiLine(string(1, letter) + ":\\" , mStyle, );
            }
            drives >>= 1;
        }

        sLine += "</line>";
        mpFolderList->AddLineNode(sLine);
    }

#endif

    for (const auto& entry : std::filesystem::directory_iterator(folder))
    {
        try
        {
            if (std::filesystem::is_directory(entry.status()))
            {
                std::cout << entry.path() << std::endl;
                sLine = "<line><text link=" + ZMessage("scan;folder=" + entry.path().string(), this).ToString() + ">" + entry.path().filename().string() + "</text></line>";
                mpFolderList->AddLineNode(sLine);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Error processing " << entry.path() << ": " << ex.what() << std::endl;
        }
    }

    if (mpOpenFolderBtn)
        mpOpenFolderBtn->SetVisible();

    ZRect r(VisibleArea());
    r.OffsetRect(mAreaInParent.TL());
    SetArea(r);


    mpFolderList->SetVisible();
    mpFolderList->Invalidate();

    return false;
}

void ZWinFolderSelector::SetState(eState state)
{
    if (mState != state)
    {
        mState = state;

        ZRect r = VisibleArea();
        r.OffsetRect(mAreaInParent.TL());
        SetArea(r);
    }

    if (mState == kExpanded)
    {
        ZRect rList(PathArea());
        rList.OffsetRect(0, rList.Height() + gSpacer);
        rList.bottom = grFullArea.Height() / 2 - gM*4;
        mpFolderList->SetArea(rList);

        int64_t nSide = gM;
        ZRect rBtn(nSide, nSide);
        rBtn = ZGUI::Arrange(rBtn, rList, ZGUI::RB, mStyle.paddingH, mStyle.paddingV);
        mpOpenFolderBtn->SetArea(rBtn);
        mpOpenFolderBtn->SetVisible();
    }
    else
    {
        if (mpFolderList && mpFolderList->mbVisible)
            mpFolderList->SetVisible(false);
        if (mpOpenFolderBtn)
            mpOpenFolderBtn->SetVisible(false);
    }
}


bool ZWinFolderSelector::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "scan")
    {
        ScanFolder(message.GetParam("folder"));
        return true;
    }
    return ZWin::HandleMessage(message);
}

void ZWinFolderSelector::MouseOver(int64_t x, std::filesystem::path& subPath, ZRect& rArea)
{
    ZRect r(PathArea());
    tZFontPtr pFont = mStyle.Font();



    string sPath(mCurPath.string());
    int64_t fullW = pFont->StringWidth(sPath);

    int64_t startW = 0;
    int64_t endW = 0;
    size_t slash = sPath.find('\\');
    do
    {
        endW = pFont->StringWidth(sPath.substr(0, slash-1)) + mStyle.paddingH * 2;
        if (x < endW)
        {
            subPath = sPath.substr(0, slash);
            rArea.SetRect(r.left + startW, r.top, r.left + endW, r.bottom);
            return;
        }
        startW = endW;
        slash = sPath.find('\\', slash + 1);

    } while (slash != string::npos);

    subPath = mCurPath;
    rArea.SetRect(r.left + startW, r.top, r.right, r.bottom);
}


/*
int64_t ZWinFolderSelector::ComputeTailVisibleChars(int64_t nWidth)
{
    string sPath = mCurPath.string();

    tZFontPtr pFont = mStyle.Font();

    for (int64_t nChars = 1; nChars < sPath.length(); nChars++)
    {
        if (pFont->StringWidth(sPath.substr(sPath.length() - nChars)) > nWidth)
            return nChars-1;
    }
    
    return sPath.length();
}

std::string ZWinFolderSelector::VisiblePath()
{
    int64_t nAvailWidth = mAreaLocal.Width() - mStyle.paddingH * 2;
    string sPath = mCurPath.string();
    tZFontPtr pFont = mStyle.Font();
    int64_t nFullWidth = pFont->StringWidth(sPath);

    // Whole thing fit?
    if (nFullWidth < nAvailWidth)
        return sPath;

    // Try root path + "..." + child path
    // example: C:\...\photos

    string sRoot = mCurPath.root_path().string() + "...";
    string sChildFolder = mCurPath.filename().string();
    int64_t nRootWidth = pFont->StringWidth(sRoot);
    int64_t nChildWidth = pFont->StringWidth(sChildFolder);

    if (nRootWidth + nChildWidth <= nAvailWidth)
    {
        int64_t nRemainingWidth = nAvailWidth - nRootWidth;
        return sRoot + sPath.substr(sPath.length() - ComputeTailVisibleChars(nRemainingWidth));
    }
     
    // just return the tail of what fits
    return string(sPath.substr(sPath.length() - ComputeTailVisibleChars(nAvailWidth)));
}
*/