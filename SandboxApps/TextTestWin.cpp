#include "TextTestWin.h"
#include "ZGraphicSystem.h"
#include "ZWinBtn.h"
#include "ZRasterizer.h"
#include "ZRandom.h"
#include "ZWinFormattedText.h"
#include "ZXMLNode.h"
#include "ZWinControlPanel.h"
#include "helpers/StringHelpers.h"
#include "ZWinPaletteDialog.h"
#include "Resources.h"

#ifdef _WIN64        // for open/save file dialong
#include <windows.h>
#include <shobjidl.h> 
#endif

using namespace std;

extern bool gbApplicationExiting;


static string sAliceText = "\"You can't think how glad I am to see you again, you dear old thing!\" said the Duchess, as she tucked her arm affectionately into Alice's, and they walked off together.\n\
Alice was very glad to find her in such a pleasant temper, and thought to herself that perhaps it was only the pepper that had made her so savage when they met in the kitchen.\n\
\"When I'm a Duchess,\" she said to herself, (not in a very hopeful tone though), \"I won't have any pepper in my kitchen at all. Soup does very well without-Maybe it's always pepper that makes people hot-tempered,\" she went on, very much pleased at having found out a new kind of rule, \"and vinegar that makes them sour-and camomile that makes them bitter-and-and barley-sugar and such things that make children sweet-tempered. I only wish people knew that: then they wouldn't be so stingy about it, you know-\"\n\
She had quite forgotten the Duchess by this time, and was a little startled when she heard her voice close to her ear. \"You're thinking about something, my dear, and that makes you forget to talk. I can't tell you just now what the moral of that is, but I shall remember it in a bit.\"\n\
\"Perhaps it hasn't one,\" Alice ventured to remark.\n\
\"Tut, tut, child!\" said the Duchess. \"Everything's got a moral, if only you can find it.\" And she squeezed herself up closer to Alice's side as she spoke.\n\
Alice did not much like keeping so close to her: first, because the Duchess was very ugly; and secondly, because she was exactly the right height to rest her chin upon Alice's shoulder, and it was an uncomfortably sharp chin. However, she did not like to be rude, so she bore it as well as she could.\n\
\"The game's going on rather better now,\" she said, by way of keeping up the conversation a little.\n\
\"'Tis so,\" said the Duchess: \"and the moral of that is-'Oh, 'tis love, 'tis love, that makes the world go round!'\"\n\
\"Somebody said,\" Alice whispered, \"that it's done by everybody minding their own business!\"\n\
\"Ah, well! It means much the same thing,\" said the Duchess, digging her sharp little chin into Alice's shoulder as she added, \"and the moral of that is-'Take care of the sense, and the sounds will take care of themselves.'\"\n\
\"How fond she is of finding morals in things!\" Alice thought to herself.\n\
\"I dare say you're wondering why I don't put my arm round your waist,\" the Duchess said after a pause: \"the reason is, that I'm doubtful about the temper of your flamingo. Shall I try the experiment?\"\n\
\"He might bite,\" Alice cautiously replied, not feeling at all anxious to have the experiment tried.\n\
\"Very true,\" said the Duchess: \"flamingoes and mustard both bite. And the moral of that is-'Birds of a feather flock together.'\"\n\
\"Only mustard isn't a bird,\" Alice remarked.\n\
\"Right, as usual,\" said the Duchess: \"what a clear way you have of putting things!\"\n\
\"It's a mineral, I think,\" said Alice.\n\
\"Of course it is,\" said the Duchess, who seemed ready to agree to everything that Alice said; \"there's a large mustard-mine near here. And the moral of that is-'The more there is of mine, the less there is of yours.'\"\n\
\"Oh, I know!\" exclaimed Alice, who had not attended to this last remark, \"it's a vegetable. It doesn't look like one, but it is.\"\n\
\"I quite agree with you,\" said the Duchess; \"and the moral of that is-'Be what you would seem to be'-or if you'd like it put more simply-'Never imagine yourself not to be otherwise than what it might appear to others that what you were or might have been was not otherwise than what you had been would have appeared to them to be otherwise.'\"\n\
\"I think I should understand that better,\" Alice said very politely, \"if I had it written down: but I can't quite follow it as you say it.\"\n\
\"That's nothing to what I could say if I chose,\" the Duchess replied, in a pleased tone.\n\
\"Pray don't trouble yourself to say it any longer than that,\" said Alice.\n\
\"Oh, don't talk about trouble!\" said the Duchess. \"I make you a present of everything I've said as yet.\"\n\
\"A cheap sort of present!\" thought Alice. \"I'm glad they don't give birthday presents like that!\" But she did not venture to say it out loud.\n\
\"Thinking again?\" the Duchess asked, with another dig of her sharp little chin.\n\
\"I've a right to think,\" said Alice sharply, for she was beginning to feel a little worried.\n\
\"Just about as much right,\" said the Duchess, \"as pigs have to fly; and the m-\"\n\
But here, to Alice's great surprise, the Duchess's voice died away, even in the middle of her favourite word 'moral,' and the arm that was linked into hers began to tremble. Alice looked up, and there stood the Queen in front of them, with her arms folded, frowning like a thunderstorm.\n\
\"A fine day, your Majesty!\" the Duchess began in a low, weak voice.\n\
\"Now, I give you fair warning,\" shouted the Queen, stamping on the ground as she spoke; \"either you or your head must be off, and that in about half no time! Take your choice!\"\n\
The Duchess took her choice, and was gone in a moment.\n\
\"Let's go on with the game,\" the Queen said to Alice; and Alice was too much frightened to say a word, but slowly followed her back to the croquet-ground.\n\
The other guests had taken advantage of the Queen's absence, and were resting in the shade: however, the moment they saw her, they hurried back to the game, the Queen merely remarking that a moment's delay would cost them their lives.\n\
All the time they were playing the Queen never left off quarrelling with the other players, and shouting \"Off with his head!\" or \"Off with her head!\" Those whom she sentenced were taken into custody by the soldiers, who of course had to leave off being arches to do this, so that by the end of half an hour or so there were no arches left, and all the players, except the King, the Queen, and Alice, were in custody and under sentence of execution.\n\
Then the Queen left off, quite out of breath, and said to Alice, \"Have you seen the Mock Turtle yet?\"\n\
\"No,\" said Alice. \"I don't even know what a Mock Turtle is.\"\n\
\"It's the thing Mock Turtle Soup is made from,\" said the Queen.\n\
\"I never saw one, or heard of one,\" said Alice.\n\
\"Come on, then,\" said the Queen, \"and he shall tell you his history,\"";


