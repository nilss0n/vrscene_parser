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
        auto i = 0;
        for(auto&& c : vrscene.comments) {
            std::cout << c;
        }
        for(auto&& i : vrscene.includes) {
            std::cout << i;
        }

        for(auto&& p : vrscene.plugins) {
            if(i++ >= 10) break;
            std::cout << p.type << " " << p.name << std::endl;
            for(auto&& it : p.attributes) {
                std::cout << "\t" << it.first << "=" << it.second << std::endl;
            }
        }
    }
    return 0;
}
