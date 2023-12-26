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
int routeType;
int currentFlitId;
int mode;
map<pair<string,string>,int>flitCount;
map<pair<string,string>,string>srcDestMap;
map<vector<string>,vector<pair<int,int>>>flitStart;   //src destn packet   time
// map<string,int>flitStart;
ofstream logFile("log.txt");
ofstream logFile_pvs("log_pvs.txt");
ofstream reportFile("report.txt");
ofstream reportFile_pvs("report_pvs.txt");
map<int,vector<vector<string>>>logmap;
// map<int,vector<vector<string>>>reportMap;
vector<vector<string>>finalpath;
double cycleTime;
map<vector<string>,int>idMap;   
vector<vector<string>>packets;   //head body tail src destn
multimap<int, tuple<string, string, string>>trafficData;

void printGraph1(string filename){
    int arr[21] = {0};
    string connections[21] = {"1PE", "2PE", "3PE", "4PE", "5PE", "6PE", "7PE", "8PE", "9PE", "12", "23", "45", "56", "78", "89",
    "14", "25", "36", "47", "58", "69"};
    for(auto i:flitCount){
        if(i.first.second == "PE") arr[stoi(i.first.first.substr(0, 1)) - 1] += i.second;
        else if(i.first.second == "east"){
            if(stoi(i.first.first) < 3) arr[stoi(i.first.first) + 8] += i.second;
            else if(stoi(i.first.first) < 6) arr[stoi(i.first.first) + 7] += i.second;
            else if(stoi(i.first.first) < 9) arr[stoi(i.first.first) + 6] += i.second;
        }
        else if(i.first.second == "west"){
            if(stoi(i.first.first) <= 3) arr[stoi(i.first.first) + 7] += i.second;
            else if(stoi(i.first.first) <= 6) arr[stoi(i.first.first) + 6] += i.second;
            else if(stoi(i.first.first) <= 9) arr[stoi(i.first.first) + 5] += i.second;
        }
        else if(i.first.second == "north") arr[stoi(i.first.first) + 11] += i.second;
        else if(i.first.second == "south") arr[stoi(i.first.first) + 14] += i.second;
    }
    ofstream dataFile(filename, ios::app);
    dataFile << "Connections ";
    for(int i = 0; i < 21; i++){
        dataFile << connections[i] << " ";
    } dataFile << endl;

    dataFile << "FlitCount ";
    for(int i = 0; i < 21; i++){
        dataFile << arr[i] << " ";
    } dataFile << endl;

    dataFile.close();
    return;
}

void printGraphsPVA(){
    ofstream dataFile("dataPVA.txt");
    dataFile.close();
    printGraph1("dataPVA.txt");
    // printGraph2();
    // system("python graph.py");
    return;
}
void printGraphsPVS(){
    ofstream dataFile("dataPVS.txt");
    dataFile.close();
    printGraph1("dataPVS.txt");
    // printGraph2();
    // system("python graph.py");
    return;
}




