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

using namespace std;

map<string,pair<string,string>>srcDestMap;
map<string,int>flitStart;
ofstream logFile("log.txt");
ofstream reportFile("report.txt");
map<int,vector<vector<string>>>logmap;
vector<vector<string>>finalpath;

multimap<int, tuple<string, string, string>> extract(string& filename) {
    int errors = 0;
    if(!logFile.is_open()){
        perror("Error opening the log file");
    }

    ifstream file(filename);
    if(!file.is_open()){
        errors++;
        logFile << "Error opening traffic file" << endl;
        logFile.close();
    }

    string line;
    multimap<int, tuple<string, string, string>> trafficData;

    int prev_cycle = -1, next_cycle, src, dst, head_flit_count = 0, body_flit_count = 0, tail_flit_count = 0, line_number = 0;
    while (getline(file, line)) {
        istringstream iss(line);
        string word; 
        vector<string> words;

        while (iss >> word) {
            words.push_back(word);
        }

        line_number++;
        if(stoi(words[0]) < prev_cycle){
            logFile << "Error in line " << line_number << " : Cycle number out of order." << endl;
            errors++;
            continue;
        }
        prev_cycle = stoi(words[0]);
        if (words.size() == 3) {
            string headFlit = words[2].substr(0, 32);
            string sourceID = headFlit.substr(0, 16);
            string destinationID = headFlit.substr(16, 16);
            string bodyFlit = words[2].substr(32, 32);
            string tailFlit = words[2].substr(64, 32);
            tuple<string, string, string> dataTuple1(sourceID, destinationID, headFlit);
            tuple<string, string, string> dataTuple2(sourceID, destinationID, bodyFlit);
            tuple<string, string, string> dataTuple3(sourceID, destinationID, tailFlit);
            trafficData.insert(make_pair(stoi(words[0]), dataTuple1));
            trafficData.insert(make_pair(stoi(words[0]), dataTuple2));
            trafficData.insert(make_pair(stoi(words[0]), dataTuple3));
        } else {
            string flitType = words[3].substr(words[3].length() - 2);
            cout <<flitType<< endl;
            if(flitType == "00") head_flit_count++;
            else if(flitType == "01") body_flit_count++;
            else if(flitType == "10") tail_flit_count++;
            else{
                logFile << "Error in line " << line_number << " : Flit type " << flitType << " is not defined." << endl;
                errors++;
            }
            if(body_flit_count > head_flit_count || tail_flit_count > body_flit_count){
                logFile << "Error in line " << line_number << " : Flit out of order." << endl;
                errors++;
            }
            // Create a tuple with extracted data
            cout<<words[0]<<words[1]<<words[2]<<words[3]<<endl;
            tuple<string, string, string> dataTuple(words[1], words[2], words[3]);
            // Insert the tuple into the multimap using insert method
            trafficData.insert(make_pair(stoi(words[0]), dataTuple));
            // cout<<dataTuple;
        }
    }
   for (const auto& data : trafficData) {
        int cycle = data.first;
        auto values = data.second;
        cout << "Cycle: " << cycle << " | SourceID: " << get<0>(values)
             << " | Destination: " << get<1>(values) << " | flitBinary: " << get<2>(values) <<endl;
             }

    // file.close();
    return trafficData;
}

int getFreq(){
    ifstream delayFile("Delays.txt");
    if (!delayFile.is_open()) {
        cerr << "Unable to open the config file." << endl;
        return 1; 
    }
    int maxDelay = -1;
    string line; 
    while (getline(delayFile, line)) {
        size_t pos = line.find(" = ");
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            int value = stoi(line.substr(pos + 3));
            maxDelay = max(maxDelay, value);
        }
    }
    delayFile.close();
    double clockFreq = 1000/maxDelay;
    cout << "Clock cycle time = " << maxDelay << " ms" << endl;
    cout << "Clock frequency = " << clockFreq << " Hz" << endl;
    return maxDelay;
}
class Router;
class Port{             //address of next router and current with its input output
    public:
        Router *router,*parentRouter;
        string input,output,direction;
        Port(Router *curr,Router *newparent,string dir){
            input="";
            output="";
            router=curr;
            parentRouter=newparent;
            direction=dir;
        }
};

