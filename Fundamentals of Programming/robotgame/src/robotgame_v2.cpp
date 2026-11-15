#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <algorithm>
#include <iomanip>
#include <cstdlib>
#include <windows.h>

using namespace std;

// 关卡配置结构
struct Level 
{
    int num;
    string name;
    vector<int>in;
    vector<int>out;
    int drawercnt;
    vector<string>instructions;
    bool passed;
};

// 游戏状态
struct Game 
{
    vector<int>in;
    vector<int>out;
    map<int, int>drawercnt;
    map<int, bool>draweroccupy;
    int num;
    bool hand;
    int instrptr;
    int instrexe;
};

// 指令结构
struct instruction 
{
    string name;
    int num;
    bool has_num;
};

// 全局变量
vector<Level>levels;
string levelfile="level2.txt";

//先集成一些要用的函数

void initialize(); //游戏初始化
void loadgame(); //加载进度
void savegame(); //保存进度
void levelselection(); //关卡选择界面——功能1
bool selectinput(int &selectedLevel); //防止用户奇奇怪怪的输入
void inoutline(const vector<int>& sequence);
void levelinfo(const Level &level); //关卡信息
vector<instruction>keyboard(); //键盘输入
vector<instruction>file(string &filename); //文件输入
//游戏主界面——功能2
void maingame(Level& level, const vector<instruction>& program);
void displaygame(const Game& state, const Level& level, int currentinstruction, const string& lastname);
bool checkOutput(const vector<int>& output, const vector<int>& target); //检查输出的对不对
void clearScreen();

int main(void) 
{
    SetConsoleOutputCP(65001);
    system("chcp 65001>nul");
    initialize();
    loadgame();
    
    while (true) 
    {
        clearScreen();
        levelselection();
        
        int selectedLevel;
        if (!selectinput(selectedLevel)) 
        {
            // 退出前保存进度
            cout << "GOODBYE!!" << endl;
            break;
        }
        //如果关卡无效，设置为-1，继续循环
        if (selectedLevel < 0 || selectedLevel >= (int)levels.size()) 
        {
            continue;
        }

        
        clearScreen();
        levelinfo(levels[selectedLevel]);
        
        cout <<endl<<"请选择输入方式"<<endl;
        cout <<"1.键盘输入"<< endl;
        cout <<"2.文件输入"<< endl;
        cout <<"请输入选项 (1/2): ";
        
        int choice;
        cin >> choice;
        cin.ignore();
        
        vector<instruction> program;
        
        if (choice == 1) 
        {
            program=keyboard();
        } 
        else if (choice == 2) 
        {
            cout << "请输入文件路径:";
            string filename;
            getline(cin, filename);
            program=file(filename);
        } 
        else 
        {
            cout << "无效选择!返回主菜单。" << endl;
            cin.ignore();
            continue;
        }

        if (program.empty()) 
        {
            cout << "程序为空!返回主菜单。" << endl;
            cin.ignore();
            continue;
        }
        
        maingame(levels[selectedLevel], program);
        
        savegame();
        
        cout << "\n按回车键返回主菜单...";
        cin.ignore();
    }
    
    return 0;
}

void initialize() 
{
    Level L1;
    L1.num=1;
    L1.name="小试牛刀！";
    L1.in={1,2};
    L1.out={1,2};
    L1.drawercnt=0;
    L1.instructions= {"inbox","outbox"};
    L1.passed=false;
    levels.push_back(L1);

    Level L2;
    L2.num=2;
    L2.name="锋芒毕露！";
    L2.in={3,9,5,1,-2,-2,9,-9};
    L2.out={-6,6,4,-4,0,0,18,-18};
    L2.drawercnt=3;
    L2.instructions={"inbox","outbox","copyfrom",
                      "copyto","add","sub",
                      "jump","jumpifzero"};
    L2.passed=false;
    levels.push_back(L2);

    Level L3;
    L3.num=3;
    L3.name="渐入佳境! ";
    L3.in={6,2,7,7,-9,3,-3,-3};
    L3.out={7,-3};
    L3.drawercnt=3;
    L3.instructions={"inbox","outbox","copyfrom",
                      "copyto","add","sub",
                      "jump","jumpifzero"};
    L3.passed = false;
    levels.push_back(L3);

    Level L4;
    L4.num = 4;
    L4.name = "挑战一下! ";
    L4.in = { 7,1,5,1,0,0,5 };
    L4.out = { 56,8,40,8,0,0,40 };
    L4.drawercnt=1;
    L4.instructions = { "inbox","outbox","copyfrom",
                     "copyto","add","sub",
                     "jump","jumpifzero" };
    L4.passed = false;
    levels.push_back(L4);
}

