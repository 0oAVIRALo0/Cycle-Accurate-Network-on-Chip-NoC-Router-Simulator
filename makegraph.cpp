#include <algorithm>
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
#include <cstdlib>

using namespace std;

map<string,pair<string,string>>srcDestMap;
map<string,int>flitStart;
ofstream logFile("log.txt");
ofstream logFile_pvs("log_pvs.txt");
ofstream reportFile("report.txt");
ofstream reportFile_pvs("report_pvs.txt");
map<int,vector<vector<string>>>logmap;
vector<vector<string>>finalpath;
int delay_accum = 0;

map<int, tuple<string, string, string>> extract(string& filename) {
    ifstream file(filename);
    string line;
    map<int, tuple<string, string, string>> trafficData;
    while (getline(file, line)) {
        istringstream iss(line);
        int cycle;
        string source, destination, flitBinary;
        if(iss >> cycle >> source >> destination >> flitBinary){
            trafficData[cycle] = make_tuple(source, destination, flitBinary);
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
    cout << "Clock cycle time = " << maxDelay << " ms" << endl;
    cout << "Clock frequency = " << clockFreq << " Hz" << endl;
    return maxDelay;
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

int check_trafficFile_errors(){
    int errors = 0;

    std::ifstream trafficFile("traffic.txt"); // Open the traffic file
    if(!trafficFile.is_open()){
        errors++;
        logFile << "Error opening traffic file" << endl;
        logFile.close();
        return errors;
    }

    int prev_cycle = -1, next_cycle, src, dst, head_flit_count = 0, body_flit_count = 0, tail_flit_count = 0, line_number = 0;
    string flitBinary;

    // Read one line of the traffic file to assign values to the 4 variables
    // Keep on doing this till the EOF is reached
    while(trafficFile >> next_cycle >> src >> dst >> flitBinary){
        line_number++;
        if(next_cycle < prev_cycle){
            logFile << "Error in line " << line_number << " : Cycle number out of order." << endl;
            errors++;
        }
        prev_cycle = next_cycle;
        string flitType = flitBinary.substr(flitBinary.length() - 2);
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
        if(src < 1 || src > 9){
            logFile << "Error in line " << line_number << " : Source router number out of range (should be 1 to 9)" << endl;
            errors++;
        }
        if(dst < 1 || dst > 9){
            logFile << "Error in line " << line_number << " : Destination router number out of range (should be 1 to 9)" << endl;
            errors++;
        }
    }
    if(!(head_flit_count == body_flit_count && body_flit_count == tail_flit_count)){
        logFile << "Not all packets are complete in the traffic file." << endl;
        errors++;
    }
    return errors;
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

// Map for storing all the newly created routers
map<string,Router *>RouterMap;

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
        // base case reached destination return destination i.e. currentRouter
        return currentRouter;
    }

    vector<Port *>validPorts = getValidPorts(prevRouter,currentRouter,startRouter);

    // Checking in the same row IF going east or west is even required
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

    // If havent returned until now we need to move laterally 

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
Port *getNextYXRouter(Port *currentRouter,string destination,string prevRouter,Router *startRouter){     //switch allocator
    if(currentRouter && currentRouter->router->id==destination){
        // base case reached destination return destination i.e. currentRouter
        return currentRouter;
    }

    // Checking in the same column IF going east or west is even required
    vector<Port *>validPorts=getValidPorts(prevRouter,currentRouter,startRouter);
    Port *temp=validPorts[2];  //search in east
    while(temp){
        // cout<<"entered east ";
        if(temp->router->id==destination){   //if found return address of router
            // cout<<"returning east";
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
            // cout<<"returned north of ";
            // if(currentRouter && currentRouter->parentRouter)cout<<"currently in "<<currentRouter->parentRouter->id<<endl;
            return validPorts[0];}
    }
    if(validPorts[1]){  //check only south
        if(getNextXYRouter(validPorts[1],destination,"north",nullptr)){
            // if(currentRouter && currentRouter->parentRouter)cout<<"currently in "<<currentRouter->parentRouter->id<<endl;
            return validPorts[1];}
    }
    return NULL;
}
Port *getOppositePort(Port *currPort){
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
vector<Port*> sendPacketXYRouting(string start,string destination,Router *Router1,int startcycle){
    // cout<<"sendPacketXYRouting star Ated for start "<<Router1->id<<endl;
    vector<Port*>path;
    // cout<<Router1->PE->input;
    //------------------decoding buffer
    string flitBinary = Router1->PE->input;    //copy data from input to direction output
    // currRouter->output=data;
    string flitType = flitBinary.substr(flitBinary.size() - 2);
    if(flitType == "00")flitType = "head";
    if(flitType == "01")flitType = "body";
    if(flitType == "10")flitType = "tail";
    logmap[startcycle].push_back({start,"buffer",flitBinary,flitType});

    //------------------decoding 
    Port *currRouter = getNextXYRouter(nullptr, destination, "", Router1); //  find the directRouter of any 
    startcycle++;
    logmap[startcycle].push_back({start,"SA",flitBinary,flitType});
    path.push_back(currRouter);
    // //data transfer XBAR----------------------------------
    startcycle++;
    Port *oppPort = getOppositePort(currRouter);    //copy data from input to direction output
    currRouter->output = flitBinary;
    oppPort->input = flitBinary;
    logmap[startcycle].push_back({start,"XBAR",flitBinary,flitType});

    // data transfer XBAR----------------------------------

    while(true){
        Port *nextOppPort=getOppositePort(currRouter);
        string flitdata=nextOppPort->input;
        flitType=flitdata.substr(flitdata.size() - 2);
        if(flitType=="00")flitType="head";
        if(flitType=="01")flitType="body";
        if(flitType=="10")flitType="tail";
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"buffer",flitdata,flitType});
        Port *nextRouter=getNextXYRouter(currRouter,destination,".",nullptr);
        if(currRouter->router->id==destination){
            nextRouter=currRouter->router->PE;
            startcycle++;
            logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,flitType});
            nextRouter->input=flitdata;
            startcycle++;
            logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,flitType});
            break;
        }
        path.push_back(nextRouter);
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,flitType});
        oppPort=getOppositePort(nextRouter);    //copy data from input to direction output
        currRouter->output=flitBinary;
        oppPort->input=flitBinary;
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,flitType});
        currRouter=nextRouter;
    }
    return path;
}
vector<Port*> sendPacketYXRouting(string start,string destination,Router *Router1,int startcycle){
    // cout<<"sendPacketXYRouting star Ated for start "<<Router1->id<<endl;
    vector<Port*>path;
    // cout<<Router1->PE->input;
    //------------------decoding buffer
    string flitBinary = Router1->PE->input;    //copy data from input to direction output
    // currRouter->output=data;
    string flitType = flitBinary.substr(flitBinary.size() - 2);
    if(flitType == "00")flitType = "head";
    if(flitType == "01")flitType = "body";
    if(flitType == "10")flitType = "tail";
    logmap[startcycle].push_back({start,"buffer",flitBinary,flitType});

    //------------------decoding 
    Port *currRouter = getNextYXRouter(nullptr, destination, "", Router1); //  find the directRouter of any 
    startcycle++;
    logmap[startcycle].push_back({start,"SA",flitBinary,flitType});
    path.push_back(currRouter);
    // //data transfer XBAR----------------------------------
    startcycle++;
    Port *oppPort = getOppositePort(currRouter);    //copy data from input to direction output
    currRouter->output = flitBinary;
    oppPort->input = flitBinary;
    logmap[startcycle].push_back({start,"XBAR",flitBinary,flitType});

    // data transfer XBAR----------------------------------

    while(true){
        Port *nextOppPort=getOppositePort(currRouter);
        string flitdata=nextOppPort->input;
        flitType=flitdata.substr(flitdata.size() - 2);
        if(flitType=="00")flitType="head";
        if(flitType=="01")flitType="body";
        if(flitType=="10")flitType="tail";
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"buffer",flitdata,flitType});
        Port *nextRouter=getNextYXRouter(currRouter,destination,".",nullptr);
        if(currRouter->router->id==destination){
            nextRouter=currRouter->router->PE;
            startcycle++;
            logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,flitType});
            nextRouter->input=flitdata;
            startcycle++;
            logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,flitType});
            break;
        }
        path.push_back(nextRouter);
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"SA",flitBinary,flitType});
        oppPort=getOppositePort(nextRouter);    //copy data from input to direction output
        currRouter->output=flitBinary;
        oppPort->input=flitBinary;
        startcycle++;
        logmap[startcycle].push_back({(currRouter->router->id),"XBAR",flitBinary,flitType});
        currRouter=nextRouter;
    }
    return path;
} 
void sendPacket(map<string,Router *>RouterMap, int routingtype){
    string filename = "traffic.txt"; 
    map<int, tuple<string, string, string>> trafficData = extract(filename);
    vector<Port*>path;
    
    for (auto& data : trafficData) {
        int cycle = data.first;
        auto values = data.second;
        string source = get<0>(values);
        string destination = get<1>(values);
        string flitBinary = get<2>(values);


        string binaryPayload = flitBinary.substr(2, flitBinary.size()); // Extract bits for payload
        string flitType = flitBinary.substr(flitBinary.size() - 2);
        
        // Head Flit
        if(flitType == "00"){
            string currVal = flitBinary.substr(2,flitBinary.size()-4);

            string binarysrcNode = currVal.substr(currVal.size() - 15);
            
            // Src Node from head flit
            int srcNode = stoi(binarysrcNode, 0, 2);
            string binarydestNode = currVal.substr(0, currVal.size() - 15);
            // Des Node from head flit
            int destNode = stoi(binarydestNode, 0, 2);

            //cout<<srcNode<<" "<<destNode<<endl;
            srcDestMap[binaryPayload] = {to_string(srcNode),to_string(destNode)};
        }
            flitStart[binaryPayload] = cycle; // Added Payload to map with cycle info

            RouterMap[source]-> PE -> input = binaryPayload;
            // Sends the packet
            if(routingtype == 0){
                path = sendPacketXYRouting(source, destination, RouterMap[source], cycle);
            } else{
                path = sendPacketYXRouting(source, destination, RouterMap[source], cycle);
            }
            
            string currPath = "";
            
            for(auto it:path){
                currPath+=it->parentRouter->id + " -> ";
                currPath+=it->direction + " ";
            }
            currPath += destination;
            // Write the path to finalpath
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
    reportFile << left << "Router No" << setw(25) << "     Stage" << setw(45) << "Flit" << setw(15) << "Delay" << endl;
    reportFile << string(95, '-') << endl;

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
void printLogMap_pvs(map<int, vector<vector<string>>>& logmap,int clk) {
    logFile_pvs<<left<<setw(15)<<"Cycle No"<<setw(15)<<"No of Flits "<<setw(20)<<"Router No"<<setw(20)<<"Stage"<<setw(40)<<"Flit"<<setw(15)<<"Type"<<endl;
    logFile_pvs<<string(120, '-')<<endl;

    for (auto& data : logmap) {
        for (auto& vec : data.second) {
            logFile_pvs<<left<<setw(15)<<data.first<<setw(15)<<data.second.size()<<setw(20)<<vec[0]<<setw(20)<<vec[1]<<setw(40)<<vec[2]<<setw(15)<<vec[3]<<endl;
        }
    }
}

map<string, bool> updatepen;
void printReport_pvs(map<int, vector<vector<string>>>& logmap,int clk) {
    reportFile_pvs << left << "Router No" << setw(25) << "     Stage" << setw(45) << "Flit" << setw(15) << "Delay" << endl;
    reportFile_pvs << string(95, '-') << endl;

    for (auto& data : logmap) {
        for (auto& vec : data.second) {
            Router *router = RouterMap[vec[0]];
            double dbuf = router -> IOdelay, dsa = router -> switchAllocatorDelay, dxbar = router -> xbarDelay;
            vector<pair<double, string>> arrangeDealays;
            arrangeDealays.push_back({dbuf, "buffer"});
            arrangeDealays.push_back({dsa, "SA"});
            arrangeDealays.push_back({dxbar, "XBAR"});
            sort(arrangeDealays.begin(), arrangeDealays.end());
            double dmax = arrangeDealays[2].first;
            string stage = arrangeDealays[2].second;

            // Delay from back?
            if(updatepen[vec[2]]){
                reportFile_pvs<<left<<"Router "<< vec[0] << "'s " <<vec[1]<< " didn't receive flit on time. Flit "<<vec[2] << " is delayed by "<< delay_accum<< " clock cycle."<<endl;
                updatepen[vec[2]] = false;
            }

            // If delay of the router is less than that of a ideal PVA mode router
            // No change router will forward the flit on the upcoming clock tick
            if(dmax <= clk){
                reportFile_pvs<<left<<setw(15)<<vec[0]<<setw(15)<<vec[1]<<setw(35)<<vec[2]<<clk*(data.first-(flitStart[vec[2]]) + delay_accum)<<endl;
            } else{
            // If delay of the router is greater than that of a ideal PVA mode router
            // It cannot complete all its functioning in alloted clk delay so it will require addidtional clock cycle
                if(stage != vec[1]){
                    reportFile_pvs<<left<<setw(15)<<vec[0]<<setw(15)<<vec[1]<<setw(35)<<vec[2]<<clk*(data.first-(flitStart[vec[2]]) + delay_accum)<<endl;
                    continue;
                }

                reportFile_pvs<<left<<"Delays of Router " << vec[0] << " are(Buffer, SA, XBAR): "<<dbuf << " " << dsa << " " << dxbar <<endl;
                reportFile_pvs<<left<<stage<< " of Router "<< vec[0] << " took longer than expected. Flit "<<vec[2] << " is delayed by additional 1 clock cycle."<<endl; 
                reportFile_pvs<<left<<setw(15)<<vec[0]<<setw(15)<<vec[1]<<setw(35)<<vec[2]<<clk*(data.first-(flitStart[vec[2]]) + delay_accum)<<endl;
                delay_accum++;
                updatepen[vec[2]] = true;
            }
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

void printGraph1(map<pair<string,string>,int>flitCount){
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
    ofstream dataFile("data.txt", ios::app);
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

void printGraphs(map<pair<string,string>,int>flitCount){
    ofstream dataFile("data.txt");
    dataFile.close();
    printGraph1(flitCount);
    // printGraph2();
    system("python graph.py");
    return;
}

bool nlimits(double val, double mean, double stnddev){
    return (val >= (mean - (3.0 * stnddev))) && (val <= (mean + (3.0 * stnddev)));
}


int main(int argc, char* argv[]){
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <routing_type>" << endl;
        return 1;
    }
    
    if (!reportFile.is_open()) {
        cerr << "Failed to open the report file." << endl;
        return 1;
    }
    if (!logFile.is_open()) {
        cerr << "Failed to open the log file." << endl;
        return 1;
    }
    
    string routingType = argv[1]; 

    int routingtype;
    routingtype = (routingType == "XY") ? 0 : (routingType == "YX") ? 1 : -1;

    if (routingtype == -1) {
        cerr << "Invalid routing type: " << routingType << std::endl;
        return 1;
    }

    reportFile.close();
    logFile.close();
    reportFile.open("report.txt", ios::app);
    logFile.open("log.txt", ios::app);

    int errorCount = check_trafficFile_errors();
    if(errorCount > 0){
        cout << errorCount << " error(s) found in the traffic file. Check log file for more information" << endl;
        logFile.close();
        ifstream logFile("log.txt");
        logFile_pvs << logFile.rdbuf();
        logFile.close();
        return 1;
    }

    // Getting Clock Frequency
    int clkRate=getFreq();
    Router *router_1 = new Router("1");
    Router *router_2 = new Router("2");
    Router *router_3 = new Router("3");
    Router *router_4 = new Router("4");
    Router *router_5 = new Router("5");
    Router *router_6 = new Router("6");
    Router *router_7 = new Router("7");
    Router *router_8 = new Router("8");
    Router *router_9 = new Router("9");

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

    RouterMap["1"] = router_1;
    RouterMap["2"] = router_2;
    RouterMap["3"] = router_3;
    RouterMap["4"] = router_4;
    RouterMap["5"] = router_5;
    RouterMap["6"] = router_6;
    RouterMap["7"] = router_7;
    RouterMap["8"] = router_8;
    RouterMap["9"] = router_9;

    // PVA Mode
    sendPacket(RouterMap, routingtype);
    printLogMap(logmap,clkRate);
    printPaths(finalpath, logFile);
    printReport(logmap, clkRate);

    // PVS Mode

    // Taking nominal Delay values from Router 1
    // Mean of nominal delay across differnt router
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
            // cout << Buffer_pvs << " " << SA_pvs << " " << XBAR_pvs << endl;
            Dpvs.push_back({Buffer_pvs, SA_pvs, XBAR_pvs});
        }
    }
    // cout << stnddev_buffernd << " " << stnddev_sand << " " << stnddev_xbarnd << endl;
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

    logmap.clear(); finalpath.clear();
    sendPacket(RouterMap, routingtype);
    printLogMap_pvs(logmap,clkRate);
    printPaths_pvs(finalpath, logFile_pvs);
    printReport_pvs(logmap, clkRate);

    map<pair<string, string>, int> flitCount;
    flitCount[make_pair("1","east")]++;
    flitCount[make_pair("2","west")]++;
    printGraphs(flitCount);
    return 0;
}