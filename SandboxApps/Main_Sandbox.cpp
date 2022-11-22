#include "Main_Sandbox.h"
#include "helpers/StringHelpers.h"

using namespace std;

// Global Variables:

list<int32_t> frameworkFontSizes = { 12, 18, 20, 30, 40, 60, 100, 150/*, 250, 1000*/ };
list<string> frameworkFontNames = { "Verdana", "Gadugi", "Soft Elegance", "Blackadder ITC", "Vivaldi", "Felix Titling", "Wingdings 3" };
string gsDefaultFontName("Verdana");

ZRect                   grFullArea;
ZMainWin*               gpMainWin;
ZWinControlPanel*       gpControlPanel;
ZGraphicSystem          gGraphicSystem;
ZGraphicSystem*         gpGraphicSystem = &gGraphicSystem;
ZRasterizer             gRasterizer;
bool                    gbGraphicSystemResetNeeded(false);
bool                    gbApplicationExiting = false;
bool                    gbPaused = false;
ZTimer                  gTimer(true);
int64_t                 gnFramesPerSecond = 0;
ZFontSystem*            gpFontSystem = nullptr;
std::list<string>       gDebugOutQueue;
std::mutex              gDebugOutMutex;
ZMessageSystem          gMessageSystem;
ZTickManager            gTickManager;
ZAnimator               gAnimator;
int64_t                 gnCheckerWindowCount = 49;
int64_t                 gnLifeGridSize = 500;
ZWin*                   gpCaptureWin = nullptr;
ZWin*                   gpMouseOverWin = nullptr;
ZPoint                  gLastMouseMove;