void loadgame() 
{
    //读最高关卡
    ifstream fin(levelfile);
    if (fin.is_open()) {
        int maxLevel;
        if (fin>>maxLevel) {
            //更低的关卡都通过了
            for (int i = 0; i < (int)levels.size() && i < maxLevel; i++) {
                levels[i].passed = true;
            }
        }
        fin.close();
    }
}

void savegame() 
{
    int maxLevel = 0;
    for (const auto &level : levels) {
        if (level.passed && level.num>maxLevel) {
            maxLevel = level.num;
        }
    }
    // 保存最高关卡
    if (maxLevel > 0) {
        ofstream fout(levelfile);
        if (fout.is_open()) {
            fout<<maxLevel<<endl;
            fout.close();
        }
    }
}

void levelselection()
{
    cout<<"请选择你心仪的关卡~"<<endl<<endl;

    for(size_t i=0; i<levels.size();i++)
    {
        cout <<" 关卡 "<< (i + 1) << ": " << levels[i].name;

        if (levels[i].passed) {
            cout << " 已通过ovo";
        } else if (i == 0 || levels[i - 1].passed) {
            cout << " 可进入TvT";
        } else {
            cout << " 未解锁qwq";
        }
        cout << endl;
    }

    cout << endl;
    cout << "输入 0 退出游戏~" << endl;
}

bool selectinput(int &selectedLevel)
{
    cout << "请选择关卡 (0-" << levels.size() << "): ";

    if (!(cin >> selectedLevel)) 
    {
        cin.clear();
        cin.ignore(10000, '\n');
        cout << "无效输入！试试输入数字吧~" << endl;
        cin.ignore();
        return true;
    }

    if (selectedLevel == 0) 
    {
        return false;
    }

    if (selectedLevel < 1 || selectedLevel > (int)levels.size()) 
    {
        cout << "关卡正在开发中！！" << endl;
        cout << "按回车键返回关卡选择界面~" << endl;
        cin.ignore();
        cin.ignore();
        selectedLevel = -1;
        return true;
    }

    selectedLevel--;

    if (selectedLevel > 0 && !levels[selectedLevel - 1].passed) 
    {
        cout << "还想跳关！" << endl;
        cout << "按回车键返回关卡选择界面~" << endl;
        cin.ignore();
        cin.ignore();
        selectedLevel = -1;
        return true;
    }

    return true;
}

void levelinfo(const Level& level) {
    cout << "========================================\n" << endl;
    cout << "  关卡 " << level.num << ": " << level.name << endl;
    cout << "========================================\n\n" << endl;
    cout << endl;
    
    cout << "输入序列: \n";
    inoutline(level.in);
    cout << "\n";
    
    cout << "目标输出: \n";
    inoutline(level.out);
    cout << "\n";
    
    cout << "可用空地数: " << level.drawercnt << "\n";
    
    cout << "可用指令: ";
    for (size_t i = 0; i < level.instructions.size(); i++) {
        cout << level.instructions[i];
        if (i < level.instructions.size() - 1) cout << ", ";
    }
    cout << "\n\n";
}

vector<instruction> keyboard() 
{
    vector<instruction> program;

    cout <<endl<<"请输入指令数量: ";
    int count;
    cin >> count;
    cin.ignore();

    cout << "请依次输入指令（指令+操作数）:" << endl;

    for (int i = 0; i < count; i++) {
        cout << "指令 " << (i + 1) << ": ";
        string line;
        getline(cin, line);

        stringstream ssin(line);
        instruction instr;
        ssin>> instr.name;

        if (ssin>> instr.num) 
        {
            instr.has_num=true;
        } else {
            instr.has_num=false;
            instr.num=0;
        }

        program.push_back(instr);
    }

    return program;
}