TextTestWin::TextTestWin()
{
	mpFont = NULL;
    msWinName="TextTestWin";
    mbViewBackground = true;
    mDraggingTextbox = false;
    mbAcceptsFocus = true;
    mbAcceptsCursorMessages = true;
}


const int64_t kSquareSize = 1;

const uint32_t kOnPixel = 0xffffffff;
const uint32_t kOffPixel = 0xff000000;

/////////////////////////////////////////////////////////////////////////
// 
// TextTestWin
//
/////////////////////////////////////////////////////////////////////////

bool TextTestWin::Init()
{

    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;

    mIdleSleepMS = 10000;
    mPalette.mColorMap.push_back(ZGUI::EditableColor("font_col", 0xff88ff88));
    mPalette.mColorMap.push_back(ZGUI::EditableColor("shadow_col", 0xff000000));
    mPalette.mColorMap.push_back(ZGUI::EditableColor("bg_col", 0xff999999));

    SetFocus();

#ifndef _WIN64
    std::vector<string> fontNames = gpFontSystem->GetFontNames();
    string sFontName = fontNames[RANDU64(0, fontNames.size())];

    std::vector<int32_t> fontSizes = gpFontSystem->GetAvailableSizes(sFontName);

    int32_t nSize = fontSizes[RANDU64(0, fontSizes.size())];

    mpFont = gpFontSystem->GetDefaultFont(sFontName, nSize);
#endif

#ifdef _WIN64
    mpBackground.reset(new ZBuffer());
    mpBackground.get()->LoadBuffer("res/paper.jpg");
#else
    mpBackground.reset(new ZBuffer());
    mpBackground.get()->LoadBuffer("/app0/res/paper.jpg");
#endif


#ifdef _WIN64
    mnSelectedFontIndex = (int32_t)(RANDU64(0, gWindowsFontFacenames.size()));


    mCustomFontHeight = 100;
    mTextBox.style.fp.sFacename = gWindowsFontFacenames[mnSelectedFontIndex];
    mbViewShadow = true;

    mTextBox.sText = "Cool Beans!";
    mTextBox.style = gStyleCaption;
//    mTextBox.style.bgCol = 0x88000088;
    mTextBox.style.pos = ZGUI::LT;
    mTextBox.style.pad.h = 25;
    mTextBox.style.pad.v = 25;
    mTextBox.style.look.colTop = 0xffffff00;
    mTextBox.style.look.colBottom = 0xffffff00;
    mTextBox.style.fp = mTextBox.style.fp;
    mTextBox.style.fp.nScalePoints = ZFontParams::ScalePoints(mCustomFontHeight);
    mTextBox.style.look.decoration = ZGUI::ZTextLook::kNormal;
    mTextBox.style.wrap = true;
//    int64_t offset = std::max <int64_t>(mTextBox.style.fp.Height() / 32, 1);
    int64_t offset = 0;
    mTextBox.shadow.offset.Set(offset, offset);
    mShadowRadius = 20.0;
    //mShadowFalloff = 20.0;









    mbEnableKerning = true;

    ZFont* pNewFont = new ZFont();
    pNewFont->Init(mTextBox.style.fp);
    pNewFont->SetEnableKerning(mbEnableKerning);
    mpFont.reset(pNewFont);

    mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;

    mIdleSleepMS = 10000;

    SetFocus();

    int32_t nWidth = (int32_t)(mAreaLocal.Width() / 8);
    int32_t nHeight = (int32_t)(mAreaLocal.Height()) / 2;
    int32_t nSpacer = (int32_t)mAreaLocal.right / 200;
    ZRect rFontSelectionWin(mAreaLocal.right - nWidth - nSpacer, mAreaLocal.top + nSpacer, mAreaLocal.right, mAreaLocal.top + nHeight - nSpacer);
    ZRect rTextAreaWin(10, 10, rFontSelectionWin.Width() - 10, rFontSelectionWin.Height() - 10);

    //ZWinScriptedDialog* pWin = new ZWinScriptedDialog();
    ZWinFormattedDoc* pWin = new ZWinFormattedDoc();
    pWin->SetArea(rFontSelectionWin);
    pWin->mStyle = mTextBox.style;
    pWin->mStyle.fp = gDefaultTextFont;
    pWin->mStyle.bgCol = 0xff444444;
    pWin->SetBehavior(ZWinFormattedDoc::kScrollable | ZWinFormattedDoc::kBackgroundFill, true);

    mpBackground.get()->LoadBuffer("res/paper.jpg");

    //    string sDialogScript("<scripteddialog winname=fontselectionwin area=" + RectToString(rFontSelectionWin) + " draw_background=1 bgcolor=0xff444444>");
    //    sDialogScript += "<textwin area=" + RectToString(rTextAreaWin) + " text_background_fill=1 text_fill_color=0xff444444 text_edge_blt=0 text_scrollable=1 underline_links=1 auto_scroll=1 auto_scroll_momentum=0.001>";



    //    string sEncodedFontParams("fontparams="+SH::URL_Encode(gDefaultTextFont)+" ");

    for (int i = 0; i < gWindowsFontFacenames.size(); i++)
    {
        string sMessage;
        Sprintf(sMessage, "link={setcustomfont;fontindex=%d;target=TextTestWin}", i);

        string sLine("<line wrap=0><text color=0xffffffff color2=0xffffffff deco=normal ");
        //        sLine += sEncodedFontParams;
        sLine += sMessage.c_str();
        sLine += " position=lc>";
        sLine += gWindowsFontFacenames[i];
        sLine += "</text></line>";

        pWin->AddLineNode(sLine);
    }

    //    pWin->PreInit(sDialogScript);
    ChildAdd(pWin);



    UpdateText();
    mTextBox.area.SetRect(gM, gM, grFullArea.right-gM, grFullArea.bottom-gM);


    ZRect rControlPanel(rFontSelectionWin.left, rFontSelectionWin.bottom, rFontSelectionWin.right, mAreaLocal.bottom);     // upper right for now

    ZWinControlPanel* pCP = new ZWinControlPanel();
    pCP->SetArea(rControlPanel);
    //    pCP->SetTriggerRect(grControlPanelTrigger);




    pCP->Init();

    ZGUI::ZTextLook captionLook(ZGUI::ZTextLook::kShadowed, 0xffffffff, 0xffffffff);

    pCP->Toggle("viewbg", &mbViewBackground, "Background", "{invalidate;target=TextTestWin}", "{invalidate;target=TextTestWin}");

    pCP->Button("choosecolor", "Color", ZMessage("choosecolor", this));

    pCP->Caption("heightlabel", "Height");

    pCP->Slider("fontheight", &mCustomFontHeight, 8, 800, 2, 0.1, "{setcustomfont;target=TextTestWin}", true, false);

    pCP->Caption("weight", "Weight");
    pCP->Slider("fontweight", &mTextBox.style.fp.nWeight, 2, 9, 100, 0.1, "{setcustomfont;target=TextTestWin}", true, false);

    pCP->Caption("Tracking", "Tracking");
    pCP->Slider("fonttracking", &mTextBox.style.fp.nTracking, -20, 20, 1, 0.1, "{setfonttracking;target=TextTestWin}", true, false);

    pCP->Caption("Fixed Width", "Fixed Width");
    pCP->Slider("fontfixedwidth", &mTextBox.style.fp.nFixedWidth, 0, 200, 1, 0.1, "{setcustomfont;target=TextTestWin}", true, false);

    pCP->Toggle("fontitalic", &mTextBox.style.fp.bItalic, "Italic", "{setcustomfont;target=TextTestWin}", "{setcustomfont;target=TextTestWin}");

    pCP->Toggle("fontsymbolic", &mTextBox.style.fp.bSymbolic, "Symbolic", "{setcustomfont;target=TextTestWin}", "{setcustomfont;target=TextTestWin}");



    pCP->AddSpace(16);

    pCP->Toggle("viewkerning", &mbEnableKerning, "View Kerning", "{togglekerning;enable=1;target=TextTestWin}", "{togglekerning;enable=0;target=TextTestWin}");

/*    ZWinLabel* pCaption = pCP->Caption("shadow_offset", "View Shadow x/y");
    pCaption->mStyle.look.colTop = 0xff00ff00;
    pCaption->mStyle.look.colBottom = 0xff008800;*/

    pCP->AddSpace(16);

    ZWinCheck* pCheck = pCP->Toggle("toggleshadow", &mbViewShadow, "View Shadow", "{updatetext;target=TextTestWin}", "{updatetext;target=TextTestWin}");

  
    pCP->Slider("shadowoffset_x", &mTextBox.shadow.offset.x, -200, 200, 1, 0.1, "{updatetext;target=TextTestWin}", true, false);
    pCP->Slider("shadowoffset_y", &mTextBox.shadow.offset.y, -200, 200, 1, 0.1, "{updatetext;target=TextTestWin}", true, false);

    ZWinLabel* pCaption = pCP->Caption("shadow_blur", "View Shadow Radius");
    pCaption->mStyle.look.colTop = 0xff00ff00;
    pCaption->mStyle.look.colBottom = 0xff008800;

    pCP->Slider("shadow_radius", &mShadowRadius, 0.0, 100.0, 1.0, 0.1, "{updatetext;target=TextTestWin}", true, false);
//    pCP->Slider("shadow_falloff", &mShadowFalloff, 0.1, 100.0, 1.0, 0.1, "{updatetext;target=TextTestWin}", true, false);
    
    pCP->AddSpace(16);

    pCP->Button("loadfont", "Load Font", "{loadfont;target=TextTestWin}");
    pCP->Button("savefont", "Save Font", "{savefont;target=TextTestWin}");

    ChildAdd(pCP);

#endif //_WIN64


	return ZWin::Init();
}

