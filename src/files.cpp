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
        string filename = entry.path().stem();
        string filenameWithExtension = entry.path().filename();
        try
        {
            string extension = filenameWithExtension.substr(filenameWithExtension.find("1"));
        }
        catch(...){

        }

        files.push_back(filename);
    }

    for (int i = 0; i < files.size(); i++)
        cout << files[i] << "\n";
    return files;
}
