#include "TextTestWin.h"
#include "ZGraphicSystem.h"
#include "ZWinBtn.h"
#include "ZRasterizer.h"
#include "ZRandom.h"
#include "ZFormattedTextWin.h"
#include "ZScriptedDialogWin.h"
#include "ZXMLNode.h"
#include "ZWinControlPanel.h"
#include "helpers/StringHelpers.h"

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

    msText = "Type here";

    SetFocus();

#ifndef _WIN64
    std::vector<string> fontNames = gpFontSystem->GetFontNames();
    string sFontName = fontNames[ RANDU64(0, fontNames.size()) ];

    std::vector<int32_t> fontSizes = gpFontSystem->GetAvailableSizes(sFontName);

    int32_t nSize = fontSizes[ RANDU64(0, fontSizes.size()) ];

    mpFont = gpFontSystem->GetDefaultFont(sFontName, nSize);
#endif

#ifdef _WIN64
    mBackground.LoadBuffer("res/paper.jpg");
#else
    mBackground.LoadBuffer("/app0/res/paper.jpg");
#endif

    
#ifdef _WIN64
	//mpFont = gGraphicSystem.GetDefaultFont(0);
    mnSelectedFontIndex = (int32_t) (RANDU64( 0, gWindowsFontFacenames.size() ));

    mCustomFontParams.nHeight = 50;
    mCustomFontParams.nWeight = 200;
    mCustomFontParams.sFacename = gWindowsFontFacenames[mnSelectedFontIndex];
//    mCustomFontParams.sFacename = "Blackadder ITC\n";

    mbEnableKerning = true;

    ZDynamicFont* pNewFont = new ZDynamicFont();
    pNewFont->Init(mCustomFontParams);
    pNewFont->SetEnableKerning(mbEnableKerning);
    mpFont.reset(pNewFont);

	mbAcceptsCursorMessages = true;
    mbAcceptsFocus = true;

    mIdleSleepMS = 10000;

    msText = "Type here";

    SetFocus();

    int32_t nWidth = (int32_t) (mAreaToDrawTo.right / 8);
    ZRect rFontSelectionWin(mAreaToDrawTo.right - nWidth, mAreaToDrawTo.top + 100, mAreaToDrawTo.right, mAreaToDrawTo.bottom - 400);
    ZRect rTextAreaWin(10,10,rFontSelectionWin.Width()-10,rFontSelectionWin.Height()-10);

    ZScriptedDialogWin* pWin = new ZScriptedDialogWin();
    pWin->SetArea(rFontSelectionWin);

    mBackground.LoadBuffer("res/paper.jpg");

/*    ZFormattedTextWin* pTextWin = new ZFormattedTextWin();
    pTextWin->SetScrollable();
    int32_t nWidth = mAreaToDrawTo.right/8;
//    ZRect rTextWin(mAreaToDrawTo.right -nWidth, mAreaToDrawTo.top+150, mAreaToDrawTo.right, mAreaToDrawTo.bottom - 205);
*/

    string sDialogScript("<scripteddialog winname=fontselectionwin area=" + RectToString(rFontSelectionWin) + " draw_background=1 bgcolor=0xff444444>");
    sDialogScript += "<textwin area=" + RectToString(rTextAreaWin) + " text_background_fill=1 text_fill_color=0xff444444 text_edge_blt=0 text_scrollable=1 underline_links=1 auto_scroll=1 auto_scroll_momentum=0.001>";

    for (int i = 0; i < gWindowsFontFacenames.size(); i++)
    {
        string sMessage;
        Sprintf(sMessage, "link=\"<msg>type=setcustomfont;fontindex=%d;target=TextTestWin</msg>\"", i);

        sDialogScript += "<line wrap=0><text size=2 color=0xffffffff color2=0xffffffff style=shadowed ";
        sDialogScript += sMessage.c_str();
        sDialogScript += " position=MiddleLeft>";
        sDialogScript += gWindowsFontFacenames[i];
        sDialogScript += "</text></line>";
    }
    sDialogScript += "</textwin>";