bool TextTestWin::Shutdown()
{
	if (!mbInitted)
		return false;

	ZWin::Shutdown();

	return true;
}


bool TextTestWin::Paint()
{
    if (!PrePaintCheck())
        return false;

    const std::lock_guard<std::recursive_mutex> surfaceLock(mpSurface.get()->GetMutex());

	ZRect rText(32, 32, mAreaLocal.right*4/5, mAreaLocal.bottom);

    if (mbViewBackground && mpBackground.get()->GetArea().Width() > 0)
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(mAreaLocal, verts);
        gRasterizer.Rasterize(mpSurface.get(), mpBackground.get(), verts);
    }
    else
    {
        mpSurface.get()->Fill(mPalette.Get("bg_col"));
    }


    mTextBox.Paint(mpSurface.get());


    /*

    string sTemp;
    rText.SetRect(32, 32+ mpFont->Height(), mAreaLocal.right * 4 / 5,mAreaLocal.bottom);

    
    Sprintf(sTemp, "Font: %s Size:%d", mpFont->GetFontParams().sFacename.c_str(), mpFont->Height());
    ZGUI::Style sampleStyle(mpFont->GetFontParams(), ZGUI::ZTextLook(ZGUI::ZTextLook::kNormal, 0xFF880044, 0xFF000000), ZGUI::LT, gSpacer, 0, true);
    mpFont->DrawTextParagraph(mpSurface.get(), sTemp.c_str(), rText, &sampleStyle);
    rText.OffsetRect(0, mpFont->Height()*2);

    string sSampleText("The quick brown fox jumped over the lazy dog.\nA Relic is Relish of Radishes! Show me the $$$$");
    for (int i = 0; i < 2 && rText.top < mAreaLocal.bottom; i++)
    {
        uint32_t nCol1 = RANDI64(0xff000000, 0xffffffff);
        uint32_t nCol2 = RANDI64(0xff000000, 0xffffffff);

        if (RANDBOOL)
            nCol2 = nCol1;

        ZGUI::Style style(mpFont->GetFontParams(), ZGUI::ZTextLook(ZGUI::ZTextLook::kShadowed, nCol1, nCol2), ZGUI::LT, 0, 0, true);

        if (RANDBOOL)
            mpFont->DrawTextParagraph(mpSurface.get(), sSampleText, rText, &style);
        else
            mpFont->DrawTextParagraph(mpSurface.get(), sSampleText, rText, &style);

        int64_t nLines = mpFont->CalculateNumberOfLines(rText.Width(), (uint8_t*)sSampleText.data(), sSampleText.length());
        rText.OffsetRect(0, mpFont->Height() * nLines);
    }


    TIME_SECTION_START(TextTestLines);
    mpFont->DrawTextParagraph(mpSurface.get(), sAliceText, rText, &sampleStyle);

  TIME_SECTION_END(TextTestLines);
  */

    
	return ZWin::Paint();
}