class Router{
    public:
        Port *north=NULL,*south=NULL,*east=NULL,*west=NULL,*PE=NULL;
        string id;
        int IOdelay,switchAllocatorDelay,xbarDelay;
        Router(string nid){
            id=nid;
        }
        void setRouter(Router *nnorth,Router *nsouth,Router *neast,Router *nwest,Router *currRouter){  
            if(nnorth) north=new Port(nnorth,currRouter,"north");
            if(nsouth)south=new Port(nsouth,currRouter,"south");
            if(neast) east=new Port(neast,currRouter,"east");
            if(nwest) west=new Port(nwest,currRouter,"west");
            PE=new Port(nullptr,currRouter,".");
        }
};  
vector<Port *>getValidPorts(string prevRouter,Port *currRouter,Router *startRouter){   // gives all the three DIrectrouters and the not initialized ports will be null in order of list 
    vector<Port *>ans(4,nullptr);
    Router *parent=startRouter;
    if(currRouter){
        parent=currRouter->router;
    }
    
    if(parent->north && parent->north!=currRouter){
        ans[0]=parent->north;
    }
    if(parent->south && parent->south!=currRouter){
        ans[1]=parent->south;
    }
    if(parent->east && parent->east!=currRouter){
        ans[2]=parent->east;
    }
    if(parent->west && parent->west!=currRouter){
        ans[3]=parent->west;
    }
    if(currRouter){
        if(currRouter->direction=="south")ans[0]=nullptr;
        if(currRouter->direction=="north")ans[1]=nullptr;
        if(currRouter->direction=="west")ans[2]=nullptr;
        if(currRouter->direction=="east")ans[3]=nullptr;
    }
    return ans;
}

Port *getNextXYRouter(Port *currentRouter,string destination,string prevRouter,Router *startRouter){     //switch allocator
    if(currentRouter && currentRouter->router->id==destination){
        return currentRouter;}
    vector<Port *>validPorts=getValidPorts(prevRouter,currentRouter,startRouter);
    Port *temp=validPorts[0];  //search in north
    while(temp){
        // cout<<"entered north ";
        if(temp->router->id==destination){   //if found return address of router
            // cout<<"returning north";
            return validPorts[0];
        }
        temp=temp->router->north;
    }
    temp=validPorts[1];   //search in south
    while(temp){
        if(temp->router->id==destination){
            return validPorts[1];
        }
        temp=temp->router->south;
    }   

    // if currid-1 %3 < finalid-1%3 left else right if equal then same column
    if(validPorts[2]){   //check only east
        if(getNextXYRouter(validPorts[2],destination,"west",nullptr)){
            // cout<<"returned east of ";
            // if(currentRouter && currentRouter->parentRouter)cout<<"currently in "<<currentRouter->parentRouter->id<<endl;
            return validPorts[2];}
    }
    if(validPorts[3]){
        if(getNextXYRouter(validPorts[3],destination,"east",nullptr)){
            // if(currentRouter && currentRouter->parentRouter)cout<<"currently in "<<currentRouter->parentRouter->id<<endl;
            return validPorts[3];}
    }
    return NULL;
}
Port * getOppositePort(Port *currPort){
    string currDirection=currPort->direction;
    if(currDirection=="east"){
        return currPort->router->west;
    }
    if(currDirection=="west"){
        return currPort->router->east;
    }
    if(currDirection=="north"){
        return currPort->router->south;
    }
    if(currDirection=="south")return currPort->router->north;
    return nullptr;
}

void congestionCheck(string start,int &cycle,string state){
    while(1){
        bool flag=false;
        for(auto allpackets:logmap[cycle]){
            if(allpackets[0]==start && allpackets[1]==state){
                flag=true;
                cycle++;
            }
        }
        if(flag==false){
            break;
        }
    }
}