//    sDialogScript += "<btn size=4 area=0,0,200,30 color=0xff000000 style=normal message=\"<msg>type=kill_window;target=fontselectionwin</msg>\">Done</btn>";
    sDialogScript += "</scripteddialog>";

//    sDialogScript += "<btn size=4 area=" + RectToString(grBottomRightButton) + " color=0xff000000 color2=0xff000000 style=normal position=middlecenter message=\"<msg>type=kill_window;target=optionswin</msg>\">Done</btn>";

//    ZXMLNode doc;
//    doc.Init(sDialogScript);

//    pTextWin->InitFromXML(doc.GetChild("textwin"));
//    pWin->InitFromXML(doc.GetChild("scripteddialog"));
    pWin->PreInit(sDialogScript);
    ChildAdd(pWin);


    ZRect rControlPanel(rFontSelectionWin.left, rFontSelectionWin.bottom, rFontSelectionWin.right, mAreaToDrawTo.bottom);     // upper right for now

    ZWinControlPanel* pCP = new ZWinControlPanel();
    pCP->SetArea(rControlPanel);
    //    pCP->SetTriggerRect(grControlPanelTrigger);

    pCP->Init();

//    pCP->AddButton("Custom Font", "type=setcustomfont;target=TextTestWin");
    pCP->AddCaption("Height", 2, 0xff444444, 0xff737373, ZFont::kEmbossed);

    pCP->AddSlider(&mCustomFontParams.nHeight, 2, 100, 2, "type=setcustomfont;target=TextTestWin", true, false, 2);

    pCP->AddCaption("Weight", 2, 0xff444444, 0xff737373, ZFont::kEmbossed);
    pCP->AddSlider(&mCustomFontParams.nWeight, 2, 9, 100, "type=setcustomfont;target=TextTestWin", true, false, 2);

    pCP->AddCaption("Tracking", 2, 0xff444444, 0xff737373, ZFont::kEmbossed);
    pCP->AddSlider(&mCustomFontParams.nTracking, -20, 20, 1, "type=setfonttracking;target=TextTestWin", true, false, 2);

    pCP->AddToggle(&mCustomFontParams.bItalic, "Italic", "type=setcustomfont;target=TextTestWin", "type=setcustomfont;target=TextTestWin", 2, 0xff737373, ZFont::kEmbossed);

    pCP->AddToggle(&mCustomFontParams.bUnderline, "Underline", "type=setcustomfont;target=TextTestWin", "type=setcustomfont;target=TextTestWin", 2, 0xff737373, ZFont::kEmbossed);

    pCP->AddToggle(&mCustomFontParams.bStrikeOut, "StrikeOut", "type=setcustomfont;target=TextTestWin", "type=setcustomfont;target=TextTestWin", 2, 0xff737373, ZFont::kEmbossed);



    pCP->AddSpace(16);

    pCP->AddToggle(&mbEnableKerning, "View Kerning", "type=togglekerning;enable=1;target=TextTestWin", "type=togglekerning;enable=0;target=TextTestWin", 2, 0xff737373, ZFont::kEmbossed);
    pCP->AddSpace(16);

    pCP->AddButton("Load Font", "type=loadfont;target=TextTestWin", 2, 0xff737373, 0xff737373, ZFont::kEmbossed);
    pCP->AddButton("Save Font", "type=savefont;target=TextTestWin", 2, 0xff737373, 0xff737373, ZFont::kEmbossed);

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
    const std::lock_guard<std::mutex> surfaceLock(mpTransformTexture->GetMutex());
    if (!mbInvalid)
        return true;



	string sTemp;

	ZRect rText(0, 0, mAreaToDrawTo.right, mAreaToDrawTo.bottom);

    if (mBackground.GetArea().Width() > 0)
    {
        tUVVertexArray verts;
        gRasterizer.RectToVerts(mAreaToDrawTo, verts);
        gRasterizer.Rasterize(mpTransformTexture, &mBackground, verts);
    }
    else
        mpTransformTexture->Fill(mAreaToDrawTo, 0xff595850);

    sTemp = msText + '|';

    assert(mpFont);
    int32_t nLines = (int32_t) (mpFont->CalculateNumberOfLines(rText.Width(), sTemp.data(), sTemp.length()));
    
    mpFont->DrawTextParagraph(mpTransformTexture, sTemp, rText, 0xAA000000, 0x88000000);





    rText.SetRect(20, 20+ nLines*mpFont->FontHeight(),mAreaToDrawTo.right-20,mAreaToDrawTo.bottom);

    
    Sprintf(sTemp, "Font: %s Size:%d", mpFont->GetFontParams()->sFacename.c_str(), mpFont->FontHeight());
    mpFont->DrawTextParagraph(mpTransformTexture, sTemp.c_str(), rText);
    rText.OffsetRect(0, mpFont->FontHeight()*2);

    string sSampleText("The quick brown fox jumped over the lazy dog.\nA Relic is Relish of Radishes! Show me the $$$$");
    for (int i = 0; i < 1 && rText.top < mAreaToDrawTo.bottom; i++)
    {
        uint32_t nCol1 = RANDI64(0xff000000, 0xffffffff);
        uint32_t nCol2 = RANDI64(0xff000000, 0xffffffff);

        mpFont->DrawTextParagraph(mpTransformTexture, sSampleText, rText, nCol1, nCol2, ZFont::kTopLeft, ZFont::kShadowed);

        int64_t nLines = mpFont->CalculateNumberOfLines(rText.Width(), sSampleText.data(), sSampleText.length());
        rText.OffsetRect(0, mpFont->FontHeight() * nLines);
    }


        TIME_SECTION_START(TextTestLines);
    mpFont->DrawTextParagraph(mpTransformTexture, sAliceText, rText, 0xaa696157, 0x88696157, ZFont::kTopLeft, ZFont::kEmbossed);

  TIME_SECTION_END(TextTestLines);
  

    
	return ZWin::Paint();
}

