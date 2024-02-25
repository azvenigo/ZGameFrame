#pragma once

#include <string>
#include <list>

namespace ZWinFileDialog
{
    bool ShowLoadDialog(std::string fileTypeDescription, std::string fileTypes, std::string& sFilenameResult, std::string sDefaultFolder = "");
    bool ShowMultiLoadDialog(std::string fileTypeDescription, std::string fileTypes, std::list<std::string>& resultFilenames, std::string sDefaultFolder = "");

    bool ShowSelectFolderDialog(std::string& sFilenameResult, std::string sDefaultFolder = "", const std::string& sTitle = "");

    bool ShowSaveDialog(std::string fileTypeDescription, std::string fileTypes, std::string& sFilenameResult, std::string sDefaultFolder = "");
};
