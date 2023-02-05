#pragma once

#include <string>
#include <list>

namespace ZWinFileDialog
{
    bool ShowLoadDialog(std::string fileTypeDescription, std::string fileTypes, std::string& sFilenameResult);
    bool ShowMultiLoadDialog(std::string fileTypeDescription, std::string fileTypes, std::list<std::string>& resultFilenames);

    bool ShowSaveDialog(std::string fileTypeDescription, std::string fileTypes, std::string& sFilenameResult);
};
