#include<bits/stdc++.h>
using namespace std;

int NUM_FILES=0;
int MAX_CLOCK_CYCLES=0;

vector<vector<int>> register_File(50,vector<int>(32,0));
bitset<8388608> memory;
//int INSTR_MEM = 0;
vector<int> INSTR_MEM(50,0);
vector<int> start_mem(50,0);
vector<int> end_mem(50,0);
vector<int> CLOCK_CYCLES(50,0);
int currentCycle=0;
vector<int> error(50,0);


struct DATA_BUS {
    int ins; // 0 for lw and 1 for sw, assumme its replicates to 1byte actually
    int value;
};
struct DRAM_DATA {
    bitset<1024*8> rowBuffer;
    DATA_BUS data;
    int colOffset;
    bool exec;
    bool write;
    bool read;
    int mem;
    int reg;
    int memValue;
    int regValue;
    int CLOCK_CYCLES_INSTRUCTED;
    int ACTIVATED_ROW_NUMBER;
    int ROW_BUFFER_UPDATE_COUNT;
    int ROW_ACCESS_DELAY;
    int COL_ACCESS_DELAY;
    int START_CYCLE;
    int filenum;

    DRAM_DATA(){
        exec=false;
        write=false;
        read=false;
        ACTIVATED_ROW_NUMBER = -1;
        START_CYCLE=0;
        CLOCK_CYCLES_INSTRUCTED=0;
        ROW_BUFFER_UPDATE_COUNT = 0;
        ROW_ACCESS_DELAY = 10;
        COL_ACCESS_DELAY = 2;
    }

};
DRAM_DATA DRAM;

vector<string> rev_register_map = {"$zero","$at","$v0","$v1","$a0","$a1","$a2","$a3","$t0","$t1","$t2","$t3","$t4"
                                    ,"$t5","$t6","$t7","$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7","$t8","$t9"
                                    ,"$k0","$k1","$gp","$sp","$fp","$ra"};
vector<string> ins = {"add","sub","mul","beq","bne","slt","j","lw","sw","addi"};
unordered_map<string,int> label_map;
unordered_set<string> labels_used;
unordered_map<int,int> changedMemLocation_map;
unordered_map<string,int> ins_map;
void initializeInsMap(){
    ins_map["add"] = 0;
    ins_map["sub"] = 1;
    ins_map["mul"] = 2;
    ins_map["beq"] = 3;
    ins_map["bne"] = 4;
    ins_map["slt"] = 5;
    ins_map["j"] = 6;
    ins_map["lw"] = 7;
    ins_map["sw"] = 8;
    ins_map["addi"] = 9;
}
unordered_map<string,int> register_map;
void initializeRegisterMap(){
    register_map["$zero"]=0;
    register_map["$at"]=1;
    register_map["$v0"]=2;
    register_map["$v1"]=3;
    register_map["$a0"]=4;
    register_map["$a1"]=5;
    register_map["$a2"]=6;
    register_map["$a3"]=7;
    register_map["$t0"]=8;
    register_map["$t1"]=9;
    register_map["$t2"]=10;
    register_map["$t3"]=11;
    register_map["$t4"]=12;
    register_map["$t5"]=13;
    register_map["$t6"]=14;
    register_map["$t7"]=15;
    register_map["$s0"]=16;
    register_map["$s1"]=17;
    register_map["$s2"]=18;
    register_map["$s3"]=19;
    register_map["$s4"]=20;
    register_map["$s5"]=21;
    register_map["$s6"]=22;
    register_map["$s7"]=23;
    register_map["$t8"]=24;
    register_map["$t9"]=25;
    register_map["$k0"]=26;
    register_map["$k1"]=27;
    register_map["$gp"]=28;
    register_map["$sp"]=29;
    register_map["$fp"]=30;
    register_map["$ra"]=31;
}

vector<bool> changeReg(50,false); // for printing changed reg values after every instruction
vector<int> changedRegister(50);
vector<int> changedRegValue(50);

int INSTR_NUM = 0;

string intToBits5(int v){
    return bitset<5>(v).to_string();
}
string intToBits4(int v){
    return bitset<4>(v).to_string();
}
string intToBits18(int v){
    return bitset<18>(v).to_string();
}
string intToBits28(int v){
    return bitset<28>(v).to_string();
}
string intToBits23(int v){
    return bitset<23>(v).to_string();
}
string intToBits32(int v){
    return bitset<32>(v).to_string();
}

int reg(string s){
    return register_map[s];
}

int register_file(int p,int filenum){
    return register_File[filenum][p];
}

int goto_lab(string s){
    return label_map[s];
}

void copyToMem(string s,int m){
    int i=0;
    for(char ch:s){
        if(ch=='0'){
            memory[m+i]=0;
        }
        else{
            memory[m+i]=1;
        }
        i++;
    }
}

void putRegInMem(string r,int m){
    int re = reg(r);
    string res = intToBits5(re);
    copyToMem(res,m);
}

int getFromMem(int m,int sz){
    int num=0;
    int flip=1;
    if(sz>=18 && memory[m]==1){
        flip=0;
    }
    for(int i=m;i<m+sz;i++){
        num*=2;
        if(memory[i]==flip){
            num+=1;
        }
    }
    if(sz>=18 && memory[m]==1){
        num = (num+1)*-1;
    }
    return num;
}

