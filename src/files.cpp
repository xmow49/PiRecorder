#include <iostream>
#include <string>
#include <vector>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
using namespace std;

vector<string> getFiles(string path)
{
    vector<string> files;
    for (const auto &entry : fs::directory_iterator(path))
    {
        string filename = entry.path().filename().string();
        if (filename.find(".wav") != std::string::npos)
        {
            files.push_back(filename);
        }
    }
    return files;
}