vector<Port*> sendPacketXYRouting(string start,string destination,Router *Router1,int startcycle){
    vector<Port*>path;
    //------------------decoding buffer
    string flitBinary=Router1->PE->input;    //copy data from input to direction output
    // currRouter->output=data;
    string flitType = flitBinary.substr(flitBinary.size() - 2);
    if(flitType=="00")flitType="header";
    if(flitType=="01")flitType="tail";
    if(flitType=="10")flitType="body";
    Port *currRouter=getNextXYRouter(nullptr,destination,"",Router1); //  find the directRouter of any 
    congestionCheck(start,startcycle,currRouter->direction+"buffer");
    logmap[startcycle].push_back({start,currRouter->direction+"buffer",flitBinary,flitType});
    startcycle++;

    //------------------decoding 
    congestionCheck(start,startcycle,"SA");
    logmap[startcycle].push_back({start,"SA",flitBinary,""});
    path.push_back(currRouter);
    startcycle++;
    // //data transfer XBAR----------------------------------
    Port *oppPort=getOppositePort(currRouter);    //copy data from input to direction output
    currRouter->output=flitBinary;
    oppPort->input=flitBinary;
    logmap[startcycle].push_back({start,"XBAR",flitBinary,""});

    // data transfer XBAR----------------------------------

    while(true){
        Port *nextOppPort=getOppositePort(currRouter);
        string flitdata=nextOppPort->input;
        flitType=flitdata.substr(flitdata.size() - 2);
        if(flitType=="00")flitType="header";
        if(flitType=="01")flitType="tail";
        if(flitType=="10")flitType="body";
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"buffer",flitdata,flitType});
        Port *nextRouter=getNextXYRouter(currRouter,destination,".",nullptr);
        if(currRouter->router->id==destination){
            nextRouter=currRouter->router->PE;
            startcycle++;
            logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,""});
            nextRouter->input=flitdata;
            startcycle++;
            logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,""});
            break;
        }
        path.push_back(nextRouter);
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,""});
        oppPort=getOppositePort(nextRouter);    //copy data from input to direction output
        currRouter->output=flitBinary;
        oppPort->input=flitBinary;
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,""});
        currRouter=nextRouter;
    }
    return path;
} 
void sendPacket(map<string,Router *>RouterMap){
    string filename = "bonustraffic.txt"; 
    multimap<int, tuple<string, string>>trafficData=extract(filename);
    vector<Port*>path;
    
    for (auto& data : trafficData) {
        int cycle = data.first;
        auto values = data.second;
        string source = get<0>(values);
        string flit = get<1>(values);
        string binaryPayload = flitBinary.substr(2, flitBinary.size()); // Extract remaining bits for payload
        string flitType = flitBinary.substr(flitBinary.size() - 2);
        if(flitType=="00"){
            string currVal=flitBinary.substr(2,flitBinary.size()-4);
            string binarysrcNode = currVal.substr(currVal.size() - 15);
            int srcNode = stoi(binarysrcNode, 0, 2);
            string binarydestNode = currVal.substr(0, currVal.size() - 15);
            int destNode = stoi(binarydestNode, 0, 2);
            cout<<srcNode<<" "<<destNode<<endl;
            srcDestMap[binaryPayload]={to_string(srcNode),to_string(destNode)};
        }
            flitStart[binaryPayload]=cycle;
            RouterMap[source]->PE->input=binaryPayload;
            path=sendPacketXYRouting(source,destination,RouterMap[source],cycle);
            string currPath="";
            for(auto it:path){currPath+=it->parentRouter->id + " -> ";
            currPath+=it->direction;}
            currPath+=destination;
            finalpath.push_back({currPath,source,destination,flitBinary});
            // cout<<destination<<endl;
    }
}