bool TextTestWin::OnChar(char key)
{
#ifdef _WIN64
	switch (key)
	{
    case VK_BACK:
        if (!mTextBox.sText.empty())
        {
            mTextBox.sText = mTextBox.sText.substr(0, mTextBox.sText.length() - 1);
            UpdateText();
        }
        break;

    case VK_RETURN:
        mTextBox.sText += '\n';
        break;

    case VK_ESCAPE:
        gMessageSystem.Post("{quit_app_confirmed}");
        break;
    default:
        mTextBox.sText += key;
        UpdateText();
		break;
	}

    Invalidate();
#endif
	return ZWin::OnKeyDown(key);
}

void TextTestWin::UpdateText()
{
//    mTextBox.style.fp = mTextBox.style.fp;
    if (mbViewShadow)
    {
        mTextBox.style.look.SetCol(mPalette.Get("font_col"));
        mTextBox.shadow.col = mPalette.Get("shadow_col");
        mTextBox.shadow.radius = mShadowRadius;
//        mTextBox.shadow.falloff = mShadowFalloff;
    }
    else
    {
        mTextBox.shadow.col = 0x0;
    }

    Invalidate();
}

void TextTestWin::UpdateFontByParams()
{
#ifdef _WIN64
    ZFont* pNewFont = new ZFont();
    mTextBox.style.fp.nScalePoints = ZFontParams::ScalePoints(mCustomFontHeight);
    pNewFont->Init(mTextBox.style.fp);

    if (mTextBox.style.fp.nFixedWidth > 0)
        mbEnableKerning = false;

    pNewFont->SetEnableKerning(mbEnableKerning);

    mpFont.reset(pNewFont);

    UpdateText();
#endif

}