vector<instruction> file(string &filename) 
{
    vector<instruction> program;

    string filepath = filename;
    if (filename.find('/') == string::npos &&
        filename.find('\\') == string::npos) 
    {
    }
    ifstream fin(filepath);

    if (!fin.is_open()) 
    {
        cout << "无法打开文件: " << filepath << endl;
        return program;
    }

    int count;
    fin>>count;
    fin.ignore();

    for (int i = 0; i < count; i++) 
    {
        string line;
        getline(fin, line);

        stringstream ssin(line);
        instruction instr;
        ssin>>instr.name;

        if (ssin>>instr.num) 
        {
            instr.has_num= true;
        } 
        else 
        {
            instr.has_num=false;
            instr.num=0;
        }

        program.push_back(instr);
    }

    fin.close();
    return program;
}

void inoutline(const vector<int>& sequence) 
{
    // 第一行：上边框
    for (size_t i = 0; i < sequence.size(); i++) 
    {
        cout << "+---+";
        if (i < sequence.size() - 1) cout << " ";
    }
    cout << "\n";
    
    // 第二行：数值
    for (size_t i = 0; i < sequence.size(); i++) 
    {
        cout << "|" << setw(2) << sequence[i] << " |";
        if (i < sequence.size() - 1) cout << " ";
    }
    cout << "\n";
    
    // 第三行：下边框
    for (size_t i = 0; i < sequence.size(); i++) 
    {
        cout << "+---+";
        if (i < sequence.size() - 1) cout << " ";
    }
    cout << "\n";
}

void displaygame(const Game& state, const Level& level, int currentinstruction, const string& lastname) {
    cout << "\n========================================" << endl;
    cout << "执行指令 " << currentinstruction << ": " << lastname << endl;
    cout << "========================================" << endl;
    cout << endl;
    
    // 显示输入带
    cout << "输入带:\n";
    inoutline(level.in);
    cout << "\n";
    
    // 显示当前积木
    cout << "机器人手中持有 (Robot):\n";
    if (state.hand) 
    {
        vector<int> temp = {state.num};
        inoutline(temp);
        cout << "@   @\n";
        cout << "-----\n";
        cout << "|@ @|\n";
        cout << "  +  \n";
        cout << "/   \\"<<endl;
        cout << " | | \n";
    } 
    else 
    {
        cout << "+---+\n";
        cout << "|   |\n";
        cout << "+---+\n";
        cout << "@   @\n";
        cout << "-----\n";
        cout << "|@ @|\n";
        cout << "  +  \n";
        cout << "/   \\"<<endl;
        cout << " | | \n";
        cout << "(空)\n";
    }
    cout << "\n";
    
      // 显示空地状态
    cout << "空地状态 (Ground):\n";
    // 第一行：上边框
    for (int i = 0; i < level.drawercnt; i++) 
    {
        cout << "+---+";
        if (i < level.drawercnt - 1) cout << " ";
    }
    cout << "\n";
    
    // 第二行：数值
    for (int i = 0; i < level.drawercnt; i++) 
    {
        if (state.draweroccupy.count(i) && state.draweroccupy.at(i)) {
            cout << "|" << setw(2) << state.drawercnt.at(i) << " |";
        } else {
            cout << "|   |";
        }
        if (i < level.drawercnt - 1) cout << " ";
    }
    cout << "\n";
    
    // 第三行：下边框
    for (int i = 0; i < level.drawercnt; i++) 
    {
        cout << "+---+";
        if (i < level.drawercnt - 1) cout << " ";
    }
    cout << "\n";
    
    // 第四行：地址编号
    for (int i = 0; i < level.drawercnt; i++) 
    {
        cout << " " << i << " ";
        if (i < level.drawercnt - 1) cout << " ";
    }
    cout << "\n\n";
    
    // 显示输出带
    cout << "输出带 (OutBox Line):\n";
    if (!state.out.empty()) 
    {
        inoutline(state.out);
    } 
    else 
    {
        cout << "(空)\n";
    }
    cout << "\n";
    
    cout << "已执行指令数: " << state.instrexe << endl;
    cout << "========================================" << endl;
}