multimap<int, tuple<string, string, string>> extract(string& filename) {
    int errors = 0;
    ifstream file(filename);
    string line;
    multimap<int, tuple<string, string, string>> trafficData, t2;
    
    int prev_cycle = -1, next_cycle, src, dst, head_flit_count = 0, body_flit_count = 0, tail_flit_count = 0, line_number = 0;
    // int cnt=0
    while (getline(file, line)) {
        istringstream iss(line);
        string word;
        vector<string> words;
        while (iss >> word) {
            words.push_back(word);
        }
        if (words.size() == 0) break;

        line_number++;
        if (stoi(words[0]) < prev_cycle) {
            logFile << "Error in line " << line_number << " : Cycle number out of order." << endl;
            errors++;
            continue;
        }
        prev_cycle = stoi(words[0]);

        if (words.size() == 3) {
            string headFlit = words[2].substr(0, 32);
            string sourceID = headFlit.substr(0, 16);
            string destinationID = headFlit.substr(16);
            string bodyFlit = words[2].substr(32, 32);
            string tailFlit = words[2].substr(64, 32);

            int srcNode = stoi(sourceID, 0, 2);
            int destNode = stoi(destinationID, 0, 2);
            tuple<string, string, string> dataTuple1(to_string(srcNode), to_string(destNode), headFlit);
            tuple<string, string, string> dataTuple2(to_string(srcNode), to_string(destNode), bodyFlit);
            tuple<string, string, string> dataTuple3(to_string(srcNode), to_string(destNode), tailFlit);
            trafficData.insert(make_pair(stoi(words[0]), dataTuple1));
            trafficData.insert(make_pair(stoi(words[0]), dataTuple2));
            trafficData.insert(make_pair(stoi(words[0]), dataTuple3));
            packets.push_back({headFlit,bodyFlit,tailFlit,to_string(srcNode),to_string(destNode)});
        } else {
            string flitType = words[3].substr(words[3].length() - 2);
            if (flitType == "00") head_flit_count++;
            else if (flitType == "01") body_flit_count++;
            else if (flitType == "10") tail_flit_count++;
            else {
                logFile << "Error in line " << line_number << " : Flit type " << flitType << " is not defined." << endl;
                errors++;
                continue;
            }
            if (body_flit_count > head_flit_count || tail_flit_count > body_flit_count) {
                logFile << "Error in line " << line_number << " : Flit out of order." << endl;
                errors++;
                continue;
            }
            
            if(flitType=="00"){
                packets.push_back({words[3],"X","X",words[1],words[2]});
            }
            else if(flitType=="01"){
                bool errflag=false;
                for(auto &it:packets){
                    if(it[1]=="X"){
                        if(it[3]==words[1] && it[4]==words[2]){
                            it[1]=words[3];
                            break;
                        }
                        else{
                            errflag=true;
                            reportFile<<"ERROR IN LINE NUMBER" << line_number<<"BODY IS SEND TO SAME SRC AND DEST BEFORE HEAD"<<endl;
                            break;
                        }
                    }
                }
                if(errflag==true)continue;
            }
            else{
                bool errflag=false;
                for(auto &it:packets){
                    if(it[2]=="X"){
                        if(it[3]==words[1] && it[4]==words[2]){
                            it[2]=words[3];
                            
                            break;
                        }
                        else{
                            reportFile<<"ERROR IN LINE NUMBER" << line_number<<"TAIL IS SEND TO SAME SRC AND DEST BEFORE BODY"<<endl;
                            errflag=true;
                            break;
                        }
                    }
                }
                if(errflag==true)continue;
            }
            // cout<<"--------------------"<<flitType<<endl;
            // for(auto it:packets){
            //     cout<<get<0>(it)<<get<1>(it)<<get<2>(it)<<endl;
            // }
            // cout<<"--------------------"<<endl;
            tuple<string, string, string> dataTuple(words[1], words[2], words[3]);
            trafficData.insert(make_pair(stoi(words[0]), dataTuple));
        }
    }


    file.close();
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
    return (double)maxDelay;
}

