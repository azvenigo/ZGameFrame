#include "ZMainWin.h"
#include "ZWinFolderSelector.h"
#include "ZGUIStyle.h"
#include "helpers/Registry.h"
#include "Resources.h"
#include "ZWinFormattedText.h"

using namespace std;

ZWinFolderLabel::ZWinFolderLabel()
{
    mBehavior = kNone;
    mStyle = gDefaultDialogStyle;
    mStyle.pos = ZGUI::LC;
    mbAcceptsCursorMessages = true;
}

bool ZWinFolderLabel::Init()
{
    SizeToPath();
    OnMouseOut();
    return ZWin::Init();
}

bool ZWinFolderLabel::Paint() 
{
    if (!PrePaintCheck()) return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    if (ARGB_A(mStyle.bgCol) > 0x0f)
    {
        mpSurface->Fill(mStyle.bgCol);
    }
    else
    {
        mpSurface->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, mAreaLocal, ZBuffer::kEdgeBltMiddle_Stretch | ZBuffer::kEdgeBltSides_Stretch);
    }

    string sPath(VisiblePath());
    ZRect r(mStyle.Font()->StringRect(sPath));
    r = ZGUI::Arrange(r, mAreaLocal, ZGUI::LC, mStyle.paddingH, mStyle.paddingV);

    string sVisiblePath(VisiblePath());

    mStyle.Font()->DrawText(mpSurface.get(), VisiblePath(), r);

    if (mrMouseOver.Area() > 0)
    {
        mpSurface->FillAlpha(0x88FFFFFF, &mrMouseOver);
        ZGUI::ZTextLook look(ZGUI::ZTextLook::kShadowed, gDefaultHighlight, gDefaultHighlight);
        mStyle.Font()->DrawText(mpSurface.get(), mMouseOverSubpath.string(), r, &look);
        //        mpSurface->DrawRectAlpha(0x88ffffff, mrMouseOver);
    }

    return ZWin::Paint();
}

bool ZWinFolderLabel::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
    if (sType == "set_path")
    {
        mCurPath = message.GetParam("folder");
        return true;
    }
    else if (sType == "scan")
    {
        ZRect rList(mAreaInParent.left, mAreaInParent.bottom, grFullArea.right / 2, grFullArea.bottom/2);
        ZWinFolderSelector* pWin = ZWinFolderSelector::Show(this, rList);

        mCurPath = message.GetParam("folder");
        pWin->ScanFolder(mCurPath.string());
        return true;
    }
    return ZWin::HandleMessage(message);
}

void ZWinFolderLabel::SizeToPath()
{
    ZRect rPathRect(PathRect());
    ZRect rExpanded(mAreaInParent);
    rExpanded.right = rExpanded.left + rPathRect.Width();
    SetArea(rExpanded);
    Invalidate();
}


bool ZWinFolderLabel::OnMouseIn()
{
    ZRect rPathRect(PathRect());
    ZRect rExpanded(mAreaInParent);
    rExpanded.right = rExpanded.left + rPathRect.Width();
    SetArea(rExpanded);

    return ZWin::OnMouseIn();
}

bool ZWinFolderLabel::OnMouseOut()
{
    mrMouseOver.SetRect(0, 0, 0, 0);
    SizeToPath();

    return ZWin::OnMouseOut();
}


bool ZWinFolderLabel::OnMouseDownL(int64_t x, int64_t y)
{
    if ((mBehavior&kSelectable)!=0)
        gMessageSystem.Post("scan", this, "folder", mMouseOverSubpath.string());

    return ZWin::OnMouseDownL(x, y);
}

bool ZWinFolderLabel::OnMouseMove(int64_t x, int64_t y)
{
    if ((mBehavior & kSelectable) != 0)
    {
        ZRect rPathArea(PathRect());

        std::filesystem::path subPath;
        MouseOver(x - rPathArea.left, subPath, mrMouseOver);
        if (mMouseOverSubpath != subPath)
        {
            ZDEBUG_OUT("subPath:", subPath, "\n");
            mMouseOverSubpath = subPath;
            Invalidate();
        }
    }

    return ZWin::OnMouseMove(x, y);
}

std::string ZWinFolderLabel::VisiblePath()
{
    string sPath(mCurPath.string());

    if (mAreaAbsolute.PtInRect(gInput.lastMouseMove) || (mBehavior&kCollapsable)==0)  // if not collapsable, always full path
        return sPath;



    int64_t nAvailWidth = mAreaLocal.Width() - mStyle.paddingH * 2;
    tZFontPtr pFont = mStyle.Font();
    int64_t nFullWidth = pFont->StringWidth(sPath);

    // Whole thing fit?
    if (nFullWidth < nAvailWidth)
        return sPath;

    // if path containes fewer than two subdirectories, return the whole thing
    size_t separators = std::count(sPath.begin(), sPath.end(), '\\');
    if (separators < 4)
        return sPath;

    return mCurPath.root_path().string() + "...\\" + mCurPath.filename().string();
}

ZRect ZWinFolderLabel::PathRect()
{
    return ZRect(mStyle.paddingH*2+mStyle.Font()->StringWidth(VisiblePath()), mStyle.paddingV*2+mStyle.fp.Height());
}














