#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <algorithm>
#include <utility>

using namespace std;

//关卡信息
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

//游戏情况
struct Game
{
    vector<int>in;
    vector<int>out;
    map<int,int>drawercnt;
    map<int,bool>draweroccupy;
    int instrptr;
    int instrexe;
    //worker
    int num;
    int hand;
    int workerx;//位置相对传送带
};

//指令
struct instruction
{
    string name;
    int num;
    bool has_num;
};

// 全局变量
vector<Level> levels;

//函数声明
void initialize();

pair<int, int> executeProgramOJ(Level &level,
                                const vector<instruction> &program);
bool checkout(const vector<int> &output, const vector<int> &target);

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
    L3.name="渐入佳境!";
    L3.in={6,2,7,7,-9,3,-3,-3};
    L3.out={7,-3};
    L3.drawercnt=3;
    L3.instructions={"inbox","outbox","copyfrom",
                      "copyto","add","sub",
                      "jump","jumpifzero"};
    L3.passed = false;
    levels.push_back(L3);
}

bool checkout(const vector<int> &output, const vector<int> &target) 
{
    if (output.size() != target.size()) return false;
    for (size_t i = 0; i < output.size(); i++) 
    {
        if (output[i] != target[i]) return false;
    }
    return true;
}

pair<int, int> executeProgramOJ(Level &level,
                                const vector<instruction> &program) 
{
    Game state;
    state.in= level.in;
    state.hand = false;
    state.instrptr = 0;
    state.instrexe = 0;
    state.workerx = 0;

    for (int i = 0; i < level.drawercnt; i++) 
    {
        state.draweroccupy[i] = false;
    }

    // 检查可用指令集
    auto isInstructionAvailable = [&](const string &cmd) 
    {
        return find(level.instructions.begin(),
                    level.instructions.end(),
                    cmd) != level.instructions.end();
    };

    // 主执行循环
    while (true) {
        // 如果指令执行完成
        if (state.instrptr >= (int)program.size()) 
        {
            // 检查输出结果
            if (checkout(state.out, level.out)) 
            {
                return make_pair(0, 0);  // Success
            } 
            else 
            {
                return make_pair(1, 0);  // Fail
            }
        }

        int originalIP = state.instrptr;
        const instruction &instr = program[state.instrptr];

        if (!isInstructionAvailable(instr.name)) 
        {
            return make_pair(2, originalIP + 1);  // Error on instruction X
        }

        state.instrexe++;
        bool error = false;

        if (instr.name == "inbox") 
        {
            if (state.in.empty()) 
            {
                state.instrptr++;
                if (checkout(state.out, level.out)) 
                {
                    return make_pair(0, 0);  // Success
                } 
                else 
                {
                    return make_pair(1, 0);  // Fail
                }
            }
            if (state.workerx != 0) 
            {
                state.workerx = 0;
            }
            // 取积木
            state.num = state.in[0];
            state.in.erase(state.in.begin());
            state.hand = true;
            state.instrptr++;
        } 
        else if (instr.name == "outbox") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else 
            {
                if (state.workerx != 3) 
                {
                    state.workerx = 3;
                }
                // 放下积木
                state.out.push_back(state.num);
                state.hand = false;
                state.instrptr++;
            }
        } 
        else if (instr.name == "add") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num < 0 || instr.num >= level.drawercnt ||
                       !state.draweroccupy[instr.num]) 
            {
                error = true;
            } 
            else 
            {
                int targetX = instr.num;
                state.workerx = targetX;
                state.num += state.drawercnt[instr.num];
                state.instrptr++;
            }
        } 
        else if (instr.name == "sub") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num < 0 || instr.num >= level.drawercnt ||
                       !state.draweroccupy[instr.num]) 
            {
                error = true;
            } 
            else 
            {
                int targetX = instr.num;
                state.workerx = targetX;
                state.num -= state.drawercnt[instr.num];
                state.instrptr++;
            }
        } 
        else if (instr.name == "copyto") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num < 0 || instr.num >= level.drawercnt) 
            {
                error = true;
            } 
            else 
            {
                int targetX = instr.num;
                state.workerx = targetX;
                state.drawercnt[instr.num] = state.num;
                state.draweroccupy[instr.num] = true;
                state.hand = false;
                state.instrptr++;
            }
        } 
        else if (instr.name == "copyfrom") 
        {
            if (instr.num < 0 || instr.num >= level.drawercnt ||
                !state.draweroccupy[instr.num]) 
            {
                error = true;
            } 
            else 
            {
                int targetX = instr.num;
                state.workerx = targetX;
                state.num = state.drawercnt[instr.num];
                state.hand = true;
                state.instrptr++;
            }
        } 
        else if (instr.name == "jump") 
        {
            if (instr.num < 1 || instr.num > (int)program.size()) 
            {
                error = true;
            } 
            else 
            {
                state.instrptr = instr.num - 1;
            }
        } 
        else if (instr.name == "jumpifzero") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num < 1 || instr.num > (int)program.size()) 
            {
                error = true;
            } 
            else 
            {
                if (state.num == 0) 
                {
                    state.instrptr = instr.num - 1;
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

        if (error) 
        {
            return make_pair(2, originalIP + 1);  // Error on instruction X
        }
    }
}

int main(int argc, char *argv[]) 
{
    initialize();

    int levelId;
    if (!(cin >> levelId)) 
    {
        cout << "Fail" << endl;
        return 0;
    }

    int instructionCount;
    if (!(cin >> instructionCount)) 
    {

        cout << "Fail" << endl;
        return 0;
    }
    cin.ignore();

    if (levelId < 1 || levelId > (int)levels.size() || instructionCount < 1 ||
        instructionCount > 100) 
    {
        cout << "Fail" << endl;
        return 0;
    }

    vector<instruction> program;
    for (int i = 0; i < instructionCount; i++) 
    {
        string line;
        if (!getline(cin, line)) 
        {
            cout << "Fail" << endl;
            return 0;
        }

        stringstream ssin(line);
        instruction instr;
        ssin>> instr.name;

        if (instr.name == "inbox" || instr.name == "outbox") 
        {
            string remaining;
            if (ssin>> remaining) 
            {

                cout << "Error on instruction " << (i + 1) << endl;
                return 0;
            }
            instr.has_num = false;
            instr.num = 0;
        } 
        else 
        {
            if (ssin>> instr.num) 
            {
                instr.has_num = true;
            } 
            else 
            {
                instr.has_num = false;
                instr.num = 0;
            }
        }

        program.push_back(instr);
    }


    Level &level = levels[levelId - 1];
    pair<int, int> result = executeProgramOJ(level, program);

    if (result.first == 0) 
    {
        cout << "Success" << endl;
    } 
    else if (result.first == 1) 
    {
        cout << "Fail" << endl;
    } 
    else 
    {
        cout << "Error on instruction " << result.second << endl;
    }

    return 0;
}
