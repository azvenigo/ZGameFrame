#include "Main_Sandbox.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"

#include "LifeWin.h"
#include "ZWinImage.h"
#include "ProcessImageWin.h"
#include "ZWinDebugConsole.h"
#include "ZWinSlider.h"
#include "FloatLinesWin.h"
#include "TextTestWin.h"
#include "3DTestWin.h"
#include "ZChessWin.h"
#include "Resources.h"
#include "TestWin.h"


using namespace std;

// Global Variables:
ZRect                   grFullArea;
ZMainWin*               gpMainWin;
ZWinControlPanel*       gpControlPanel;
ZWinDebugConsole*       gpDebugConsole;
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
std::list<std::string>  gDebugOutHistory;
std::mutex              gDebugOutHistoryMutex;
size_t                  gDebugHistoryMaxSize = 512;
size_t                  gDebugHistoryCounter = 0;


ZMessageSystem          gMessageSystem;
ZTickManager            gTickManager;
ZAnimator               gAnimator;
int64_t                 gnCheckerWindowCount = 8;
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

    gpControlPanel->AddButton("Quit", "quit_app_confirmed");
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);


    gpControlPanel->AddButton("FloatLinesWin",  "initchildwindows;mode=1;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("LifeWin",        "initchildwindows;mode=5;target=MainAppMessageTarget");
    gpControlPanel->AddSlider(&gnLifeGridSize, 1, 100, 5, 0.2, "initchildwindows;mode=5;target=MainAppMessageTarget", true, false);
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);

    gpControlPanel->AddButton("ImageProcessor", "initchildwindows;mode=4;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight/2);
    gpControlPanel->AddButton("Checkerboard", "initchildwindows;mode=2;target=MainAppMessageTarget");
    gpControlPanel->AddSlider(&gnCheckerWindowCount, 1, 64, 1, 0.2, "", true, false);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("Font Tool", "initchildwindows;mode=6;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("3DTestWin", "initchildwindows;mode=7;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("ChessWin", "initchildwindows;mode=8;target=MainAppMessageTarget");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->AddButton("TestWin", "initchildwindows;mode=9;target=MainAppMessageTarget");


    gpControlPanel->FitToControls();

    gpMainWin->ChildAdd(gpControlPanel);


    gpDebugConsole = new ZWinDebugConsole();
    gpDebugConsole->SetArea(ZRect(grFullArea.left, grFullArea.top, grFullArea.right, grFullArea.Height() / 2));
    gpMainWin->ChildAdd(gpDebugConsole, false);

}

void Sandbox::SandboxDeleteAllButControlPanelAndDebugConsole()
{
    gpMainWin->ChildRemove(gpControlPanel);
    gpMainWin->ChildRemove(gpDebugConsole);
    gpMainWin->ChildDeleteAll();
}


void Sandbox::SandboxInitChildWindows(Sandbox::eSandboxMode mode)
{
    assert(gpControlPanel);     // needs to exist before this

    SandboxDeleteAllButControlPanelAndDebugConsole();

    gRegistry["sandbox"]["mode"] = (int32_t)mode;
   
    if (mode == eSandboxMode::kTestWin)
    {
        TestWin* pWin = new TestWin();
        pWin->SetArea(grFullArea);
        gpMainWin->ChildAdd(pWin);

    }
    else if (mode == eSandboxMode::kFloatLinesWin)
	{

		cFloatLinesWin* pWin = new cFloatLinesWin();
		pWin->SetVisible();
		pWin->SetArea(grFullArea);
		gpMainWin->ChildAdd(pWin);
	}

	else if (mode == eSandboxMode::kCheckerboard)
	{
		int64_t nSubWidth = (int64_t) (grFullArea.Width() / (sqrt(gnCheckerWindowCount)));
        int64_t nSubHeight = (int64_t) (nSubWidth / gGraphicSystem.GetAspectRatio());

        int n3DCount = 0;

		for (int64_t y = 0; y < grFullArea.Height(); y += nSubHeight)
		{
			for (int64_t x = 0; x < grFullArea.Width(); x += nSubWidth)
			{
				int64_t nGridX = x / nSubWidth;
				int64_t nGridY = y / nSubHeight;

				ZRect rSub(x, y, x + nSubWidth, y + nSubHeight);
				if ((nGridX + nGridY % 2) % 2)
				{
                    if (RANDI64(0, 5) == 1)
                    {
                        ZChessWin* pWin = new ZChessWin();
                        pWin->SetArea(rSub);
                        pWin->SetDemoMode(true);
                        gpMainWin->ChildAdd(pWin);

                    }
                    else
                    {
                        cFloatLinesWin* pWin = new cFloatLinesWin();
                        pWin->SetArea(rSub);
                        gpMainWin->ChildAdd(pWin);
                    }
				}
				else
				{
                    if (n3DCount < 5 && RANDI64(0, 10) == 1)
                    {
                        n3DCount++;
                        Z3DTestWin* pWin = new Z3DTestWin();
                        pWin->SetArea(rSub);
                        pWin->SetControlPanelEnabled(false);

                        uint32_t nCol1 = RANDU64(0xff000000, 0xffffffff);
                        uint32_t nCol2 = RANDU64(0xff000000, 0xffffffff);

                        gpMainWin->ChildAdd(pWin);
                    }
                    else
                    {
                        cLifeWin* pWin = new cLifeWin();
                        pWin->SetArea(rSub);
                        pWin->SetGridSize((int64_t)(rSub.Width() / (4.0)), (int64_t)(rSub.Height() / 4.0));
                        gpMainWin->ChildAdd(pWin);
                    }

                    
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

    else if (mode == eSandboxMode::k3DTestWin)
    {
        Z3DTestWin* pWin = new Z3DTestWin();
        pWin->SetArea(grFullArea);

        uint32_t nCol1 = RANDU64(0xff000000, 0xffffffff);
        uint32_t nCol2 = RANDU64(0xff000000, 0xffffffff);

        gpMainWin->ChildAdd(pWin);

    }

    else if (mode == eSandboxMode::kChessWin)
    {
        ZChessWin* pWin = new ZChessWin();
        pWin->SetArea(grFullArea);
        //pWin->SetDemoMode(true);
        gpMainWin->ChildAdd(pWin);
    }

    gpMainWin->ChildAdd(gpControlPanel);
    gpMainWin->ChildAdd(gpDebugConsole, false);
}



class MainAppMessageTarget : public IMessageTarget
{
public:
	MainAppMessageTarget()
	{
		gMessageSystem.RegisterTarget(this);
        gMessageSystem.AddNotification("toggleconsole", this);
	}

	string GetTargetName() { return "MainAppMessageTarget"; }
	bool ReceiveMessage(const ZMessage& message)
	{
		string sType = message.GetType();
		if (sType == "initchildwindows")
		{
            SandboxInitChildWindows((Sandbox::eSandboxMode) SH::ToInt(message.GetParam("mode")));
		}
        else if (sType == "toggleconsole")
        {
            gpDebugConsole->SetVisible(!gpDebugConsole->IsVisible());
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
    gpFontSystem->SetDefaultFont(gDefaultTextFont);

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
    if (!gRegistry.Get("sandbox", "mode", nMode))
        nMode = kImageProcess;

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