void Sandbox::InitControlPanel()
{

    int64_t panelW = grFullArea.Width() / 8;    
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(grFullArea.right - panelW, 0, grFullArea.right, panelH);     // upper right for now

    gpControlPanel = new ZWinControlPanel();
    gpControlPanel->SetArea(rControlPanel);
    gpControlPanel->SetTriggerRect(grControlPanelTrigger);
    gpControlPanel->SetHideOnMouseExit(true);

    gpControlPanel->Init();


    gpControlPanel->AddButton("Quit", "type=quit_app_confirmed");
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);


    gpControlPanel->AddButton("FloatLinesWin",  "type=initchildwindows;mode=1;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("LifeWin",        "type=initchildwindows;mode=5;target=MainAppMessageTarget");
    gpControlPanel->AddSlider(&gnLifeGridSize, 1, 100, 5, "type=initchildwindows;mode=16;target=MainAppMessageTarget", true, false, 1);
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);

    gpControlPanel->AddButton("ImageProcessor", "type=initchildwindows;mode=4;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight/2);
    gpControlPanel->AddButton("Checkerboard", "type=initchildwindows;mode=2;target=MainAppMessageTarget");
    gpControlPanel->AddSlider(&gnCheckerWindowCount, 1, 250, 1, "", true, false, 1);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("Font Tool", "type=initchildwindows;mode=6;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("Marquee", "type=initchildwindows;mode=7;target=MainAppMessageTarget");

    gpMainWin->ChildAdd(gpControlPanel);
}

void Sandbox::SandboxDeleteAllButControlPanel()
{
    gpMainWin->ChildRemove(gpControlPanel);
    gpMainWin->ChildDeleteAll();
}


void Sandbox::SandboxInitChildWindows(Sandbox::eSandboxMode mode)
{
    assert(gpControlPanel);     // needs to exist before this

    SandboxDeleteAllButControlPanel();
   
    if (mode == eSandboxMode::kFloatLinesWin)
	{

		cFloatLinesWin* pWin = new cFloatLinesWin();
		//	pWin->SetArea(0, 0, 500, 500);
		pWin->Show();
		pWin->SetArea(grFullArea);
		//pWin->SetImageToProcess("res/414A2616.jpg");
		//pWin->SetImageToProcess("res/test_contrast_image2.png");
		//pWin->SetImageToProcess("res/test_contrast_onepixel.png");
		gpMainWin->ChildAdd(pWin);
	}

	else if (mode == eSandboxMode::kCheckerboard)
	{
//        double fAspect = (double)grFullArea.Width() / (double)grFullArea.Height();

		int64_t nSubWidth = (int64_t) (grFullArea.Width() / (sqrt(gnCheckerWindowCount)));
//		int64_t nSubHeight = grFullArea.Height() / (sqrt(((double)gnCheckerWindowCount/gGraphicSystem.GetAspectRatio())));
        int64_t nSubHeight = (int64_t) (nSubWidth / gGraphicSystem.GetAspectRatio());

		for (int64_t y = 0; y < grFullArea.Height(); y += nSubHeight)
		{
			for (int64_t x = 0; x < grFullArea.Width(); x += nSubWidth)
			{
				int64_t nGridX = x / nSubWidth;
				int64_t nGridY = y / nSubHeight;

				ZRect rSub(x, y, x + nSubWidth, y + nSubHeight);

				//			int64_t nScreenX = rand() % (grFullArea.Width() - nSubWidth);
				//			int64_t nScreenY = rand() % grFullArea.Height() - nSubHeight;
				//			ZRect rSub(nScreenX, nScreenY, nScreenX + nSubWidth, nScreenY + nSubHeight );

				if ((nGridX + nGridY % 2) % 2)
				{
					cFloatLinesWin* pWin = new cFloatLinesWin();
					pWin->SetArea(rSub);
					gpMainWin->ChildAdd(pWin);
				}
				else
				{
					cLifeWin* pWin = new cLifeWin();
					pWin->SetArea(rSub);
					pWin->SetGridSize((int64_t) (rSub.Width()/(4.0)), (int64_t) (rSub.Height()/4.0));
					gpMainWin->ChildAdd(pWin);
				}
			}
		}
	}

	else if (mode == eSandboxMode::kOneLifeWin)
	{
		cLifeWin* pWin = new cLifeWin();
		pWin->SetArea(grFullArea);
//        pWin->SetArea(0,0,800,600);

//        double fAspect = (double) grFullArea.Width() / (double) grFullArea.Height();

		pWin->SetGridSize((int64_t) (gnLifeGridSize*(gGraphicSystem.GetAspectRatio())), gnLifeGridSize);
		gpMainWin->ChildAdd(pWin);
	}
	else if (mode == eSandboxMode::kImageProcess)
	{
#ifdef _WIN64
		cProcessImageWin* pWin = new cProcessImageWin();
		pWin->SetArea(0, 0, grFullArea.Width(), grFullArea.Height());


        std::list<string> filenames = {
        "res/414A2616.jpg",
        "res/414A2617.jpg",
        "res/414A2618.jpg",
        "res/414A2619.jpg",
        "res/414A2620.jpg",
        "res/414A2621.jpg",
        "res/414A2622.jpg",
        "res/414A2623.jpg",
        "res/414A2624.jpg",
    
        
        };
        pWin->LoadImages(filenames);


//		pWin->LoadImages("res/414A2616.jpg");
		//pWin->SetImageToProcess("res/test_contrast_image2.png");
		//pWin->SetImageToProcess("res/test_contrast_onepixel.png");
		gpMainWin->ChildAdd(pWin);
#endif
	}

    else if (mode == eSandboxMode::kTextTestWin)
    {
        ZRect rArea(grFullArea);
//        rArea.right = grFullArea.Width()/2;

        TextTestWin* pWin = new TextTestWin();
        pWin->SetArea(rArea);
        gpMainWin->ChildAdd(pWin);
    }

    else if (mode == eSandboxMode::kMarquee)
    {
        Marquee* pWin = new Marquee();
        pWin->SetArea(grFullArea);

        uint32_t nCol1 = RANDU64(0xff000000, 0xffffffff);
        uint32_t nCol2 = RANDU64(0xff000000, 0xffffffff);

        int32_t nDefaultFontCount = gpFontSystem->GetDefaultFontCount();

//        pWin->SetText("T", 7 /*rand()%nDefaultFontCount*/, nCol1, nCol2, ZFont::kNormal);
        pWin->SetText("This is amazing!", rand()%nDefaultFontCount, nCol1, nCol2, ZFont::kNormal);
        pWin->SetFill(0xff000000);
        pWin->SetSpeed(-200.0);

        gpMainWin->ChildAdd(pWin);

    }

    gpMainWin->ChildAdd(gpControlPanel);
}



class MainAppMessageTarget : public IMessageTarget
{
public:
	MainAppMessageTarget()
	{
		gMessageSystem.RegisterTarget(this);
	}

	string GetTargetName() { return "MainAppMessageTarget"; }
	bool ReceiveMessage(const ZMessage& message)
	{
		string sType = message.GetType();
		if (sType == "initchildwindows")
		{
            SandboxInitChildWindows((Sandbox::eSandboxMode) StringHelpers::ToInt(message.GetParam("mode")));
		}

		return true;
	}
};

MainAppMessageTarget gMainAppMessageTarget;

void Sandbox::SandboxInitializeFonts()
{
    if (gpFontSystem)
        return;

    gpFontSystem = new ZFontSystem();
    gpFontSystem->Init();
    gpFontSystem->SetDefaultFontName(gsDefaultFontName);

    for (auto fontSize : frameworkFontSizes)
    {
        for (auto fontName : frameworkFontNames)
        {
            string sFontFile;
            Sprintf(sFontFile, "res/default_resources/%s_%d.zfont", fontName.c_str(), fontSize); // todo, move this define elsewhere?
            gpFontSystem->LoadFont(sFontFile);
        }
    }
    
}

bool Sandbox::SandboxInitialize()
{
    gGraphicSystem.SetArea(grFullArea);
    if (!gGraphicSystem.Init())
    {
        assert(false);
        return false;
    }

    gResources.Init("res/default_resources/");// todo, move this define elsewhere?

    SandboxInitializeFonts();
    gpMainWin = new ZMainWin();
    gpMainWin->SetArea(0, 0, grFullArea.Width(), grFullArea.Height());
    gpMainWin->Init();

    InitControlPanel();
    SandboxInitChildWindows(kImageProcess);
    return true;
}

void Sandbox::SandboxShutdown()
{
    gbApplicationExiting = true;

    //gMessageSystem.Shutdown();
    gPreferences.Save();
    if (gpMainWin)
    {
        gpMainWin->Shutdown();
        delete gpMainWin;
        gpMainWin = nullptr;
    }

    if (gpFontSystem)
    {
        delete gpFontSystem;
        gpFontSystem = nullptr;
    }

    gResources.Shutdown();
    gGraphicSystem.Shutdown();
    gAnimator.KillAllObjects();
}