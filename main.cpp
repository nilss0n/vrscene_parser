#include <iostream>
#include <fstream>
#include <chrono>
#include "vrscene_parser.h"


using namespace std::chrono;
static auto current_time() {
    return duration_cast<microseconds>(system_clock::now().time_since_epoch());
}

int main(int, char**) {

    std::ifstream ifs ("../merged.vrscene", std::ifstream::in);
    std::string str((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    std::string_view string(str);
    ifs.close();
    auto start = current_time();
    if(auto scene = parse_vrscene(string); scene) {
        auto vrscene = *scene;
        auto dur = current_time() - start;

        std::cout << "SUCCESS " << dur.count() << std::endl;
    }
    return 0;
}
