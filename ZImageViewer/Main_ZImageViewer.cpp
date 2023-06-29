#include "Main_ZImageViewer.h"
#include "helpers/StringHelpers.h"
#include "helpers/Registry.h"
#include "helpers/CommandLineParser.h"

#include "ZWinDebugConsole.h"
#include "Resources.h"
#include "ImageViewer.h"


using namespace std;

// Global Variables:
extern ZRect            grFullArea;
ZMainWin*               gpMainWin;
ZWinDebugConsole*       gpDebugConsole;
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
ZWin*                   gpCaptureWin = nullptr;
ZWin*                   gpMouseOverWin = nullptr;
ZPoint                  gLastMouseMove;
REG::Registry           gRegistry;


void ZImageViewer::InitControlPanel()
{

    int64_t panelW = grFullArea.Width() / 8;    
    int64_t panelH = grFullArea.Height() / 2;
    ZRect rControlPanel(grFullArea.right - panelW, 0, grFullArea.right, panelH);     // upper right for now

    gpDebugConsole = new ZWinDebugConsole();
    gpDebugConsole->SetArea(ZRect(grFullArea.left, grFullArea.top, grFullArea.right, grFullArea.Height() / 2));
    gpMainWin->ChildAdd(gpDebugConsole, false);

}

void ZImageViewer::DeleteAllButControlPanelAndDebugConsole()
{
    gpMainWin->ChildRemove(gpDebugConsole);
    gpMainWin->ChildDeleteAll();
}


void ZImageViewer::InitChildWindows()
{
    DeleteAllButControlPanelAndDebugConsole();

    string sImageFilename = gRegistry.GetValue("zimageviewer", "imageviewer_filename");

    ImageViewer* pWin = new ImageViewer();
    pWin->SetArea(grFullArea);
    gpMainWin->ChildAdd(pWin);
    if (!sImageFilename.empty())
        pWin->ViewImage(sImageFilename);

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


bool ZFrameworkApp::InitRegistry(std::filesystem::path userDataPath)
{
    filesystem::path appDataPath(userDataPath);
    appDataPath += "/ZImageViewer/";

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

    filesystem::path appDataPath(userDataPath);
    appDataPath += "/ZImageViewer/";


    std::filesystem::path appPath(argv[0]);
    gRegistry["apppath"] = appPath.parent_path().string();
    gRegistry["appDataPath"] = appDataPath.string();

    std::string sImageFilename;
    CLP::CommandLineParser parser;
    parser.RegisterAppDescription("ZImageViewer");
    parser.RegisterParam(CLP::ParamDesc("PATH", &sImageFilename, CLP::kOptional | CLP::kPositional, "Filename of image to load."));

    if (!parser.Parse(argc, argv))
    {
        ZERROR("ERROR parsing commandline.");
        return false;
    }

    gRegistry["zimageviewer"]["imageviewer_filename"] = sImageFilename;


    filesystem::path resourcesPath(appPath.parent_path());
    resourcesPath.append("res/default_resources/");
    gResources.Init(resourcesPath.string());// todo, move this define elsewhere?

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

    ZImageViewer::InitControlPanel();

    ZImageViewer::InitChildWindows();

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