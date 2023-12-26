#include <vector>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <tuple>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <random>

using namespace std;
void printGraph1(map<pair<string,string>,int>flitCount){
    int arr[21] = {0};
    string connections[21] = {"1PE", "2PE", "3PE", "4PE", "5PE", "6PE", "7PE", "8PE", "9PE", "12", "23", "45", "56", "78", "89"
    "14", "25", "36", "47", "58", "69"};
    for(auto i:flitCount){
        if(i.first.second == "PE") arr[stoi(i.first.first.substr(0, 1)) - 1]++;
        else if(i.first.second == "east"){
            if(stoi(i.first.first) < 3) arr[stoi(i.first.first) + 8]++;
            else if(stoi(i.first.first) < 6) arr[stoi(i.first.first) + 7]++;
            else if(stoi(i.first.first) < 9) arr[stoi(i.first.first) + 6]++;
        }
        else if(i.first.second == "west"){
            if(stoi(i.first.first) <= 3) arr[stoi(i.first.first) + 7]++;
            else if(stoi(i.first.first) <= 6) arr[stoi(i.first.first) + 6]++;
            else if(stoi(i.first.first) <= 9) arr[stoi(i.first.first) + 5]++;
        }
        else if(i.first.second == "north") arr[stoi(i.first.first) + 11]++;
        else if(i.first.second == "south") arr[stoi(i.first.first) + 14]++;
    }
    ofstream dataFile("data.txt", ios::app);
    dataFile << "Connections ";
    for(int i = 0; i < 21; i++){
        dataFile << connections[i];
    } dataFile << endl;

    dataFile << "FlitCount";
    for(int i = 0; i < 21; i++){
        dataFile << arr[i];
    } dataFile << endl;

    dataFile.close();
    return;
}

void printGraphs(map<pair<string,string>,int>flitCount){
    printGraph1(flitCount);
    // printGraph2();
    system("python graph.py");
    return;
}
    int main(){
        map<pair<string,string>,int>flitCount;
        flitCount[make_pair("1","east")]++;
        flitCount[make_pair("2","west")]++;
        printGraphs(flitCount);
    }