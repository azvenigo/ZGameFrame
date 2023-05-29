#include "Main_ZImageViewer.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"

#include "ZWinDebugConsole.h"
#include "Resources.h"
#include "ImageViewer.h"


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
ZWin*                   gpCaptureWin = nullptr;
ZWin*                   gpMouseOverWin = nullptr;
ZPoint                  gLastMouseMove;
REG::Registry           gRegistry;


void ZImageViewer::InitControlPanel()
{

    int64_t panelW = grFullArea.Width() / 8;    
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(grFullArea.right - panelW, 0, grFullArea.right, panelH);     // upper right for now

/*    gpControlPanel = new ZWinControlPanel();
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

    gpControlPanel->FitToControls();

    gpMainWin->ChildAdd(gpControlPanel);*/


    gpDebugConsole = new ZWinDebugConsole();
    gpDebugConsole->SetArea(ZRect(grFullArea.left, grFullArea.top, grFullArea.right, grFullArea.Height() / 2));
    gpMainWin->ChildAdd(gpDebugConsole, false);

}

void ZImageViewer::DeleteAllButControlPanelAndDebugConsole()
{
//    gpMainWin->ChildRemove(gpControlPanel);
    gpMainWin->ChildRemove(gpDebugConsole);
    gpMainWin->ChildDeleteAll();
}


void ZImageViewer::InitChildWindows()
{
//    assert(gpControlPanel);     // needs to exist before this

    DeleteAllButControlPanelAndDebugConsole();

    string sImageFilename = gRegistry.GetValue("zimageviewer", "imageviewer_filename");

    ImageViewer* pWin = new ImageViewer();
    pWin->SetArea(grFullArea);
    gpMainWin->ChildAdd(pWin);
    if (!sImageFilename.empty())
        pWin->ViewImage(sImageFilename);

//    gpMainWin->ChildAdd(gpControlPanel);
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
            ZImageViewer::InitChildWindows();
		}
        else if (sType == "toggleconsole")
        {
            gpDebugConsole->SetVisible(!gpDebugConsole->IsVisible());
        }

		return true;
	}
};

MainAppMessageTarget gMainAppMessageTarget;

void ZImageViewer::InitializeFonts()
{
    if (gpFontSystem)
        return;

    gpFontSystem = new ZFontSystem();

    filesystem::path appDataPath;
    assert(gRegistry.contains("appdata"));  // required environment var
    gRegistry["appdata"].get_to(appDataPath);

    filesystem::path font_cache(appDataPath);
    font_cache.append("font_cache");


    std::filesystem::create_directory(font_cache);    // ensure font cache exists...keep in working folder or move elsewhere?
    gpFontSystem->SetCacheFolder(font_cache.string());
    gpFontSystem->SetDefaultFont(gDefaultTextFont);

    gpFontSystem->Init();
}

bool ZImageViewer::Initialize()
{
    gGraphicSystem.SetArea(grFullArea);
    if (!gGraphicSystem.Init())
    {
        assert(false);
        return false;
    }

    filesystem::path appPath;
    assert(gRegistry.contains("apppath"));  // required environment var
    gRegistry["apppath"].get_to(appPath);

    appPath.append("res/default_resources/");

    gResources.Init(appPath.string());// todo, move this define elsewhere?

    InitializeFonts();
    gpMainWin = new ZMainWin();
    gpMainWin->SetArea(grFullArea);
    gpMainWin->Init();

    InitControlPanel();

    InitChildWindows();
    return true;
}

void ZImageViewer::Shutdown()
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