vector<int> Delays(){
    ifstream delayFile("Delays.txt");
    if (!delayFile.is_open()) {
        cerr << "Unable to open the config file." << endl;
        exit(1); 
    }

    string line;
    vector<int> d;
    for(int i = 0 ; i < 3; i++){
        getline(delayFile, line);
        size_t pos = line.find(" = ");
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            int value = stoi(line.substr(pos + 3));
            d.push_back(value);
        }  
    } 
    delayFile.close();
    return d;
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
        double IOdelay,switchAllocatorDelay,xbarDelay;
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
        void setRouter(Router *nnorth,Router *nsouth,Router *neast,Router *nwest,Router *currRouter, double Buffer_nd, double SA_nd, double Xbar_nd){  
            if(nnorth) north=new Port(nnorth,currRouter,"north");
            if(nsouth)south=new Port(nsouth,currRouter,"south");
            if(neast) east=new Port(neast,currRouter,"east");
            if(nwest) west=new Port(nwest,currRouter,"west");
            PE=new Port(nullptr,currRouter,".");
            IOdelay = Buffer_nd;
            switchAllocatorDelay = SA_nd;
            xbarDelay = Xbar_nd;
        }

        void setDelays(double Buffer_nd, double SA_nd, double Xbar_nd){
            IOdelay = Buffer_nd;
            switchAllocatorDelay = SA_nd;
            xbarDelay = Xbar_nd;
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
        if(temp->router->id==destination){   //if found return address of router
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
            return validPorts[2];}
    }
    if(validPorts[3]){
        if(getNextXYRouter(validPorts[3],destination,"east",nullptr)){
            return validPorts[3];}
    }
    return NULL;
}
Port *getNextYXRouter(Port *currentRouter,string destination,string prevRouter,Router *startRouter){     //switch allocator
    if(currentRouter && currentRouter->router->id==destination){
        // base case reached destination return destination i.e. currentRouter
        return currentRouter;
    }

    // Checking in the same column IF going east or west is even required
    vector<Port *>validPorts=getValidPorts(prevRouter,currentRouter,startRouter);
    Port *temp=validPorts[2];  //search in east
    while(temp){
        if(temp->router->id==destination){   //if found return address of router
            return validPorts[2];
        }
        temp=temp->router->east;
    }
    temp=validPorts[3];   //search in south
    while(temp){
        if(temp->router->id==destination){
            return validPorts[1];
        }
        temp=temp->router->west;
    }   

    if(validPorts[0]){   //check only north
        if(getNextXYRouter(validPorts[0],destination,"south",nullptr)){
            return validPorts[0];}
    }
    if(validPorts[1]){  //check only south
        if(getNextXYRouter(validPorts[1],destination,"north",nullptr)){
            return validPorts[1];}
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

void addToMap5(int &startcycle,string state,double delay,string flitBinary,string flitType,string start,string status,int cycleTaken){
    if(delay==cycleTime){
          logmap[startcycle].push_back({start,state,flitBinary,flitType,status,to_string(cycleTaken)});
          startcycle++;
    }
    else if(delay<cycleTime){
        if(mode==2){
            status +="delay is less than cycle so it will wait in the wait buffer and wait fo next cycle";
        }
        logmap[startcycle].push_back({start,state,flitBinary,flitType,status,to_string(cycleTaken)});
        startcycle++;
    }
    else{
        string newextra=status;
        if(mode==2){
            status +="extra cycle needed:more delay than cycle time";
        }
        logmap[startcycle].push_back({start,state,flitBinary,flitType,status+"",to_string(cycleTaken)});
        startcycle++;
        if(mode==2){
            newextra+="extra cycle";
        }
        logmap[startcycle].push_back({start,state,flitBinary,flitType,status+newextra,to_string(cycleTaken+1)});
        startcycle++;
    }
}
//first congestion check than two cycles.
vector<Port*> sendPacketXYRouting(string start,string destination,Router *Router1,int startcycle){
    vector<Port*>path;
    //------------------decoding buffer
    int overallStart=startcycle;
    string flitBinary=Router1->PE->input;    //copy data from input to direction output
    string startData=flitBinary;
    // currRouter->output=data;
    string flitType = flitBinary.substr(flitBinary.size() - 2);
    if(flitType=="00")flitType="header";
    if(flitType=="01")flitType="tail";
    if(flitType=="10")flitType="body";
    flitCount[{start,"PE"}]++;
    Port *currRouter=getNextXYRouter(nullptr,destination,"",Router1); //  find the directRouter of any 
    string extra="flit injected to local PE";
    int prevCycle=startcycle;
    congestionCheck(start,startcycle,"localbuffer");
    if((startcycle-prevCycle)!=0){
        extra+=("delayed by "+to_string(startcycle-prevCycle)+" cause : congestion");}
    addToMap5(startcycle,"localbuffer",currRouter->parentRouter->IOdelay,flitBinary,flitType,start,extra,startcycle-overallStart);
    //------------------decoding 
    congestionCheck(start,startcycle,"SA");
    addToMap5(startcycle,"SA",currRouter->parentRouter->switchAllocatorDelay,flitBinary,flitType,start," ",startcycle-overallStart);
    path.push_back(currRouter);
    // //data transfer XBAR----------------------------------
    Port *oppPort=getOppositePort(currRouter);    //copy data from input to direction output
    currRouter->output=flitBinary;
    oppPort->input=flitBinary;
    addToMap5(startcycle,"XBAR",currRouter->parentRouter->xbarDelay,flitBinary,flitType,start," ",startcycle-overallStart);
    // data transfer XBAR----------------------------------

    while(true){
        Port *nextOppPort=getOppositePort(currRouter);
        string flitdata=nextOppPort->input;
        flitType=flitdata.substr(flitdata.size() - 2);
        if(flitType=="00")flitType="header";
        if(flitType=="01")flitType="tail";
        if(flitType=="10")flitType="body";
        Port *nextRouter=getNextXYRouter(currRouter,destination,".",nullptr);
        flitCount[{currRouter->parentRouter->id,currRouter->direction}]++;
        extra="";
        prevCycle=startcycle;
        congestionCheck(currRouter->router->id,startcycle,nextOppPort->direction+"buffer");
        if((startcycle-prevCycle)!=0){
            extra+=("delayed by "+to_string(startcycle-prevCycle)+" cause : congestion");}
        addToMap5(startcycle,nextOppPort->direction+"buffer",currRouter->router->IOdelay,flitBinary,flitType,currRouter->router->id,extra,startcycle-overallStart);
        // ---------------------buffer stage---------------------------------
        
        if(currRouter->router->id==destination){
            nextRouter=currRouter->router->PE;
            logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,flitType," ",to_string(startcycle-overallStart)});
            startcycle++;
            flitCount[{currRouter->router->id,"PE"}]++;
            nextRouter->input=flitdata;
            logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,flitType,"flit removed to local PE",to_string(startcycle-overallStart)});
            // int cycletaken=startcycle-overallStart+1;
            flitStart[{start,destination,startData}].push_back(make_pair(overallStart,startcycle));
            break;
        }
        extra="";
        prevCycle=startcycle;
        congestionCheck(currRouter->router->id,startcycle,"SA");
        if((startcycle-prevCycle)!=0){
            extra+=("delayed by "+to_string(startcycle-prevCycle)+" cause : congestion");}
        path.push_back(nextRouter);
        addToMap5(startcycle,"SA",currRouter->router->switchAllocatorDelay,flitBinary,flitType,currRouter->router->id,extra,startcycle-overallStart);
        oppPort=getOppositePort(nextRouter);    //copy data from input to direction output
        currRouter->output=flitBinary;
        oppPort->input=flitBinary;
        addToMap5(startcycle,"XBAR",currRouter->router->xbarDelay,flitBinary,flitType,currRouter->router->id," ",startcycle-overallStart);
        currRouter=nextRouter;
    }
    return path;
} 
vector<Port*> sendPacketYXRouting(string start,string destination,Router *Router1,int startcycle){
    vector<Port*>path;
    //------------------decoding buffer
    int overallStart=startcycle;
    string flitBinary=Router1->PE->input;    //copy data from input to direction output
    string startData=flitBinary;
    // currRouter->output=data;
    string flitType = flitBinary.substr(flitBinary.size() - 2);
    if(flitType=="00")flitType="header";
    if(flitType=="01")flitType="tail";
    if(flitType=="10")flitType="body";
    flitCount[{start,"PE"}]++;
    Port *currRouter=getNextYXRouter(nullptr,destination,"",Router1); //  find the directRouter of any 
    string extra="flit injected to local PE";
    int prevCycle=startcycle;
    congestionCheck(start,startcycle,"localbuffer");
    if((startcycle-prevCycle)!=0){
        extra+=("delayed by "+to_string(startcycle-prevCycle)+" cause : congestion");}
    addToMap5(startcycle,"localbuffer",currRouter->parentRouter->IOdelay,flitBinary,flitType,start,extra,startcycle-overallStart);
    //------------------decoding 
    congestionCheck(start,startcycle,"SA");
    addToMap5(startcycle,"SA",currRouter->parentRouter->switchAllocatorDelay,flitBinary,flitType,start," ",startcycle-overallStart);
    path.push_back(currRouter);
    // //data transfer XBAR----------------------------------
    Port *oppPort=getOppositePort(currRouter);    //copy data from input to direction output
    currRouter->output=flitBinary;
    oppPort->input=flitBinary;
    addToMap5(startcycle,"XBAR",currRouter->parentRouter->xbarDelay,flitBinary,flitType,start," ",startcycle-overallStart);
    // data transfer XBAR----------------------------------

    while(true){
        Port *nextOppPort=getOppositePort(currRouter);
        string flitdata=nextOppPort->input;
        flitType=flitdata.substr(flitdata.size() - 2);
        if(flitType=="00")flitType="header";
        if(flitType=="01")flitType="tail";
        if(flitType=="10")flitType="body";
        Port *nextRouter=getNextYXRouter(currRouter,destination,".",nullptr);
        flitCount[{currRouter->parentRouter->id,currRouter->direction}]++;
        extra="";
        prevCycle=startcycle;
        congestionCheck(currRouter->router->id,startcycle,nextOppPort->direction+"buffer");
        if((startcycle-prevCycle)!=0){
            extra+=("delayed by "+to_string(startcycle-prevCycle)+" cause : congestion");}
        addToMap5(startcycle,nextOppPort->direction+"buffer",currRouter->router->IOdelay,flitBinary,flitType,currRouter->router->id,extra,startcycle-overallStart);
        // ---------------------buffer stage---------------------------------
        
        if(currRouter->router->id==destination){
            nextRouter=currRouter->router->PE;
            logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,flitType," ",to_string(startcycle-overallStart)});
            startcycle++;
            flitCount[{currRouter->router->id,"PE"}]++;
            nextRouter->input=flitdata;
            logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,flitType,"flit removed to local PE",to_string(startcycle-overallStart)});
            // int cycletaken=startcycle-overallStart+1;
            flitStart[{start,destination,startData}].push_back(make_pair(overallStart,startcycle));
            break;
        }
        extra="";
        prevCycle=startcycle;
        congestionCheck(currRouter->router->id,startcycle,"SA");
        if((startcycle-prevCycle)!=0){
            extra+=("delayed by "+to_string(startcycle-prevCycle)+" cause : congestion");}
        path.push_back(nextRouter);
        addToMap5(startcycle,"SA",currRouter->router->switchAllocatorDelay,flitBinary,flitType,currRouter->router->id,extra,startcycle-overallStart);
        oppPort=getOppositePort(nextRouter);    //copy data from input to direction output
        currRouter->output=flitBinary;
        oppPort->input=flitBinary;
        addToMap5(startcycle,"XBAR",currRouter->router->xbarDelay,flitBinary,flitType,currRouter->router->id," ",startcycle-overallStart);
        currRouter=nextRouter;
    }
    return path;
} 
void sendPacket(int cycle,tuple<string,string,string> values,map<string,Router *>RouterMap){
vector<Port*>path;
string source = get<0>(values);
string destination = get<1>(values);
string flitBinary = get<2>(values);
string binaryPayload = flitBinary.substr(2, flitBinary.size()); // Extract remaining bits for payload
string flitType = flitBinary.substr(flitBinary.size() - 2);
if(flitType=="00"){
    string currVal=flitBinary.substr(2,flitBinary.size()-4);
    string binarysrcNode = currVal.substr(currVal.size() - 15);
    int srcNode = stoi(binarysrcNode, 0, 2);
    string binarydestNode = currVal.substr(0, currVal.size() - 15);
    int destNode = stoi(binarydestNode, 0, 2);
    // srcDestMap[binaryPayload]={to_string(srcNode),to_string(destNode)};
}
    // flitStart[binaryPayload]=cycle;
    RouterMap[source]->PE->input=binaryPayload;
    if(routeType==1){
    path=sendPacketXYRouting(source,destination,RouterMap[source],cycle);
    }
    else{
        path=sendPacketYXRouting(source,destination,RouterMap[source],cycle);
    }
    string currPath="";
    for(auto it:path){currPath+=it->parentRouter->id + " -> ";
    currPath+=it->direction;}
    currPath+=destination;
    finalpath.push_back({currPath,source,destination,flitBinary});
}

