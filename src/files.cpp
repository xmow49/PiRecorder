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
        files.push_back(entry.path().filename());

    for (int i = 0; i < files.size(); i++)
        cout << files[i] << "\n";
    return files;
}