struct Instruction {
    string command;
    vector<string> param;
    int lineNumber;
    int execution_count;
    string str;
    int mem_address;
};

vector<Instruction> program;
vector<string> temp_paramList;
string temp_command;

bool isEmpty(string s){
    for(char ch:s){
        if(ch!=' '&&ch!='\t'){
            return false;
        }
    }
    return true;
}

bool validReg(string s){
    if(s=="$at" || register_map.find(s)==register_map.end()){
        return false;
    }
    return true;
}

bool validLabel(string s){
    if(s==""){
        return false;
    }
    if(! (s[0]=='_' || (s[0]>='a'&&s[0]<='z') || (s[0]>='A'&&s[0]<='Z') )){
        return false;
    }
    for(char ch:s){
        if(! (ch=='_' || (ch>='a'&&ch<='z') || (ch>='A'&&ch<='Z') || (ch>='0'&&ch<='9') )){
            return false;
        }
    }

    return true;
}

bool isNum(string s){
    if(s==""){
        return false;
    }

    int n=s.size();
    if(n==1 && s=="-"){
        return false;
    }
    for(int i=0;i<n;i++){
        if(i==0){
            if(s[i]=='-'){
                continue;
            }
        }
        if(! (s[i]<='9'&&s[i]>='0') ){
            return false;
        }
    }


    return true;
}

bool AddSubMulSlt_check(vector<string>& tokens){
    int n=tokens.size();
    if(n!=4){
        return false;
    }

    if(!validReg(tokens[1])){
        return false;
    }
    if(!validReg(tokens[2])){
        return false;
    }
    if(!validReg(tokens[3])){
        return false;
    }
    temp_paramList = {tokens[1],tokens[2],tokens[3]};
    temp_command = tokens[0];
    return true;
}

bool Branch_check(vector<string>& tokens){
    int n = tokens.size();
    if(n!=4){
        return false;
    }

    if(!validReg(tokens[1])){
        return false;
    }
    if(!validReg(tokens[2])){
        return false;
    }

    if(!validLabel(tokens[3])){
        return false;
    }
    labels_used.insert(tokens[3]);
    temp_paramList = {tokens[1],tokens[2],tokens[3]};
    temp_command = tokens[0];

    return true;
}

bool J_check(vector<string>& tokens){
    int n=tokens.size();
    if(n!=2){
        return false;
    }

    if(!validLabel(tokens[1])){
        return false;
    }
    labels_used.insert(tokens[1]);
    temp_paramList = {tokens[1]};
    temp_command = tokens[0];

    return true;
}

bool LwSw_check(vector<string> tokens){
    int n=tokens.size();
    if(n!=3){
        return false;
    }
    if(!validReg(tokens[1])){
        return false;
    }

    if(!isNum(tokens[2])){
        int sz=tokens[2].size();
        if(sz<5){
            return false;
        }

        if(sz>=7){
            string reg_part = tokens[2].substr(sz-7+1,7-2);
            if(reg_part=="$zero"){
                if(sz==7){
                    temp_paramList = {tokens[1],reg_part,"0"};
                    temp_command = tokens[0];
                    return true;
                }
                string num_part = tokens[2].substr(0,sz-7);
                if(!isNum(num_part)){
                    return false;
                }
                int number = stoi(num_part);
                if(number>131071 || number<-131072){
                    return false;
                }
                temp_paramList = {tokens[1],reg_part,num_part};
                temp_command = tokens[0];
                return true;
            }
        }

        string reg_part = tokens[2].substr(sz-5+1,5-2);
        if(!validReg(reg_part)){
            return false;
        }

        if(sz==5){
            temp_paramList = {tokens[1],reg_part,"0"};
            temp_command = tokens[0];
            return true;
        }

        string num_part = tokens[2].substr(0,sz-5);
        if(!isNum(num_part)){
            return false;
        }
        int number = stoi(num_part);
        if(number>131071 || number<-131072){
            return false;
        }

        temp_paramList = {tokens[1],reg_part,num_part};
        temp_command = tokens[0];
        return true;
    }

    temp_paramList = {tokens[1],"$zero",tokens[2]};
    temp_command = tokens[0];

    return true;
}

bool Addi_check(vector<string>& tokens){
    int n=tokens.size();
    if(n!=4){
        return false;
    }

    if(!validReg(tokens[1])){
        return false;
    }
    if(!validReg(tokens[2])){
        return false;
    }

    if(!isNum(tokens[3])){
        return false;
    }
    int sz=tokens[3].size();
    if(sz>8){
        return false;
    }
    int number = stoi(tokens[3]);
    if(number>131071 || number<-131072){
        return false;
    }

    temp_paramList = {tokens[1],tokens[2],tokens[3]};
    temp_command = tokens[0];

    return true;
}

bool Label_check(vector<string>& tokens,int filenum){
    int n=tokens.size();
    if(n>2){
        return false;
    }
    string lab;
    if(n==2){
        if(! (tokens[1]==":") ){
            return false;
        }
        else{
            if(!validLabel(tokens[0])){
                return false;
            }
        }
        lab = tokens[0];
    }
    else if(n==1){
        int sz=tokens[0].size();
        if(tokens[0][sz-1]!=':'){
            return false;
        }
        if(!validLabel(tokens[0].substr(0,sz-1))){
            return false;
        }
        lab = tokens[0].substr(0,sz-1);
    }

    // same label occurring twice in the program.
    if(label_map.find(lab) != label_map.end()){
        return false;
    }
    label_map[lab] = INSTR_MEM[filenum];
    temp_paramList = {};
    temp_command = lab;
    return true;
}

