#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

int main() {
    ifstream input;
    input.open("t10k-images.idx3-ubyte", ios::in | ios::binary);

    ifstream labels;
    labels.open("t10k-labels.idx1-ubyte", ios::in | ios::binary);

    char byte[28 * 28];
    input.read(byte, 4 * 4);

    for (int i = 0; i < 16; i++) {
        cout << setfill('0') << setw(2) << hex << (unsigned int) byte[i] << " ";
    }
    cout << endl;

    input.read(byte, 28 * 28);
    for (int i = 0; i < 28 * 28; i++) {
        unsigned int ib = (unsigned char) byte[i];
        double db = (double) (unsigned char) byte[i];
        db = db * 2.0 / 255.0 - 1.0;
        cout << dec << ib << " : " << db << ", ";
    }
    cout << endl;
    
    return 0;
}