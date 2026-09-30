#include <iostream>
#include <iomanip>

using namespace std;

int main() {
    float arr[3];
    float value[3];

    cout<<"enter rgb values"<< endl;
    for (int i = 0; i < 3; ++i) {
        cin >> arr[i];
    }

    for (int i = 0; i < 3; ++i) {
        value[i] = arr[i] / 255.0f;
    }

    for (int i = 0; i < 3; ++i) {
        cout << value[i] << (i < 2 ? ", " : "");
    }
    cout << endl;

    return 0;
}