bool validate(string& s,int filenum){
    int n=s.size();
    char str[n+1];
    strcpy(str,s.c_str());
    char* temp = strtok(str," \t,");
    vector<string> tokens;
    while(temp != NULL){
        tokens.push_back(temp);
        temp = strtok(NULL," \t,");
    }
    n = tokens.size();
    if(n==0){
        return false;
    }

    if(tokens[0] == "add"){
        return AddSubMulSlt_check(tokens);
    }
    else if(tokens[0] == "sub"){
        return AddSubMulSlt_check(tokens);
    }
    else if(tokens[0] == "mul"){
        return AddSubMulSlt_check(tokens);
    }
    else if(tokens[0] == "beq"){
        return Branch_check(tokens);
    }
    else if(tokens[0] == "bne"){
        return Branch_check(tokens);
    }
    else if(tokens[0] == "slt"){
        return AddSubMulSlt_check(tokens);
    }
    else if(tokens[0] == "j"){
        return J_check(tokens);
    }
    else if(tokens[0] == "lw"){
        return LwSw_check(tokens);
    }
    else if(tokens[0] == "sw"){
        return LwSw_check(tokens);
    }
    else if(tokens[0] == "addi"){
        return Addi_check(tokens);
    }
    else{
        return Label_check(tokens,filenum);
    }
    return false;
}

void tokenise(int lineNum,string str,int filenum){
    INSTR_NUM++;
    Instruction temp;
    temp.command = temp_command;
    temp.param = temp_paramList;
    temp.lineNumber = lineNum;
    temp.execution_count = 0;
    temp.str = str;
    temp.mem_address = INSTR_MEM[filenum];
    program.push_back(temp);

    if(temp_paramList.size()!=0){
        INSTR_MEM[filenum]+=32;
    }
}

bool outOfBounds(int address,int filenum){
    if(address >= end_mem[filenum]){
        return true;
    }
    if(address < INSTR_MEM[filenum]){
        return true;
    }
    if(address%32 !=0 ){
        return true;
    }
    return false;
}

int add_overflow(int v3,int v1,int v2){
    if(v1>0 && v2>0 && v3<0){
        return true;
    }
    else if(v1<0 && v2<0 && v3>0){
        return true;
    }
    return false;
}

int sub_overflow(int v3,int v1,int v2){
    if(v1>0 && v2<0 && v3<0){
        return true;
    }
    else if(v1<0 && v2>0 && v3>0){
        return true;
    }
    return false;
}

int mul_overflow(int v3,int v1,int v2){
    if(v1==0 || v2==0){
        return false;
    }
    if(v2 == v3/v1){
        return false;
    }
    return true;
}


struct DRAMqueue {
    int ins; // lw=>0, sw=>1;
    int reg;
    int regValue;
    int mem;
    int memValue;
    int filenum;
};

int maxInsInQueue = 32;
int currentInsInQueue = 0;
vector<deque<DRAMqueue>> rowGroup(1024);
vector<vector<int>> regRowGroup(50,vector<int>(32,-1));
queue<int> DRAMreq;
vector<int> row(1024,0);
int MRMCYCLE = 0;

void printMRMOrderingDelay(int num,int filenum=-1){
    int start=max(MRMCYCLE,DRAM.START_CYCLE);
    start=max(currentCycle+1,start);
    if(filenum!=-1){
        start=max(start,CLOCK_CYCLES[filenum]);
    }
    cout<<"MRM Ordering Delay Cycles: "<<start<<"-"<<start+num<<"\n";
    MRMCYCLE = start+num;
}

void printMRMDelay(int num,string note,int filenum){
    int start = max(DRAM.CLOCK_CYCLES_INSTRUCTED,currentCycle);
    start=max(start,MRMCYCLE);
    if(filenum!=-1){
        start=max(start,CLOCK_CYCLES[filenum]);
    }
    cout<<"MRM Transfer Delay Cycles: "<<start<<"-"<<start+num<<" ( "<<note<<" )\n";
    DRAM.CLOCK_CYCLES_INSTRUCTED=start+num;
    MRMCYCLE=start+num;
    if(filenum!=-1){
        CLOCK_CYCLES[filenum]=DRAM.CLOCK_CYCLES_INSTRUCTED;
    }
}

void printStatus(string msg,int filenum){
    cout<<"Clock Cycles Completed: "<<CLOCK_CYCLES[filenum]<<"(File: "<<filenum+1<<" )\n";
    cout<<"Note: "<<msg<<"\n";
    if(changeReg[filenum]){
        cout<<"Changed Register: "<<rev_register_map[changedRegister[filenum]]<<": "<<changedRegValue[filenum]<<"\n";
        changeReg[filenum] = false;
    }
    cout<<"\n";
}

