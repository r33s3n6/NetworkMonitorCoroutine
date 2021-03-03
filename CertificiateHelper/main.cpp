#include <iostream>
using namespace std;



int main(int argc, char** argv) {
	if (argc > 1) {
		system((string("certmgr /c /add ")+argv[1]+" /s root").c_str());
	}
	
}