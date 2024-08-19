#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

static const regex QUOTATION_MARKS(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
static const regex SANGLE_BRACKETS(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

bool Preprocess(const path& in_file, ofstream& output, const vector<path>& include_directories);

bool CheckDirectories(const path& in_file, ofstream& output, const vector<path>& include_directories, const string& str_m, const int& num_of_cur_str) {
    if (!any_of(include_directories.begin(), include_directories.end(), [&](path f_path) { return Preprocess(f_path / str_m, output, include_directories); })) {
        cout << "unknown include file " << str_m << " at file " << in_file.string() << " at line " << num_of_cur_str << endl;
        return false;
    }
    return true;
}

bool Preprocess(const path& in_file, ofstream& output, const vector<path>& include_directories) {
    if (ifstream i_stream = ifstream(in_file)) {
        string cur_str;
        smatch m;
        for(int num_of_cur_str = 1; getline(i_stream, cur_str); ++num_of_cur_str){
            if (regex_match(cur_str, m, QUOTATION_MARKS)) {
                if (!Preprocess(in_file.parent_path() / string(m[1]), output, include_directories) && !CheckDirectories(in_file, output, include_directories, string(m[1]), num_of_cur_str)) {
                    return false;
                }
            }
            else if (regex_match(cur_str, m, SANGLE_BRACKETS)){
                if (!CheckDirectories(in_file, output, include_directories, string(m[1]), num_of_cur_str)) {
                    return false;
                }
            }
            else {
                output << cur_str << endl;
            }
        }
        return true;
    }
    return false;
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    if (ifstream i_stream = ifstream(in_file)) {
        ofstream o_stream = ofstream(out_file);
        return Preprocess(in_file, o_stream, include_directories);
    }
    return false;
    
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}