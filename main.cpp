#include <iostream>
#include "map.hpp"

int main() {
    sjtu::map<int, int> m;
    m[1] = 2;
    m[3] = 4;
    for (auto it = m.begin(); it != m.end(); ++it) {
        std::cout << it->first << " " << it->second << std::endl;
    }
    return 0;
}
