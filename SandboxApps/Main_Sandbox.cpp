#include "Main_Sandbox.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "helpers/FileLogger.h"

#include "LifeWin.h"
#include "ZWinImage.h"
#include "ProcessImageWin.h"
#include "ZWinDebugConsole.h"
#include "ZWinSlider.h"
#include "FloatLinesWin.h"
#include "TextTestWin.h"
#include "TestWin.h"
#include "OnePageDocWin.h"
#include "ParticleSandbox.h"
#include "3DTestWin.h"
#include "ZChessWin.h"
#include "Resources.h"


using namespace std;

// Global Variables:
//ZRect                   grFullArea;
ZMainWin*               gpMainWin;
ZWinControlPanel*       gpControlPanel;
ZWinDebugConsole*       gpDebugConsole;

ZAnimObject_StaticImage* gpOverlay;
ZGraphicSystem          gGraphicSystem;
ZGraphicSystem*         gpGraphicSystem = &gGraphicSystem;
ZRasterizer             gRasterizer;
bool                    gbGraphicSystemResetNeeded(false);
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
REG::Registry           gRegistry;
LOG::FileLogger         gLogger;

bool                    gbShowOverlayWin = false;


void Sandbox::InitControlPanel()
{

    int64_t controlW = grFullArea.Width() / 10;
    int64_t controlH = grFullArea.Height() / 20;
    gnControlPanelButtonHeight = grFullArea.Height() / 50;


    int64_t panelW = grFullArea.Width() / 8;    
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(0, 0, panelW, panelH);     // upper left

    gpControlPanel = new ZWinControlPanel();
    gpControlPanel->SetArea(rControlPanel);
    gpControlPanel->mrTrigger.Set(0,0,gM*2,gM*2);
    gpControlPanel->mbHideOnMouseExit = true;

    gpControlPanel->Init();

    gpControlPanel->Toggle("overlaybtn", nullptr, "Overlay Window", "{toggleoverlay;target=MainAppMessageTarget}", "{toggleoverlay;target=MainAppMessageTarget}");

    gpControlPanel->Button("quit_app_confirmed", "Quit", "{quit_app_confirmed}");
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);


    gpControlPanel->Button("FloatLinesWin", "FloatLinesWin",  "{initchildwindows;mode=1;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("LifeWin", "LifeWin", "{initchildwindows;mode=5;target=MainAppMessageTarget}");
    gpControlPanel->Slider("lifegridsize", &gnLifeGridSize, 1, 100, 5, 0.2, "{initchildwindows;mode=5;target=MainAppMessageTarget}", true, false);
    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);

    gpControlPanel->Button("ImageProcessor", "ImageProcessor", "{initchildwindows;mode=4;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight/2);
    gpControlPanel->Button("checkerboard", "Checkerboard", "{initchildwindows;mode=2;target=MainAppMessageTarget}");
    gpControlPanel->Slider("checkerwindowcount", &gnCheckerWindowCount, 1, 64, 1, 0.2, "", true, false);

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("fonttool", "Font Tool", "{initchildwindows;mode=6;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("3DTestWin", "3DTestWin", "{initchildwindows;mode=7;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("ChessWin", "ChessWin", "{initchildwindows;mode=8;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("TestWin", "TestWin", "{initchildwindows;mode=9;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("ParticleSandbox", "ParticleSandbox", "{initchildwindows;mode=10;target=MainAppMessageTarget}");

    gpControlPanel->AddSpace(gnControlPanelButtonHeight / 2);
    gpControlPanel->Button("OnePageDoc", "OnePageDoc", "{initchildwindows;mode=11;target=MainAppMessageTarget}");

    gpControlPanel->FitToControls();

    gpControlPanel->mTransformIn = ZWin::kToOrFrom;
    gpControlPanel->mTransformOut = ZWin::kToOrFrom;
    gpControlPanel->mToOrFrom = ZTransformation(ZPoint(grFullArea.right, grFullArea.top + gM), 0.0, 0.0);


    gpMainWin->ChildAdd(gpControlPanel);


    gpDebugConsole = new ZWinDebugConsole();
    gpDebugConsole->SetArea(ZRect(grFullArea.left, grFullArea.top, grFullArea.right, grFullArea.Height() / 2));
    gpMainWin->ChildAdd(gpDebugConsole, false);

}

void Sandbox::DeleteAllButControlPanelAndDebugConsole()
{
    for (auto& pWin : sandboxWins)
        gpMainWin->ChildDelete(pWin);

    sandboxWins.clear();
    gpMainWin->ChildRemove(gpControlPanel);
    gpMainWin->ChildRemove(gpDebugConsole);
//    gpMainWin->ChildDeleteAll();
}

void Sandbox::ToggleOverlay()
{
    if (gbShowOverlayWin)
    {
        if (!gpOverlay)
        {
            gpOverlay = new ZAnimObject_StaticImage();
            gpOverlay->mrDrawArea = grFullArea;
            gpOverlay->mImage.reset(new ZBuffer());
            gpOverlay->mImage->Init(grFullArea.Width(), grFullArea.Height());
            gpOverlay->mImage->Fill(0x10000000);

            ZGUI::Style titleStyle(gStyleCaption);
            titleStyle.fp.sFacename = "Ravi";
            titleStyle.fp.nScalePoints = 20000;
            titleStyle.fp.nTracking = 20;
            titleStyle.fp.nWeight = 900;
            titleStyle.pos = ZGUI::C;

            string sCaption("Where is the\nAny key?");

            ZRect r(titleStyle.Font()->Arrange(grFullArea, sCaption, ZGUI::CT));
            /*
            // BLURRY SHADOW
            titleStyle.look.colBottom = 0xff000000;
            titleStyle.look.colTop = 0xff000000;
            titleStyle.Font()->DrawTextParagraph(gpOverlay->mImage.get(), sCaption, r, &titleStyle);
            gpOverlay->mImage.get()->Blur(10.0);
            r.Offset(RANDI64(-20,20), RANDI64(-20, 20));
            */

            titleStyle.look.colTop = 0xffffffff;
            titleStyle.look.colBottom = 0xffffffff;
            titleStyle.Font()->DrawTextParagraph(gpOverlay->mImage.get(), sCaption, r, &titleStyle);
            

            // invert the alpha channel
            uint32_t* pPixels = gpOverlay->mImage->GetPixels();
            for (int64_t i = 0; i < grFullArea.Width() * grFullArea.Height(); i++)
            {
                uint32_t col = *pPixels;
                *pPixels = ARGB((255 - ARGB_A(col)), ARGB_R(col), ARGB_G(col), ARGB_B(col));

                pPixels++;
            }


            gAnimator.AddObject(gpOverlay);
        }
    }
    else
    {
        if (gpOverlay)
        {
            gAnimator.KillObject(gpOverlay);
            gpOverlay = nullptr;
        }
    }
}

void Sandbox::InitChildWindows(Sandbox::eSandboxMode mode)
{
    assert(gpControlPanel);     // needs to exist before this

    DeleteAllButControlPanelAndDebugConsole();

    gRegistry["sandbox"]["mode"] = (int32_t)mode;

    if (mode == eSandboxMode::kFloatLinesWin)
	{

		cFloatLinesWin* pWin = new cFloatLinesWin();
		pWin->SetVisible();
		pWin->SetArea(grFullArea);
		gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
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
                        sandboxWins.push_back(pWin);
                    }
                    else
                    {
                        cFloatLinesWin* pWin = new cFloatLinesWin();
                        pWin->SetArea(rSub);
                        gpMainWin->ChildAdd(pWin);
                        sandboxWins.push_back(pWin);
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
                        sandboxWins.push_back(pWin);
                    }
                    else
                    {
                        cLifeWin* pWin = new cLifeWin();
                        pWin->SetArea(rSub);
                        pWin->SetGridSize((int64_t)(rSub.Width() / (4.0)), (int64_t)(rSub.Height() / 4.0));
                        gpMainWin->ChildAdd(pWin);
                        sandboxWins.push_back(pWin);
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
        sandboxWins.push_back(pWin);
    }
	else if (mode == eSandboxMode::kImageProcess)
	{
#ifdef _WIN64
		cProcessImageWin* pWin = new cProcessImageWin();
		pWin->SetArea(grFullArea);
		gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
#endif
	}

    else if (mode == eSandboxMode::kTextTestWin)
    {
        ZRect rArea(grFullArea);
        TextTestWin* pWin = new TextTestWin();
        pWin->SetArea(rArea);
        gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
    }

    else if (mode == eSandboxMode::kParticleSandbox)
    {
        ZRect rArea(grFullArea);
        ParticleSandbox* pWin = new ParticleSandbox();
        pWin->SetArea(rArea);
        gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
    }

    else if (mode == eSandboxMode::kOnePageDoc)
    {
        ZRect rArea(grFullArea);
        OnePageDocWin* pWin = new OnePageDocWin();
        pWin->SetArea(rArea);
        gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
    }

    else if (mode == eSandboxMode::k3DTestWin)
    {
        Z3DTestWin* pWin = new Z3DTestWin();
        pWin->SetArea(grFullArea);

        uint32_t nCol1 = RANDU64(0xff000000, 0xffffffff);
        uint32_t nCol2 = RANDU64(0xff000000, 0xffffffff);

        gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
    }

    else if (mode == eSandboxMode::kChessWin)
    {
        ZChessWin* pWin = new ZChessWin();
        pWin->SetArea(grFullArea);
        //pWin->SetDemoMode(true);
        gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
    }
    else if (mode == eSandboxMode::kTestWin)
    {
        TestWin* pWin = new TestWin();
        pWin->SetArea(grFullArea);
        //pWin->SetDemoMode(true);
        gpMainWin->ChildAdd(pWin);
        sandboxWins.push_back(pWin);
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
            InitChildWindows((Sandbox::eSandboxMode) SH::ToInt(message.GetParam("mode")));
		}
        else if (sType == "toggleconsole")
        {
            gpDebugConsole->SetVisible(!gpDebugConsole->mbVisible);
        }
        else if (sType == "toggleoverlay")
        {
            if (gInput.IsKeyDown(VK_CONTROL))
            {
                gbShowOverlayWin = !gbShowOverlayWin;
                Sandbox::ToggleOverlay();
            }
        }

		return true;
	}
};

MainAppMessageTarget gMainAppMessageTarget;

bool ZFrameworkApp::InitRegistry(std::filesystem::path userDataPath)
{
    filesystem::path appDataPath(userDataPath);
    appDataPath += "/ZSandbox/";

    filesystem::path prefsPath = appDataPath.make_preferred().string() + "prefs.json";
    gRegistry.SetFilename(prefsPath.string());
    if (!gRegistry.Load())
    {
        ZDEBUG_OUT("No registry file at:%s creating path for one.");
        std::filesystem::create_directories(prefsPath.parent_path());
    }

    return true;
}


bool ZFrameworkApp::Initialize(int argc, char* argv[], std::filesystem::path userDataPath)
{
    gGraphicSystem.SetArea(grFullArea);
    if (!gGraphicSystem.Init())
    {
        assert(false);
        return false;
    }

    gResources.Init("res/");// todo, move this define elsewhere?

    ZGUI::ComputeLooks();

    filesystem::path appDataPath(userDataPath);
    appDataPath += "/ZSandbox/";

    std::filesystem::path appPath(argv[0]);
    gRegistry["apppath"] = appPath.parent_path().string();
    gRegistry["appDataPath"] = appDataPath.string();




    gpFontSystem = new ZFontSystem();
    filesystem::path font_cache(appDataPath);
    font_cache.append("font_cache");











    std::filesystem::create_directory(font_cache);    // ensure font cache exists...keep in working folder or move elsewhere?
    gpFontSystem->SetCacheFolder(font_cache.string());
    gpFontSystem->SetDefaultFont(gDefaultTextFont);

    gpFontSystem->Init();


    gpMainWin = new ZMainWin();
    gpMainWin->SetArea(grFullArea);
    gpMainWin->Init();

    Sandbox::InitControlPanel();

    int32_t nMode;
    if (!gRegistry.Get("sandbox", "mode", nMode))
        nMode = Sandbox::kImageProcess;

    Sandbox::InitChildWindows((Sandbox::eSandboxMode) nMode);
    return true;
}

void ZFrameworkApp::Shutdown()
{
    gRegistry.Save();

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