void ZWinFolderLabel::MouseOver(int64_t x, std::filesystem::path& subPath, ZRect& rArea)
{
    ZRect r(PathRect());
    r.DeflateRect(mStyle.paddingH, mStyle.paddingV);
    r.bottom -= mStyle.paddingV;
    tZFontPtr pFont = mStyle.Font();



    string sPath(mCurPath.string());
    int64_t fullW = pFont->StringWidth(sPath);

    int64_t startW = 0;
    int64_t endW = 0;
    size_t slash = sPath.find('\\');
    do
    {
        endW = pFont->StringWidth(sPath.substr(0, slash - 1)) + mStyle.paddingH * 2;
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


const string kFolderSelectorName("FolderSelector");



ZWinFolderSelector::ZWinFolderSelector()
{
    mStyle = gDefaultFormattedDocStyle;
    mStyle.look.colTop = 0xffffffff;
    mStyle.look.colBottom = 0xffffffff;
    mpOpenFolderBtn = nullptr;
    mpFolderList = nullptr;
    mbAcceptsCursorMessages = true;
    msWinName = kFolderSelectorName;
}

bool ZWinFolderSelector::Init()
{
    if (!mbInitted)
    {
        string sAppPath = gRegistry["apppath"];

        mBackground.reset(new ZBuffer());
        mBackground->Init(grFullArea.Width(), grFullArea.Height());
        mpParentWin->RenderToBuffer(mBackground, grFullArea, grFullArea, this);
        mBackground->FillAlpha(0xbb000000); // dim


        mpFolderList = new ZWinFormattedDoc();
        mpFolderList->mStyle = mStyle;
        mpFolderList->SetBehavior(ZWinFormattedDoc::kBackgroundFromParent|ZWinFormattedDoc::kScrollable, true);
//        mpFolderList->mStyle.bgCol = 0x88550000;
//        ZRect rList(gM, grFullArea.Height() / 2, grFullArea.right - gM, grFullArea.bottom - gM);
        mpFolderList->SetArea(mrList);
        ChildAdd(mpFolderList, false);


        mpOpenFolderBtn = new ZWinBtn();
        mpOpenFolderBtn->mSVGImage.Load(sAppPath + "/res/openfolder.svg");
        mpOpenFolderBtn->msButtonMessage = ZMessage("{openfolder}", this);
        mpOpenFolderBtn->msTooltip = "Open Folder";
        ChildAdd(mpOpenFolderBtn);
        int64_t nSide = gM;
        ZRect rBtn(nSide, nSide);
        rBtn = ZGUI::Arrange(rBtn, mrList, ZGUI::RB, mStyle.paddingH, mStyle.paddingV);
        mpOpenFolderBtn->SetArea(rBtn);
        mpOpenFolderBtn->SetVisible();

    }

    return ZWin::Init();
}

bool ZWinFolderSelector::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> transformSurfaceLock(mpSurface.get()->GetMutex());

    assert(mpSurface->GetArea() == grFullArea);
    assert(mBackground->GetArea() == grFullArea);
    mpSurface->Blt(mBackground.get(), grFullArea, grFullArea);

//    mpSurface->BltEdge(gDefaultDialogBackground.get(), grDefaultDialogBackgroundEdgeRect, grFullArea, ZBuffer::kEdgeBltMiddle_Stretch| ZBuffer::kEdgeBltSides_Stretch);

    return ZWin::Paint();
}


bool ZWinFolderSelector::OnMouseDownL(int64_t x, int64_t y)
{
    gMessageSystem.Post("kill_child", "name", msWinName);
    return ZWin::OnMouseDownL(x, y);
}


ZWinFolderSelector* ZWinFolderSelector::Show(ZWinFolderLabel* pLabel, ZRect& rList)
{
    // only one dialog
    ZWinFolderSelector* pWin = (ZWinFolderSelector*)gpMainWin->GetChildWindowByWinNameRecursive(kFolderSelectorName);
    if (pWin)
    {
        pWin->SetVisible();
        return pWin;
    }

    pWin = new ZWinFolderSelector();
    pWin->mrList = rList;
    pWin->mpLabel = pLabel;
    pWin->SetArea(grFullArea);

    ZWin* pGrandparent = pLabel->GetParentWin();

    pGrandparent->ChildAdd(pWin);
    pGrandparent->ChildToFront(pLabel); // bring it to the front

    return pWin;
}


bool ZWinFolderSelector::ScanFolder(const std::filesystem::path& folder)
{
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
                sLine += "<text link=" + ZMessage("{scan;folder=" + string(1, letter) + ":\\", mpLabel).ToString() + "}> [" + string(1, letter) + ":]</text>";
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
                sLine = "<line><text link=" + ZMessage("{scan;folder=" + entry.path().string(), this).ToString() + "}>" + entry.path().filename().string() + "</text></line>";
                mpFolderList->AddLineNode(sLine);
            }
        }
        catch (const std::exception& ex)
        {
            std::cerr << "Error processing " << entry.path() << ": " << ex.what() << std::endl;
        }
    }

    mpFolderList->SetVisible();
    mpFolderList->Invalidate();

    return false;
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