bool TextTestWin::OnMouseDownL(int64_t x, int64_t y)
{
    if (mTextBox.area.PtInRect(x, y))
    {
        SetCapture();
        mDraggingTextbox = true;
        mDraggingTextboxAnchor.x = x-mTextBox.area.left;
        mDraggingTextboxAnchor.y = y-mTextBox.area.left;
        Invalidate();
    }

    return ZWin::OnMouseDownL(x, y);
}

bool TextTestWin::OnMouseUpL(int64_t x, int64_t y)
{
    if (AmCapturing())
        ReleaseCapture();
    mDraggingTextbox = false;
    return ZWin::OnMouseUpL(x, y);
}

bool TextTestWin::OnMouseMove(int64_t x, int64_t y)
{
    if (mDraggingTextbox)
    {
        mTextBox.area.MoveRect(x-mDraggingTextboxAnchor.x, y-mDraggingTextboxAnchor.y);
        Invalidate();
    }
    return ZWin::OnMouseMove(x, y);
}

bool TextTestWin::OnMouseDownR(int64_t x, int64_t y)
{
    return ZWin::OnMouseDownR(x, y);
}

bool TextTestWin::HandleMessage(const ZMessage& message)
{
    string sType = message.GetType();
//    ZFontParams params(*(mpFont.get()->GetFontParams()));  // copy params
#ifdef _WIN64
    if (sType == "setcustomfont")
    {
        if (message.HasParam("fontindex"))
        {
            int32_t nIndex = (int32_t) SH::ToInt(message.GetParam("fontindex"));

            if (nIndex >= 0 && nIndex < gWindowsFontFacenames.size())
            {
                mnSelectedFontIndex = nIndex;
                mTextBox.style.fp.sFacename = gWindowsFontFacenames[mnSelectedFontIndex];
            }
        }

        UpdateFontByParams();

        return true;
    }
    else if (sType == "updatetext")
    {
        UpdateText();
        return true;
    }
    else if (sType == "setfonttracking")
    {
        mpFont->SetTracking(mTextBox.style.fp.nTracking);
        Invalidate();
        return true;
    }
    else if (sType == "togglekerning")
    {
        mbEnableKerning = SH::ToBool(message.GetParam("enable"));
        mpFont->SetEnableKerning(mbEnableKerning);
        Invalidate();
        return true;
    }
    else if (sType == "loadfont")
    {
        LoadFont();
        return true;
    }
    else if (sType == "savefont")
    {
        SaveFont();
        return true;
    }
    else if (sType == "choosecolor")
    {

        ZWinPaletteDialog::ShowPaletteDialog("testcaption", &mPalette.mColorMap, ZMessage("updatetext", this));
        return true;
    }
#endif
    return ZWin::HandleMessage(message);
}