void printDRAMexecution(){
    if(DRAM.data.ins==0){
        int num=0;
        int flip=1;
        if(DRAM.rowBuffer[DRAM.colOffset*8]==1){
            flip=0;
        }
        for(int i=DRAM.colOffset*8;i<DRAM.colOffset*8+32;i++){
            num*=2;
            if(DRAM.rowBuffer[i]==flip){
                num+=1;
            }
        }
        if(DRAM.rowBuffer[DRAM.colOffset*8]==1){
            num = (num+1)*-1;
        }
        register_File[DRAM.filenum][DRAM.data.value] = num;
        DRAM.regValue = num;
    }
    else{
        int i=0;
        string value= intToBits32(DRAM.data.value);
        for(char ch:value){
            if(ch=='0'){
                DRAM.rowBuffer[DRAM.colOffset*8+i]=0;
            }
            else{
                DRAM.rowBuffer[DRAM.colOffset*8+i]=1;
            }
            i++;
        }
        DRAM.ROW_BUFFER_UPDATE_COUNT++;
    }

    CLOCK_CYCLES[DRAM.filenum] = DRAM.CLOCK_CYCLES_INSTRUCTED;
    cout<<"Clock Cycles: "<<DRAM.START_CYCLE<<"-"<<DRAM.CLOCK_CYCLES_INSTRUCTED<<"(File: "<<DRAM.filenum+1<<" )\n";
    cout<<"Note: Completion of process on DRAM\n";
    if(DRAM.read){
        cout<<"Changed Register: "<<rev_register_map[DRAM.reg]<<": "<<DRAM.regValue<<"\n";
        cout<<"Used memory location: "<<DRAM.mem-start_mem[DRAM.filenum]/8<<"\n";
        regRowGroup[DRAM.filenum][DRAM.reg] = -1;
        DRAM.read = false;
    }
    if(DRAM.write){
        cout<<"Updated Memory Location: "<<DRAM.mem-start_mem[DRAM.filenum]/8<<"-"<<DRAM.mem-start_mem[DRAM.filenum]/8+3<<" = "<<DRAM.memValue<<"\n";
        cout<<"Register from which value was put in data bus was: "<<rev_register_map[DRAM.reg]<<"\n";
        DRAM.write = false;
    }
    cout<<"\n";
    DRAM.exec=false;
}

void DRAMwriteback(){
    for(int i=0;i<1024*8;i++){
        memory[i+DRAM.ACTIVATED_ROW_NUMBER*8*1024] = DRAM.rowBuffer[i];
    }
    DRAM.CLOCK_CYCLES_INSTRUCTED += DRAM.ROW_ACCESS_DELAY;
}

void DRAMloadRow(){
    for(int i=0;i<1024*8;i++){
        DRAM.rowBuffer[i] = memory[i+DRAM.ACTIVATED_ROW_NUMBER*8*1024];
    }
    DRAM.CLOCK_CYCLES_INSTRUCTED += DRAM.ROW_ACCESS_DELAY;
    DRAM.ROW_BUFFER_UPDATE_COUNT++;
}

void transferFromQueueToDRAM(int grp,int filenum=-1){
    printMRMDelay(1,"Queue to Dram",filenum);
    DRAMqueue val = rowGroup[grp].front();
    rowGroup[grp].pop_front();
    currentInsInQueue--;
    DRAM.exec = true;
    DRAM.data.ins = val.ins;
    if(val.ins==0){
        DRAM.data.value = val.reg;
        DRAM.read = true;
    }
    else{
        DRAM.data.value = val.regValue;
        DRAM.write=true;
    }
    DRAM.mem = val.mem;
    DRAM.memValue = val.memValue;
    DRAM.reg = val.reg;
    DRAM.regValue = val.regValue;
    DRAM.filenum = val.filenum;
    DRAM.START_CYCLE = max(DRAM.CLOCK_CYCLES_INSTRUCTED,currentCycle);
    DRAM.CLOCK_CYCLES_INSTRUCTED=DRAM.START_CYCLE;
    if(DRAM.ACTIVATED_ROW_NUMBER!=-1 && DRAM.ACTIVATED_ROW_NUMBER!=grp){
        DRAMwriteback();
        DRAM.ACTIVATED_ROW_NUMBER = grp;
        DRAMloadRow();
    }
    else if(DRAM.ACTIVATED_ROW_NUMBER!=grp){
        DRAM.ACTIVATED_ROW_NUMBER = grp;
        DRAMloadRow();
    }
    DRAM.colOffset = val.mem - grp*1024;
    DRAM.CLOCK_CYCLES_INSTRUCTED += DRAM.COL_ACCESS_DELAY;
}

void nextDRAM(int filenum=-1){
    int grp = DRAM.ACTIVATED_ROW_NUMBER;
    if(grp!=-1 && !rowGroup[grp].empty()){
        transferFromQueueToDRAM(grp,filenum);
        row[grp]++;
    }
    else{
        //printMRMDelay(4,filenum);
        int pop_counts=0;
        while(!DRAMreq.empty()){
            int num = DRAMreq.front();
            DRAMreq.pop();
            pop_counts++;
            if(row[num]>0){
                row[num]--;
            }
            else{
                printMRMDelay(pop_counts,"Next row group Decision",filenum);
                transferFromQueueToDRAM(num,filenum);
                break;
            }
        }
    }
}

