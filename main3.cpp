#include <iostream>
#include <string>
using namespace std;

// Driver Code
int main()
{
    string str = "2009, GeeksforGeeks_founded";
    string str1 = "0x6C1";
    string str2 = "-10010010101";

    // Calling stoi() for all strings.
    int num = stoi(str);
    int num_hex = stoi(str1, nullptr, 16);
    int num_bin = stoi(str2, nullptr, 2);

    // printing converted values
    cout << str << ": " << num << endl;
    cout << str1 << ": " << num_hex << endl;
    cout << str2 << ": " << num_bin << endl;

    return 0;
}