void printLogMap(map<int, vector<vector<string>>>& logmap,int clk) {
    logFile<<left<<setw(15)<<"Cycle No"<<setw(15)<<"No of Flits "<<setw(20)<<"Router No"<<setw(20)<<"Stage"<<setw(40)<<"Flit"<<setw(15)<<"Type"<<endl;
    logFile<<string(120, '-')<<endl;

    for (auto& data : logmap) {
        for (auto& vec : data.second) {
            logFile<<left<<setw(15)<<data.first<<setw(15)<<data.second.size()<<setw(20)<<vec[0]<<setw(20)<<vec[1]<<setw(40)<<vec[2]<<setw(15)<<vec[3]<<endl;
        }
    }
}

void printReport(map<int, vector<vector<string>>>& logmap,int clk) {
    reportFile<<left<<"Router No"<< setw(25)<<"     Stage"<<setw(45)<<"Flit"<<setw(15)<<"Delay"<<endl;
    reportFile<<string(95, '-')<<endl;

    for (auto& data : logmap) {
        for (auto& vec : data.second) {
            reportFile<<left<<setw(15)<<vec[0]<<setw(15)<<vec[1]<<setw(35)<<vec[2]<<clk*(data.first-(flitStart[vec[2]]))<<endl;
        }
    }
}

void printPaths(vector<vector<string>>& paths, std::ofstream& logFile) {
    logFile<<string(120, '-')<<endl;
    logFile << "Paths:" << std::endl;
    for (auto& path : paths) {
        logFile << "Path: " << path[0] << " source: " << path[1] << " destination: " << path[2]
                << " flit: " << path[3] << endl;
    }
}

int main(int argc, char* argv[]){
    if (argc != 2) {
        cerr<<"Usage: "<<argv[0]<<" <routing_type>"<<endl;
        return 1;
    }

    string routingType = argv[1]; 

    if (routingType != "XY") {
        cerr << "Invalid routing type: " << routingType << std::endl;
        return 1;
    }

    if (!logFile.is_open()) {
        cerr << "Failed to open the log file." << endl;
        return 1;
    }
    if (!reportFile.is_open()) {
        cerr << "Failed to open the log file." << endl;
        return 1;
    }
    reportFile.close();
    reportFile.open("report.txt", ios::app);
    logFile.close();
    logFile.open("log.txt", ios::app);
    int clkRate=getFreq();
    Router *router_1=new Router("1");
    Router *router_2=new Router("2");
    Router *router_3=new Router("3");
    Router *router_4=new Router("4");
    Router *router_5=new Router("5");
    Router *router_6=new Router("6");
    Router *router_7=new Router("7");
    Router *router_8=new Router("8");
    Router *router_9=new Router("9");
    router_1->setRouter(nullptr,router_4,router_2,nullptr,router_1);   //N S E W current
    router_2->setRouter(nullptr,router_5,router_3,router_1,router_2);
    router_3->setRouter(nullptr,router_6,nullptr,router_2,router_3);

    router_4->setRouter(router_1, router_7, router_5, nullptr, router_4);
    router_5->setRouter(router_2, router_8, router_6, router_4, router_5);
    router_6->setRouter(router_3, router_9, nullptr, router_5, router_6);

    router_7->setRouter(router_4, nullptr, router_8, nullptr, router_7);
    router_8->setRouter(router_5, nullptr, router_9, router_7, router_8);
    router_9->setRouter(router_6, nullptr, nullptr, router_8, router_9);
    map<string,Router *>RouterMap;
    RouterMap["1"]=router_1;
    RouterMap["2"]=router_2;
    RouterMap["3"]=router_3;
    RouterMap["4"]=router_4;
    RouterMap["5"]=router_5;
    RouterMap["6"]=router_6;
    RouterMap["7"]=router_7;
    RouterMap["8"]=router_8;
    RouterMap["9"]=router_9;
    sendPacket(RouterMap);
    printLogMap(logmap,clkRate);
    printPaths(finalpath, logFile);
    printReport(logmap, clkRate);
    for(auto it:logmap){
        cout<<" cycle:"<<it.first<<endl;
            for(auto it2:it.second){
                for(auto it3:it2)cout<<" "<<it3;
            }
            cout<<endl;
        cout<<endl;
    }
    return 0;
}


// 1  2  3
// 4  5  6
// 7  8  9