void executeDRAM(vector<int> regs,int filenum) {
    bool block = false;
    vector<pair<int,int>> grp_reg;
    for(int x:regs){
        if(regRowGroup[filenum][x]!=-1){
            block=true;
            if(DRAM.ACTIVATED_ROW_NUMBER == regRowGroup[filenum][x]){
                grp_reg.push_back({0,x});
            }
            else{
                grp_reg.push_back({1,x});
            }
        }
    }
    if(!block){
        if(DRAM.exec && DRAM.CLOCK_CYCLES_INSTRUCTED==CLOCK_CYCLES[filenum]+1){
            printDRAMexecution();
            nextDRAM();
        }
        return ;
    }

    sort(grp_reg.begin(),grp_reg.end());
    if(DRAM.exec){
        printDRAMexecution();
    }
    for(auto x:grp_reg){
        int r=x.second;
        while(regRowGroup[filenum][r]!=-1){
            int grp = regRowGroup[filenum][r];
            transferFromQueueToDRAM(grp,filenum);
            row[grp]++;
            printDRAMexecution();
        }
    }
    nextDRAM(filenum);
}

map<int,int> storeToLoadTransfers;

void putLw_SwInQueue(int r,int val,int inst,int filenum){
    string instruction = (inst==0?"lw":"sw");
    CLOCK_CYCLES[filenum]=max(CLOCK_CYCLES[filenum],MRMCYCLE);
    CLOCK_CYCLES[filenum]++;
    printStatus("DRAM request issued for "
    +instruction+" instruction(Reg: "+rev_register_map[r]+", Mem: "+to_string((val-start_mem[filenum])/8)+")",filenum);

    string msg="";
    //printMRMOrderingDelay(2,filenum);
    if(inst == 0){
        int grp = (val/8)/1024;
        while(!rowGroup[grp].empty()){
            DRAMqueue temp = rowGroup[grp].back();
            if(temp.ins==1 && temp.mem==val/8 && regRowGroup[filenum][r]==-1){
                printMRMOrderingDelay(1,filenum);
                // store to load transfer
                storeToLoadTransfers[filenum] = max(MRMCYCLE,max(DRAM.START_CYCLE,currentCycle+1))+1;
                register_File[filenum][r] = temp.regValue;
                printStatus(instruction+
                " instruction using store to load transfer executed\n"
                +"(Reg: "+rev_register_map[r]+", Mem: "+to_string((val-start_mem[filenum])/8)+")",filenum);
                return ;
            }
            else if(temp.ins==0 && temp.reg==r && temp.filenum==filenum){
                printMRMOrderingDelay(1,filenum);
                regRowGroup[filenum][temp.reg]=-1;
                msg+= "load at same register, instruction at the end of DRAM waiting queue popped\n";
                rowGroup[grp].pop_back();
                currentInsInQueue--;
                row[grp]++;
                cout<<msg;
            }
            else{
                break;
            }
        }

        if(rowGroup[grp].empty()){
            // duplicate lw instruction executing in DRAM.
            if(DRAM.exec && DRAM.data.ins==0 && DRAM.reg==r && DRAM.filenum==filenum && DRAM.mem==val/8){
                printMRMOrderingDelay(1,filenum);
                printStatus(instruction+
                " instruction executing in DRAM already(with no other waiting process available to overwrite)\n"
                +"(Reg: "+rev_register_map[r]+", Mem: "+to_string((val-start_mem[filenum])/8)+")",filenum);
                return ;
            }
        }
    }
    else{
        int grp = (val/8)/1024;
        while(!rowGroup[grp].empty()){
            DRAMqueue temp = rowGroup[grp].back();
            if(temp.ins==1 && temp.mem==val/8){
                printMRMOrderingDelay(1,filenum);
                regRowGroup[filenum][temp.reg]=-1;
                msg+= "store at same memory location, instruction at the end of DRAM waiting queue popped\n";
                rowGroup[grp].pop_back();
                currentInsInQueue--;
                row[grp]++;
                cout<<msg;
            }
            else if(temp.ins==0 && temp.mem==val/8 && temp.reg==r&&temp.filenum==filenum){
                printMRMOrderingDelay(1,filenum);
                //load than store(same mem, same reg)
                printStatus(instruction+
                " instruction is not executed as it first loaded from memory than stored back to memory which changes nothing"
                +"(Reg: "+rev_register_map[r]+", Mem: "+to_string((val-start_mem[filenum])/8)+")",filenum);
                return;
            }
            else{
                break;
            }
        }
    }
    executeDRAM({r},filenum);
    //printMRMOrderingDelay(1,filenum);
    if(inst == 0){
        int grp = (val/8)/1024;
        if(!rowGroup[grp].empty()){
            DRAMqueue temp = rowGroup[grp].back();
            if(temp.ins==1 && temp.mem==val/8){
                printMRMOrderingDelay(1,filenum);
                // store to load transfer
                storeToLoadTransfers[filenum] = max(MRMCYCLE,max(DRAM.START_CYCLE,currentCycle+1))+1;
                printStatus(instruction+
                " instruction using store to load transfer executed\n"
                +"(Reg: "+rev_register_map[r]+", Mem: "+to_string((val-start_mem[filenum])/8)+")",filenum);
                register_File[filenum][r] = temp.regValue;
                return ;
            }
        }
    }
    if(currentInsInQueue == maxInsInQueue){
        printMRMOrderingDelay(1,filenum);
        cout<<"Queue Full Hence making space\n";
        if(DRAM.exec){
            printDRAMexecution();
        }
        nextDRAM();
    }
    DRAMqueue temp;
    temp.ins = inst;
    temp.reg = r;
    temp.filenum=filenum;
    temp.regValue = register_file(r,filenum);
    temp.mem = val/8;
    temp.memValue = temp.regValue;
    rowGroup[temp.mem/1024].push_back(temp);
    currentInsInQueue++;
    DRAMreq.push(temp.mem/1024);
    if(inst == 0){
        regRowGroup[filenum][r] = temp.mem/1024;
    }

    cout<<"\n";
    //printStatus(msg+"DRAM request issued for "
    //+instruction+" instruction(Reg: "+rev_register_map[r]+", Mem: "+to_string((val-start_mem[filenum])/8)+")",filenum);
}