void startSending(map<string,Router *>RouterMap){
    int currentCycle=0,maxCycle=10;
    while(currentCycle<=maxCycle){
        if(trafficData.find(currentCycle)!=trafficData.end()){
            for(auto it:trafficData){
                if(it.first==currentCycle){
                    sendPacket(it.first,it.second,RouterMap);
                }
            }
        }
        currentCycle++;
    }
}

void printLogMap(map<int, vector<vector<string>>>& logmap,int clk) {
    logFile<<left<<setw(15)<<"Cycle No"<<setw(15)<<"No of Flits "<<setw(20)<<"Router No"<<setw(20)<<"Stage"<<setw(40)<<"Flit"<<setw(15)<<"Type"<<endl;
    logFile<<string(120, '-')<<endl;
    for (auto& data : logmap) {
        for (auto& vec : data.second) {
            logFile<<left<<setw(15)<<data.first<<setw(15)<<data.second.size()<<setw(20)<<vec[0]<<setw(20)<<vec[1]<<setw(40)<<vec[2]<<setw(15)<<vec[3]<<setw(15)<<vec[5]<<setw(15)<<endl;
            // if(vec)
        }
    }
}
void printReport(map<int, vector<vector<string>>>& logmap,int clk) {
    reportFile<<left<<setw(15)<<"Cycle No"<<setw(15)<<"No of Flits "<<setw(20)<<"Router No"<<setw(20)<<"Stage"<<setw(40)<<"Flit"<<setw(15)<<"Type"<<endl;
    reportFile<<string(120, '-')<<endl;
    for (auto data : logmap) {
        for (auto vec : data.second) {
            // int num=stoi(to_string(vec[5]));
    // cout<<typeid(vec[5]).name(); 
            // cout<<vec[5].size()<<" sizze"<<endl;
            reportFile<<left<<setw(15)<<data.first<<setw(15)<<data.second.size()<<setw(20)<<vec[0]<<setw(20)<<vec[1]<<setw(40)<<vec[2]<<setw(15)<<vec[3]<<setw(15)<<vec[5]<<setw(15)<<endl;
            // if(vec)
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
void printLogMap_pvs(map<int, vector<vector<string>>>& logmap,int clk) {
   logFile_pvs<<left<<setw(15)<<"Cycle No"<<setw(15)<<"No of Flits "<<setw(20)<<"Router No"<<setw(20)<<"Stage"<<setw(40)<<"Flit"<<setw(15)<<"Type"<<endl;
    logFile_pvs<<string(120, '-')<<endl;
    for (auto& data : logmap) {
        for (auto& vec : data.second) {
            logFile_pvs<<left<<setw(15)<<data.first<<setw(15)<<data.second.size()<<setw(20)<<vec[0]<<setw(20)<<vec[1]<<setw(40)<<vec[2]<<setw(15)<<vec[3]<<setw(15)<<vec[4]<<endl;
            // if(vec)
        }
    }
}
void printReport_pvs(map<int, vector<vector<string>>>& logmap,int clk) {
   reportFile_pvs<<left<<setw(15)<<"Cycle No"<<setw(15)<<"No of Flits "<<setw(20)<<"Router No"<<setw(20)<<"Stage"<<setw(40)<<"Flit"<<setw(15)<<"Type"<<endl;
    reportFile_pvs<<string(120, '-')<<endl;
    for (auto data : logmap) {
        for (auto vec : data.second) {
            // int num=stoi(to_string(vec[5]));
    // cout<<typeid(vec[5]).name(); 
            // cout<<vec[5].size()<<" sizze"<<endl;
            reportFile_pvs<<left<<setw(15)<<data.first<<setw(15)<<data.second.size()<<setw(20)<<vec[0]<<setw(20)<<vec[1]<<setw(40)<<vec[2]<<setw(15)<<vec[3]<<setw(15)<<vec[5]<<setw(15)<<endl;
            // if(vec)
        }
    }
}
void printPaths_pvs(vector<vector<string>>& paths, std::ofstream& logFile) {
    logFile_pvs<<string(120, '-')<<endl;
    logFile_pvs << "Paths:" << std::endl;
    for (auto& path : paths) {
        logFile_pvs << "Path: " << path[0] << " source: " << path[1] << " destination: " << path[2]
                << " flit: " << path[3] << endl;
    }
}
bool nlimits(double val, double mean, double stnddev){
    return (val >= (mean - (3.0 * stnddev))) && (val <= (mean + (3.0 * stnddev)));
}


void writeMapToFile(map<int, int>& _map, string &filename) {
    ofstream outputFile(filename);
    if (outputFile.is_open()) {
        for (auto pair : _map) {
            outputFile << pair.first << " " << pair.second << "\n";
        }
        outputFile.close();
    } else {
        cout << "Unable to open file";
    }
    // return _map;
}
int main(int argc, char* argv[]){
    if (argc != 2) {
        cerr<<"Usage: "<<argv[0]<<" <routing_type>"<<endl;
        return 1;
    }
    string routingType = argv[1]; \
    if(routingType=="XY"){
        routeType=1;
    }
    else if(routingType=="YX"){
        routeType=2;
    }
    else {
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
    double clkRate=getFreq();
    cycleTime=clkRate;
    mode=1;
    string filename = "traffic.txt"; 
    trafficData=extract(filename);
    Router *router_1=new Router("1");
    Router *router_2=new Router("2");
    Router *router_3=new Router("3");
    Router *router_4=new Router("4");
    Router *router_5=new Router("5");
    Router *router_6=new Router("6");
    Router *router_7=new Router("7");
    Router *router_8=new Router("8");
    Router *router_9=new Router("9");
    vector<int> D = Delays();
    int Buffer_nd = D[0];
    int SA_nd = D[1];
    int XBAR_nd = D[2];
    router_1->setRouter(nullptr,router_4,router_2,nullptr,router_1, Buffer_nd, SA_nd, XBAR_nd);   //N S E W current
    router_2->setRouter(nullptr,router_5,router_3,router_1,router_2, Buffer_nd, SA_nd, XBAR_nd);
    router_3->setRouter(nullptr,router_6,nullptr,router_2,router_3, Buffer_nd, SA_nd, XBAR_nd);

    router_4->setRouter(router_1, router_7, router_5, nullptr, router_4, Buffer_nd, SA_nd, XBAR_nd);
    router_5->setRouter(router_2, router_8, router_6, router_4, router_5, Buffer_nd, SA_nd, XBAR_nd);
    router_6->setRouter(router_3, router_9, nullptr, router_5, router_6, Buffer_nd, SA_nd, XBAR_nd);

    router_7->setRouter(router_4, nullptr, router_8, nullptr, router_7, Buffer_nd, SA_nd, XBAR_nd);
    router_8->setRouter(router_5, nullptr, router_9, router_7, router_8, Buffer_nd, SA_nd, XBAR_nd);
    router_9->setRouter(router_6, nullptr, nullptr, router_8, router_9, Buffer_nd, SA_nd, XBAR_nd);
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
    startSending(RouterMap);
    for(int i=0;i<15;i++){
        if(logmap[i].size()==0){
            logmap[i].push_back({"free"," "," "," "," "," "});
        }
    }
    printLogMap(logmap,clkRate);
    printPaths(finalpath, logFile);
    printReport(logmap, clkRate);   
    cout<<"shared"<<endl;
    logmap.clear();
    map<int,int>totalTime;  //id , packet;
    int cnt=1;
    for(auto it:packets){
        // cout<<"z"<<endl;
        int cycle=0,start1,end1;
        vector<int>temp(100,0);
        if(flitStart.find({it[3],it[4],it[0].substr(2)})!=flitStart.end()  && flitStart[{it[3],it[4],it[0].substr(2)}].size()>0){
        start1=flitStart[{it[3],it[4],it[0].substr(2)}][0].first;
        end1=flitStart[{it[3],it[4],it[0].substr(2)}][0].second;
        for(int j=start1;j<=end1;j++)temp[j]=1;
        flitStart[{it[3],it[4],it[0].substr(2)}].erase(flitStart[{it[3],it[4],it[0].substr(2)}].begin());
        }

        if(flitStart.find({it[3],it[4],it[1].substr(2)})!=flitStart.end()  && flitStart[{it[3],it[4],it[1].substr(2)}].size()>0){
            start1=flitStart[{it[3],it[4],it[1].substr(2)}][0].first;
            end1=flitStart[{it[3],it[4],it[1].substr(2)}][0].second;
            for(int j=start1;j<=end1;j++)temp[j]=1;
            flitStart[{it[3],it[4],it[1].substr(2)}].erase(flitStart[{it[3],it[4],it[1].substr(2)}].begin());
        }
        if(flitStart.find({it[3],it[4],it[1].substr(2)})!=flitStart.end()  && flitStart[{it[3],it[4],it[1].substr(2)}].size()>0){
           int start1=flitStart[{it[3],it[4],it[2].substr(2)}][0].first;
            int end1=flitStart[{it[3],it[4],it[2].substr(2)}][0].second;
            for(int j=start1;j<=end1;j++)temp[j]=1;
            flitStart[{it[3],it[4],it[2].substr(2)}].erase(flitStart[{it[3],it[4],it[2].substr(2)}].begin());
        }
        for(int i:temp){
            if(i==1)cycle++;
        }
        totalTime[cnt]=cycle;
        cnt++;
    }
    string pvamap="pvamap.txt";
    string pvsmap="pvsmap.txt";
    writeMapToFile(totalTime,pvamap);
    for(auto it:totalTime)cout<<it.first<<" PVA "<<it.second<<endl;
    cout<<"-----------------"<<endl;
    flitStart.clear();
    int id=1;
    logFile<<"packets as header , body , tail , id"<<endl;
    for (auto it:packets) {
        idMap[it]=id;
        logFile<<it[0]<<" "<<it[1]<<" "<<it[2]<<" "<<it[3]<<" "<<it[4]<<" "<<id<<endl;
        id++;
    }
    // packets.clear();
    double mean_buffernd, mean_sand, mean_xbarnd;
    mean_buffernd = Buffer_nd;
    mean_sand = SA_nd;
    mean_xbarnd = XBAR_nd;


    // Standard Deviation of nominal delay across differnt router
    double stnddev_buffernd, stnddev_sand, stnddev_xbarnd;
    stnddev_buffernd = 0.1 * mean_buffernd;
    stnddev_sand = 0.1 * mean_sand;
    stnddev_xbarnd = 0.1 * mean_xbarnd;

    // Normal Distribution of Buffer Delay
    default_random_engine generator;
    normal_distribution<double> distribution_buffer(mean_buffernd,stnddev_buffernd);
    normal_distribution<double> distribution_sa(mean_sand,stnddev_sand);
    normal_distribution<double> distribution_xbar(mean_xbarnd,stnddev_xbarnd);

    vector<vector<double>> Dpvs;

    while(Dpvs .size() != 9){
        double Buffer_pvs = distribution_buffer(generator);
        double SA_pvs = distribution_sa(generator);
        double XBAR_pvs = distribution_xbar(generator);
        if(nlimits(Buffer_pvs, mean_buffernd, stnddev_buffernd) && nlimits(SA_pvs, mean_sand, stnddev_sand) && nlimits(XBAR_pvs, mean_xbarnd, stnddev_xbarnd)){
            Dpvs.push_back({Buffer_pvs, SA_pvs, XBAR_pvs});
        }
    }
    for(char num = '1'; num <= '9'; num++){
        auto it = Dpvs[num -'1'];
        auto d1=it[0],d2=it[1],d3=it[2];
        RouterMap[string(1, num)] -> setDelays(d1, d2, d3);
    }


    if (!logFile_pvs.is_open()) {
        cerr << "Failed to open the log file." << endl;
        return 1;
    }
    if (!reportFile_pvs.is_open()) {
        cerr << "Failed to open the log file." << endl;
        return 1;
    }

    reportFile_pvs.close();
    reportFile_pvs.open("report_pvs.txt", ios::app);
    logFile_pvs.close();
    logFile_pvs.open("log_pvs.txt", ios::app);
    mode=2;
    printGraphsPVA();
    flitCount.clear();
    flitStart.clear();
    logmap.clear(); 
    finalpath.clear();
    startSending(RouterMap);
    printLogMap_pvs(logmap,clkRate);
    printPaths_pvs(finalpath, logFile_pvs);
    printReport_pvs(logmap, clkRate);
    printGraphsPVS();
    cout<<flitStart.size()<<endl;
    map<int,int>timePVS;  //id , packet;
    cnt=1;
    // for(auto it:flitStart)
    cout<<packets.size();
    for(auto it:packets){
       
        int cycle=0,start1,end1;
        vector<int>temp(100,0);
        if(flitStart.find({it[3],it[4],it[0].substr(2)})!=flitStart.end()  && flitStart[{it[3],it[4],it[0].substr(2)}].size()>0){
          start1=flitStart[{it[3],it[4],it[0].substr(2)}][0].first;
         end1=flitStart[{it[3],it[4],it[0].substr(2)}][0].second;
        for(int j=start1;j<=end1;j++)temp[j]=1;
        flitStart[{it[3],it[4],it[0].substr(2)}].erase(flitStart[{it[3],it[4],it[0].substr(2)}].begin());

        }

        if(flitStart.find({it[3],it[4],it[1].substr(2)})!=flitStart.end()  && flitStart[{it[3],it[4],it[1].substr(2)}].size()>0){
            start1=flitStart[{it[3],it[4],it[1].substr(2)}][0].first;
            end1=flitStart[{it[3],it[4],it[1].substr(2)}][0].second;
            for(int j=start1;j<=end1;j++)temp[j]=1;
            flitStart[{it[3],it[4],it[1].substr(2)}].erase(flitStart[{it[3],it[4],it[1].substr(2)}].begin());
        }
        if(flitStart.find({it[3],it[4],it[2].substr(2)})!=flitStart.end()  && flitStart[{it[3],it[4],it[2].substr(2)}].size()>0){
           int start1=flitStart[{it[3],it[4],it[2].substr(2)}][0].first;
            int end1=flitStart[{it[3],it[4],it[2].substr(2)}][0].second;
            for(int j=start1;j<=end1;j++)temp[j]=1;
            flitStart[{it[3],it[4],it[2].substr(2)}].erase(flitStart[{it[3],it[4],it[2].substr(2)}].begin());
        }
        for(int i:temp){
            if(i==1)cycle++;
        }
        timePVS[cnt]=cycle;
        cnt++;
    }
    // cout<<" sdfhkasjdfhsd";
    // for(auto it:timePVS){
    //     cout<<it.second<<endl;
    // }
    
    writeMapToFile(timePVS,pvsmap);
    // for(int i)
    reportFile<<"COMPARISON"<<endl;
    // for(int i=1;i<totalTime.size();i++){
    //     reportFile<<"PVA time"<<
    // }
    for(auto it:timePVS){
        reportFile<<"PVS time"<<it.second<<" PVA time "<<totalTime[it.first]<<endl;
    }
    reportFile<<"PVA TAKE LESS CYCLES";
    return 0;
}


// 1  2  3
// 4  5  6
// 7  8  9