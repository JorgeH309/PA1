#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <binary_file>\n";
        return 1;
    }

    const char* filename = argv[1];
    std::ifstream fin(filename, std::ios::binary);
    if (!fin) {
        std::cerr << "Error: cannot open file " << filename << "\n";
        return 1;
    }

    while (true) {
        int a;
        double b;

        // Read int
        fin.read(reinterpret_cast<char*>(&a), sizeof(a));
        if (fin.eof()) break;  // stop if nothing left

        // Read double
        fin.read(reinterpret_cast<char*>(&b), sizeof(b));
        if (fin.eof()) {
            std::cerr << "Warning: file ended unexpectedly after int\n";
            break;
        }

        std::cout << "Int: " << a << ", Double: " << b << "\n";
    }

    fin.close();
    return 0;
}