void initGlobal(){
    label_map.clear();
    labels_used.clear();
    program.clear();
}


int main(int argc, char* argv[]){
    if(argc<5){
        cout<<"Command line arguments are not correct!!! \n";
        return -1;
    }
    NUM_FILES = stoi(argv[1]);
    MAX_CLOCK_CYCLES = stoi(argv[2]);
    DRAM.ROW_ACCESS_DELAY = stoi(argv[4]);
    DRAM.COL_ACCESS_DELAY = stoi(argv[5]);
    string folder= argv[3];

    if(NUM_FILES>50){
        cout<<"Too many Files, can't handle the load!!\n";
        return -1;
    }
    if(NUM_FILES == 0){
        cout<<"NO file to be executed!!\n";
        return -1;
    }

    initializeRegisterMap();
    initializeInsMap();


    for(int filenum=0;filenum<NUM_FILES;filenum++){
        initGlobal();
        string filename = "./"+folder+"/t"+to_string(filenum+1)+".txt";
        ifstream fin;
        fin.open(filename);
        int lineNum=1;
        INSTR_NUM = 0;
        string str;
        while(getline(fin,str)){
            if(isEmpty(str)){
                lineNum++;
                continue;
            }
            bool correct_syntax = validate(str,filenum);
            if(!correct_syntax){
                cout<<"For file: "<<filename<<", Syntax error: At line number: "<<lineNum<<" : "<<str<<endl;
                fin.close();
                error[filenum]=1;
                break;
            }
            tokenise(lineNum,str,filenum);
            lineNum++;
        }
        if(error[filenum]){
            continue;
        }
        for(auto str:labels_used){
            if(label_map.find(str) == label_map.end()){
                cout<<"Syntax Error: Undefined label:"<<str<<"\n";
                return -1;
            }
        }
        fin.close();
        start_mem[filenum]= ((1<<20)/NUM_FILES)*filenum*8;
        while(start_mem[filenum]%32!=0){
            start_mem[filenum]++;
        }
        int i=0;
        int n=program.size();
        int m=start_mem[filenum];
        while(i<n){
            string cmd = program[i].command;
            vector<string> arg = program[i].param;
            if(arg.size() == 0){
                i++;
                continue;
            }
            int in = ins_map[cmd];
            string ins = intToBits4(in);
            copyToMem(ins,m);
            if(in==0 || in==1 || in==2 || in==5){
                putRegInMem(arg[0],m+4);
                putRegInMem(arg[1],m+9);
                putRegInMem(arg[2],m+14);
            }
            else if(in==3 || in==4){
                putRegInMem(arg[0],m+4);
                putRegInMem(arg[1],m+9);
                int v=label_map[arg[2]]/8;
                if(v>131071){
                    cout<<"Too Large Program!! Possibility of errors in storing labels\n";
                    return -1;
                }
                string ad = intToBits18(v);
                copyToMem(ad,m+14);
            }
            else if(in==6){
                int v=label_map[arg[0]]/8;
                if(v>131071){
                    cout<<"Too Large Program!! Possibility of errors in storing labels\n";
                    return -1;
                }
                string ad = intToBits28(v);
                copyToMem(ad,m+4);
            }
            else if(in==7 || in==8){
                putRegInMem(arg[0],m+4);
                putRegInMem(arg[1],m+9);
                int v=stoi(arg[2]);
                string ad = intToBits18(v);
                copyToMem(ad,m+14);
            }
            else{
                putRegInMem(arg[0],m+4);
                putRegInMem(arg[1],m+9);
                int v=stoi(arg[2]);
                string ad = intToBits18(v);
                copyToMem(ad,m+14);
            }

            m+=32;
            i++;
        }
    }

    for(int i=0;i<NUM_FILES-1;i++){
        end_mem[i]=start_mem[i+1];
        register_File[i][29] = end_mem[i];
    }
    end_mem[NUM_FILES-1]=(1<<20)*8;
    register_File[NUM_FILES-1][29] = end_mem[NUM_FILES-1];

    int TOTAL_INSTR_EXECUTED = 0;
    vector<int> program_counter(50,0);
    for(int i=0;i<NUM_FILES;i++){
        INSTR_MEM[i]+=start_mem[i];
        program_counter[i]=start_mem[i];
    }


    bool allDone=false;
    currentCycle=0;
    while(allDone==false && currentCycle<MAX_CLOCK_CYCLES){
        allDone=true;
        for(int filenum=0;filenum<NUM_FILES;filenum++){
            if(error[filenum]){
                continue;
            }
            if(storeToLoadTransfers.find(filenum)!=storeToLoadTransfers.end()&&storeToLoadTransfers[filenum]==currentCycle+1){
                continue;
            }
            int pc=program_counter[filenum];
            if(pc>=INSTR_MEM[filenum]){
                continue;
            }
            allDone=false;
            if(CLOCK_CYCLES[filenum]>currentCycle){
                continue;
            }
            int cmd = getFromMem(pc,4);
            if(cmd == 0){
                int dst = getFromMem(pc+4,5);
                int p1 = getFromMem(pc+9,5);
                int p2 = getFromMem(pc+14,5);

                executeDRAM({dst,p1,p2},filenum);
                CLOCK_CYCLES[filenum]++;
                if(dst!=0){
                    int val1 = register_file(p1,filenum);
                    int val2 = register_file(p2,filenum);
                    int val3 = val1 + val2;
                    if(add_overflow(val3,val1,val2)){
                        cout<<"File: "<<filenum+1<<", Memory Address: "<<pc<<" : Calculation overflow detected!!\n";
                        error[filenum]=1;
                        continue;
                    }
                    register_File[filenum][dst] = val3;

                    changeReg[filenum] = true;
                    changedRegister[filenum] = dst;
                    changedRegValue[filenum] = val3;
                }
                printStatus("add instruction",filenum);
            }
            else if(cmd == 1){
                int dst = getFromMem(pc+4,5);
                int p1 = getFromMem(pc+9,5);
                int p2 = getFromMem(pc+14,5);

                executeDRAM({dst,p1,p2},filenum);
                CLOCK_CYCLES[filenum]++;
                if(dst!=0){
                    int val1 = register_file(p1,filenum);
                    int val2 = register_file(p2,filenum);
                    int val3 = val1 - val2;
                    if(sub_overflow(val3,val1,val2)){
                        cout<<"File: "<<filenum+1<<", Memory Address: "<<pc<<" : Calculation overflow detected!!\n";
                        error[filenum]=1;
                        continue;
                    }
                    register_File[filenum][dst] = val3;

                    changeReg[filenum] = true;
                    changedRegister[filenum] = dst;
                    changedRegValue[filenum] = val3;
                }
                printStatus("sub instruction",filenum);
            }
            else if(cmd == 2){
                int dst = getFromMem(pc+4,5);
                int p1 = getFromMem(pc+9,5);
                int p2 = getFromMem(pc+14,5);

                executeDRAM({dst,p1,p2},filenum);
                CLOCK_CYCLES[filenum]++;
                if(dst!=0){
                    int val1 = register_file(p1,filenum);
                    int val2 = register_file(p2,filenum);
                    int val3 = val1 * val2;
                    if(mul_overflow(val3,val1,val2)){
                        cout<<"File: "<<filenum+1<<", Memory Address: "<<pc<<" : Calculation overflow detected!!\n";
                        error[filenum]=1;
                        continue;
                    }
                    register_File[filenum][dst] = val3;

                    changeReg[filenum] = true;
                    changedRegister[filenum] = dst;
                    changedRegValue[filenum] = val3;
                }
                printStatus("mul instruction",filenum);
            }
            else if(cmd == 3){
                int p1 = getFromMem(pc+4,5);
                int p2 = getFromMem(pc+9,5);

                executeDRAM({p1,p2},filenum);
                CLOCK_CYCLES[filenum]++;
                if(register_file(p1,filenum) == register_file(p2,filenum)){
                    pc = start_mem[filenum]+ getFromMem(pc+14,18)*8 - 32;
                }
                printStatus("beq instruction",filenum);
            }
            else if(cmd == 4){
                int p1 = getFromMem(pc+4,5);
                int p2 = getFromMem(pc+9,5);

                executeDRAM({p1,p2},filenum);
                CLOCK_CYCLES[filenum]++;
                if(register_file(p1,filenum) != register_file(p2,filenum)){
                    pc = start_mem[filenum]+ getFromMem(pc+14,18)*8 - 32;
                }
                printStatus("bne instruction",filenum);
            }
            else if(cmd == 5){
                int dst = getFromMem(pc+4,5);
                int p1 = getFromMem(pc+9,5);
                int p2 = getFromMem(pc+14,5);

                executeDRAM({dst,p1,p2},filenum);
                CLOCK_CYCLES[filenum]++;
                if(dst!=0 && register_file(p1,filenum) < register_file(p2,filenum)){
                    register_File[filenum][dst] = 1;
                    changeReg[filenum] = true;
                    changedRegister[filenum] = dst;
                    changedRegValue[filenum] = 1;
                }
                else if(dst!=0){
                    register_File[filenum][dst] = 0;
                    changeReg[filenum] = true;
                    changedRegister[filenum] = dst;
                    changedRegValue[filenum] = 0;
                }
                printStatus("slt instruction",filenum);
            }
            else if(cmd == 6){
                CLOCK_CYCLES[filenum]++;
                printStatus("j instruction",filenum);
                 pc = start_mem[filenum]+ getFromMem(pc+4,28)*8 - 32;
            }
            else if(cmd == 7){

                int r = getFromMem(pc+4,5);
                int a = getFromMem(pc+9,5);
                executeDRAM({a},filenum);
                int p = getFromMem(pc+14,18);
                if(r!=0){
                    int val = start_mem[filenum]+ p*8 +register_file(a,filenum)*8;
                    if(outOfBounds(val,filenum)){
                        cout<<"File: "<<filenum+1<<", Memory Address:"<<pc<<":Attempt to access memory failed!! Terminating Execution!\n";
                        error[filenum]=1;
                        continue;
                    }

                    putLw_SwInQueue(r,val,0,filenum);
                }
            }
            else if(cmd == 8){
                int r = getFromMem(pc+4,5);
                int a = getFromMem(pc+9,5);
                executeDRAM({a},filenum);
                int p = getFromMem(pc+14,18);
                int val = start_mem[filenum]+ p*8 +register_file(a,filenum)*8;
                if(outOfBounds(val,filenum)){
                    cout<<"File: "<<filenum+1<<", Memory Address:"<<pc<<":Attempt to access memory failed!! Terminating Execution!\n";
                    error[filenum]=1;
                    continue;
                }

                putLw_SwInQueue(r,val,1,filenum);
                changedMemLocation_map[val/8] = register_file(r,filenum);
            }
            else if(cmd == 9){
                int dst = getFromMem(pc+4,5);
                int r = getFromMem(pc+9,5);
                int c = getFromMem(pc+14,18);

                executeDRAM({dst,r},filenum);
                CLOCK_CYCLES[filenum]++;
                if(dst!=0){
                    int val1 = register_file(r,filenum);
                    int val2 = c;
                    int val3 = val1 + val2;
                    if(add_overflow(val3,val1,val2)){
                        cout<<"File: "<<filenum+1<<", Memory Address: "<<pc<<" : Calculation overflow detected!!\n";
                        error[filenum]=1;
                        continue;
                    }
                    register_File[filenum][dst] = register_file(r,filenum) + c;

                    changeReg[filenum] = true;
                    changedRegister[filenum] = dst;
                    changedRegValue[filenum] = register_File[filenum][dst];
                }
                printStatus("addi instruction",filenum);
            }
            pc+=32;
            program_counter[filenum]=pc;
            TOTAL_INSTR_EXECUTED++;
        }

        currentCycle++;
        if(DRAM.exec && currentCycle==DRAM.CLOCK_CYCLES_INSTRUCTED){
            printDRAMexecution();
        }

        if(!DRAM.exec && currentInsInQueue!=0){
            nextDRAM();
        }

    }

    if(currentCycle<MAX_CLOCK_CYCLES &&DRAM.exec){
        printDRAMexecution();
    }

    while(currentCycle<MAX_CLOCK_CYCLES && (!DRAMreq.empty()) ){
        nextDRAM();
        if(DRAM.START_CYCLE<MAX_CLOCK_CYCLES &&DRAM.exec){
            printDRAMexecution();
        }
        else{
            break;
        }
    }

    int executedCycles=max(currentCycle,MRMCYCLE);
    if(DRAM.exec==false){
        executedCycles=max(executedCycles,DRAM.CLOCK_CYCLES_INSTRUCTED);
    }
    else{
        executedCycles=max(executedCycles,DRAM.START_CYCLE);
    }
    // something is loaded in row buffer at least ones.
    if(DRAM.ACTIVATED_ROW_NUMBER!=-1 && executedCycles<MAX_CLOCK_CYCLES && DRAM.exec==false && currentInsInQueue==0){
        DRAM.CLOCK_CYCLES_INSTRUCTED = executedCycles;
        DRAMwriteback();
        cout<<"Clock Cycles: "<<executedCycles<<"-"<<DRAM.CLOCK_CYCLES_INSTRUCTED<<"\n";
        cout<<"Note: Automatic writeback of row buffer into the memory after execution of program completes. \n";
        executedCycles=DRAM.CLOCK_CYCLES_INSTRUCTED;
    }

    cout<<"\n";
    cout<<"Number of clock cycles: "<<executedCycles<<"\n";
    cout<<"Total Number of instructions executed: "<<TOTAL_INSTR_EXECUTED<<"\n";
    cout<<"Instructions per unit cycle: "<<(float)(TOTAL_INSTR_EXECUTED)/(float)(executedCycles)<<"\n";
    cout<<"Number of row buffer updates: "<<DRAM.ROW_BUFFER_UPDATE_COUNT<<"\n";
    for(int i=0;i<NUM_FILES;i++){
        if(error[i]){
            continue;
        }
        cout<<"Values at each register for register file of program "<<i+1<<" at the end of execution: \n";
        for(int j=0;j<32;j++){
            cout<<rev_register_map[j]<<": ";
            cout<<hex<<register_file(j,i)<<dec<<",";
        }
        cout<<"\n";
    }
    cout<<"\n";
    cout<<"Values at memory location at the end of execution(only the changed ones): \n";
    for(auto x:changedMemLocation_map){
        cout<<x.first<<"-"<<x.first+3<<" = "<<x.second<<"\n";
    }
    if(changedMemLocation_map.empty()){
        cout<<"NONE \n";
    }

    return 0;
}
