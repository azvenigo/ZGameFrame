#include "Main_Sandbox.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"

using namespace std;

// Global Variables:
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
REG::Registry           gRegistry;


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


    tZFontPtr pBtnFont = gpFontSystem->GetFont(gDefaultButtonFont);
    ZASSERT(pBtnFont);

    gpControlPanel->AddButton("Quit", "type=quit_app_confirmed", pBtnFont);
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);


    gpControlPanel->AddButton("FloatLinesWin",  "type=initchildwindows;mode=1;target=MainAppMessageTarget", pBtnFont);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("LifeWin",        "type=initchildwindows;mode=5;target=MainAppMessageTarget", pBtnFont);
    gpControlPanel->AddSlider(&gnLifeGridSize, 1, 100, 5, "type=initchildwindows;mode=16;target=MainAppMessageTarget", true, false);
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);

    gpControlPanel->AddButton("ImageProcessor", "type=initchildwindows;mode=4;target=MainAppMessageTarget", pBtnFont);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight/2);
    gpControlPanel->AddButton("Checkerboard", "type=initchildwindows;mode=2;target=MainAppMessageTarget", pBtnFont);
    gpControlPanel->AddSlider(&gnCheckerWindowCount, 1, 250, 1, "", true, false);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("Font Tool", "type=initchildwindows;mode=6;target=MainAppMessageTarget", pBtnFont);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("Marquee", "type=initchildwindows;mode=7;target=MainAppMessageTarget", pBtnFont);

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

    gRegistry["sandbox"]["mode"] = (int32_t)mode;
   
    if (mode == eSandboxMode::kFloatLinesWin)
	{

		cFloatLinesWin* pWin = new cFloatLinesWin();
		pWin->Show();
		pWin->SetArea(grFullArea);
		gpMainWin->ChildAdd(pWin);
	}

	else if (mode == eSandboxMode::kCheckerboard)
	{
		int64_t nSubWidth = (int64_t) (grFullArea.Width() / (sqrt(gnCheckerWindowCount)));
        int64_t nSubHeight = (int64_t) (nSubWidth / gGraphicSystem.GetAspectRatio());

		for (int64_t y = 0; y < grFullArea.Height(); y += nSubHeight)
		{
			for (int64_t x = 0; x < grFullArea.Width(); x += nSubWidth)
			{
				int64_t nGridX = x / nSubWidth;
				int64_t nGridY = y / nSubHeight;

				ZRect rSub(x, y, x + nSubWidth, y + nSubHeight);
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
		pWin->SetGridSize((int64_t) (gnLifeGridSize*(gGraphicSystem.GetAspectRatio())), gnLifeGridSize);
		gpMainWin->ChildAdd(pWin);
	}
	else if (mode == eSandboxMode::kImageProcess)
	{
#ifdef _WIN64
		cProcessImageWin* pWin = new cProcessImageWin();
		pWin->SetArea(grFullArea);
		gpMainWin->ChildAdd(pWin);
#endif
	}

    else if (mode == eSandboxMode::kTextTestWin)
    {
        ZRect rArea(grFullArea);
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

        tZFontPtr  pFont = gpFontSystem->GetFont(gDefaultTextFont.sFacename, 10 + rand() % 500);

        pWin->SetText("This is amazing!", pFont, nCol1, nCol2, ZFont::kNormal);
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

    std::filesystem::create_directory("font_cache");    // ensure font cache exists...keep in working folder or move elsewhere?
    gpFontSystem->SetCacheFolder("font_cache");

    gpFontSystem->Init();
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
    gpMainWin->SetArea(grFullArea);
    gpMainWin->Init();

    InitControlPanel();

    int32_t nMode;
    gRegistry.GetOrSetDefault("sandbox", "mode", nMode, (int32_t) kImageProcess);
    SandboxInitChildWindows((eSandboxMode) nMode);
    return true;
}

void Sandbox::SandboxShutdown()
{
    gbApplicationExiting = true;

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