bool TextTestWin::OnChar(char key)
{
#ifdef _WIN64
	switch (key)
	{
    case VK_BACK:
        if (!msText.empty())
            msText = msText.substr(0, msText.length()-1);
        break;

    case VK_RETURN:
        msText += '\n';
        break;

    case VK_ESCAPE:
        gMessageSystem.Post("quit_app_confirmed");
        break;
    default:
        msText += key;
		break;
	}

    Invalidate();
#endif
	return ZWin::OnKeyDown(key);
}


void TextTestWin::UpdateFontByParams()
{
#ifdef _WIN64
    ZDynamicFont* pNewFont = new ZDynamicFont();
    pNewFont->Init(mCustomFontParams);
    pNewFont->SetEnableKerning(mbEnableKerning);

    mpFont.reset(pNewFont);
    Invalidate();
#endif

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
            int32_t nIndex = (int32_t) StringHelpers::ToInt(message.GetParam("fontindex"));

            if (nIndex >= 0 && nIndex < gWindowsFontFacenames.size())
            {
                mnSelectedFontIndex = nIndex;
                mCustomFontParams.sFacename = gWindowsFontFacenames[mnSelectedFontIndex];
            }
        }

        UpdateFontByParams();

        return true;
    }
    else if (sType == "setfonttracking")
    {
        mpFont->SetTracking(mCustomFontParams.nTracking);
        Invalidate();
        return true;
    }
    else if (sType == "togglekerning")
    {
        mbEnableKerning = StringHelpers::ToBool(message.GetParam("enable"));
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

                        filename = StringHelpers::wstring2string(pszFilePath);
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
            mCustomFontParams = *mpFont->GetFontParams();
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
            Sprintf(sFilename, "%s_%d.zfont", mpFont->GetFontParams()->sFacename.c_str(), mpFont->GetFontParams()->nHeight);

            pFileSave->SetFileName(StringHelpers::string2wstring(sFilename).c_str());

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

                    filename = StringHelpers::wstring2string(pszFilePath);
//                        wstring wideFilename(pszFilePath);
//                        filenames.push_back(WideToAscii(wideFilename));
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