void maingame(Level& level, const vector<instruction>& program) 
{
    Game state;
    state.in = level.in;
    state.hand = false;
    state.instrptr = 0;
    state.instrexe = 0;
    
    for (int i = 0; i < level.drawercnt; i++) 
    {
        
        state.draweroccupy[i] = false;
    }
    
    cout << "\n开始执行程序...\n" << endl;
    
    // 检查可用指令集
    auto isinstructionAvailable = [&](const string& cmd) 
    {
        return find(level.instructions.begin(), 
                   level.instructions.end(), cmd) != level.instructions.end();
    };
    
    while (state.instrptr < program.size()) 
    {
        int originalIP = state.instrptr;
        const instruction& inst = program[state.instrptr];
        
        // 构造显示的命令字符串
        string displayname = inst.name;
        if (inst.has_num) {
            displayname += " " + to_string(inst.num);
        }
        
        // 检查指令是否在可用集中
        if (!isinstructionAvailable(inst.name)) {
            cout << "Error on instruction " << (originalIP + 1) << endl;
            return;
        }
        
        state.instrexe++;
        bool error = false;
        string errorMsg;
        
        if (inst.name == "inbox") 
        {
            if (state.in.empty()) 
            {
                state.instrptr++;
                displaygame(state, level, originalIP + 1, displayname);
                break;
            }
            state.num = state.in[0];
            state.in.erase(state.in.begin());
            state.hand = true;
            state.instrptr++;
            
        } 
        else if (inst.name == "outbox") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else 
            {
                state.out.push_back(state.num);
                state.hand = false;
                state.instrptr++;
            }
            
        } 
        else if (inst.name == "add") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (inst.num < 0 || inst.num >= level.drawercnt || 
                      !state.draweroccupy[inst.num]) 
                      {
                error = true;
            } 
            else 
            {
                state.num += state.drawercnt[inst.num];
                state.instrptr++;
            }
            
        } 
        else if (inst.name == "sub") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (inst.num < 0 || inst.num >= level.drawercnt || 
                      !state.draweroccupy[inst.num]) 
                      {
                error = true;
            } 
            else 
            {
                state.num -= state.drawercnt[inst.num];
                state.instrptr++;
            }
            
        } 
        else if (inst.name == "copyto") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (inst.num < 0 || inst.num >= level.drawercnt) 
            {
                error = true;
            } 
            else 
            {
                state.drawercnt[inst.num] = state.num;
                state.draweroccupy[inst.num] = true;
                state.instrptr++;
            }
            
        } 
        else if (inst.name == "copyfrom") 
        {
            if (inst.num < 0 || inst.num >= level.drawercnt || 
               !state.draweroccupy[inst.num]) 
            {
                error = true;
            } 
            else 
            {
                state.num = state.drawercnt[inst.num];
                state.hand = true;
                state.instrptr++;
            }
            
        } 
        else if (inst.name == "jump") 
        {
            if (inst.num < 1 || inst.num > program.size()) 
            {
                error = true;
            } 
            else 
            {
                state.instrptr = inst.num - 1;
            }
            
        } 
        else if (inst.name == "jumpifzero") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (inst.num < 1 || inst.num > program.size()) 
            {
                error = true;
            } 
            else 
            {
                if (state.num == 0) {
                    state.instrptr = inst.num - 1;
                } 
                else 
                {
                    state.instrptr++;
                }
            }
            
        } 
        else 
        {
            error = true;
        }
        
        displaygame(state, level, originalIP + 1, displayname);
        
        if (error) 
        {
            cout << "\nError on instruction " << (originalIP + 1) << endl;
            return;
        }
    }
    
    // 检查输出
    cout << "\n========================================" << endl;
    if (checkOutput(state.out, level.out)) 
    {
        cout << "Success" << endl;
        level.passed = true;
    } 
    else 
    {
        cout << "Fail" << endl;
    }
    cout << "总共执行的指令数: " << state.instrexe << endl;
    cout << "========================================" << endl;
}

bool checkOutput(const vector<int>& output, const vector<int>& target) 
{
    if (output.size() != target.size()) return false;
    for (size_t i = 0; i < output.size(); i++) 
    {
        if (output[i] != target[i]) return false;
    }
    return true;
}

void clearScreen() 
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