bool TextTestWin::LoadFont()
{
    string filename;
#ifdef _WIN64
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog* pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

        if (SUCCEEDED(hr))
        {
            COMDLG_FILTERSPEC rgSpec[] =
            {
                { L"ZFont", L"*.zfont" },
                { L"All Files", L"*.*" }
            };

            pFileOpen->SetFileTypes(2, rgSpec);

            DWORD dwOptions;
            // Specify multiselect.
            hr = pFileOpen->GetOptions(&dwOptions);

            // Show the Open dialog box.
            hr = pFileOpen->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItemArray* pItems;
                hr = pFileOpen->GetResults(&pItems);
                if (SUCCEEDED(hr))
                {
                    DWORD nCount = 0;
                    pItems->GetCount(&nCount);
                    for (DWORD i = 0; i < nCount; i++)
                    {
                        IShellItem* pItem;
                        pItems->GetItemAt(i, &pItem);

                        PWSTR pszFilePath;
                        pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath);

                        filename = SH::wstring2string(pszFilePath);
                        //                        wstring wideFilename(pszFilePath);
                        //                        filenames.push_back(WideToAscii(wideFilename));
                        pItem->Release();
                    }
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }

    if (!filename.empty())
    {
        if (mpFont->LoadFont(filename))
            mTextBox.style.fp = mpFont->GetFontParams();
        InvalidateChildren();
        return true;
    }
#endif

    return false;
}

bool TextTestWin::SaveFont()
{
    string filename;

#ifdef _WIN64
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileSaveDialog* pFileSave;

        // Create the FileSaveDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

        if (SUCCEEDED(hr))
        {
            COMDLG_FILTERSPEC rgSpec[] =
            {
                { L"ZFont", L"*.zfont" },
                { L"All Files", L"*.*" }
            };

            pFileSave->SetFileTypes(2, rgSpec);

            DWORD dwOptions;
            // Specify multiselect.
            hr = pFileSave->GetOptions(&dwOptions);

            string sFilename;
            Sprintf(sFilename, "%s_%d.zfont", mpFont->GetFontParams().sFacename.c_str(), mpFont->GetFontParams().Height());

            pFileSave->SetFileName(SH::string2wstring(sFilename).c_str());

            // Show the Save dialog box.
            hr = pFileSave->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileSave->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    PWSTR pszFilePath;
                    pItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &pszFilePath);

                    filename = SH::wstring2string(pszFilePath);
                    pItem->Release();
                }
            }
            pFileSave->Release();
        }
        CoUninitialize();
    }

    if (!filename.empty())
        return mpFont->SaveFont(filename);
#endif

    return false;
}