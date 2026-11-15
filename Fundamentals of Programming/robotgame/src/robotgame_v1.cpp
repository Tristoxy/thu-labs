#include<iostream>
#include<vector>
#include<string>
#include<sstream>
#include<fstream>
#include<map> //stl映射
#include<algorithm>
#include<iomanip>
#include<cctype> //处理字符
#include<cstdlib> //清屏
#include<utility> //pair
#include<windows.h>//处理终端乱码
#include<conio.h>

using namespace std;

//动画延迟（ms)
#define delay 2000;

void clearScreen();//得先声明！

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

//全局变量
vector<Level>levels;
string progressfile="game.txt";
string levelfile="level.txt";

// 动画
bool isAutoPlay = true;      // 默认自动播放
bool isPaused = false;       // 暂停状态
bool shouldRestart = false;  // 是否从头开始播放

//屏幕大小布局
class Screen 
{
 private:
    static const int WIDTH = 100;  // 容纳所有元素
    static const int HEIGHT = 26;  // 容纳更多信息
    char buffer[HEIGHT][WIDTH];

 public:
    Screen() { clear(); }

    void clear() 
    {
        for (int y = 0; y < HEIGHT; y++) 
        {
            for (int x = 0; x < WIDTH; x++) 
            {
                buffer[y][x] = ' ';
            }
        }
    }

    void setChar(int x, int y, char ch) 
    {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) 
        {
            buffer[y][x] = ch;
        }
    }

    void setString(int x, int y, const string &str) 
    {
        for (size_t i = 0; i < str.length() && x + static_cast<int>(i) < WIDTH;
             i++) 
        {
            setChar(x + static_cast<int>(i), y, str[i]);
        }
    }

    void render() 
    {
        clearScreen();

        // 使用回车符将光标移到开头，避免换行导致滚动
        for (int y = 0; y < HEIGHT; y++) 
        {
            for (int x = 0; x < WIDTH; x++) 
            {
                cout << buffer[y][x];
            }
            if (y < HEIGHT - 1) 
            {
                cout << endl;
            }
        }
        // 将光标移到屏幕左上角，准备下次渲染
        cout.flush();
    }

    char getChar(int x, int y) const 
    {
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) 
        {
            return buffer[y][x];
        }
        return ' ';
    }
};

//画输入传送带
class InputBelt 
{
 private:
    int x, y;

 public:
    InputBelt(int x, int y) : x(x), y(y) {}

    void update(Screen &screen, const vector<int> &inputBelt) 
    {
        int startY = y + 2;
        if (x - 3 >= 0) 
        {
            screen.setString(x - 3, startY, "IN");
        } 
        else 
        {
            screen.setString(x, startY - 1, "IN");
        }
        const int FIXED_POSITIONS = 6;

        for (int i = 0; i < FIXED_POSITIONS; i++) 
        {

            screen.setString(x, startY + i * 3, "+---+");

            string content = "|";
            if (i < (int)inputBelt.size()) 
            {
                int value = inputBelt[i];
                string valueStr = to_string(value);
                if (value >= 0 && value < 10) 
                {
                    content += " " + valueStr + " ";
                } 
                else if (value >= 10 && value < 100) 
                {
                    content += valueStr + " ";
                } 
                else if (value >= 100) 
                {
                    content += valueStr.substr(0, 3);
                } 
                else if (value < 0 && value > -10) 
                {
                    content += valueStr + " ";
                } 
                else if (value <= -10 && value > -100) 
                {
                    content += valueStr;
                } 
                else
                {
                    content += valueStr.substr(0, 3);
                }
            } 
            else 
            {
                // 没有数字，显示大写X
                content += " X ";
            }

            content += "|";
            screen.setString(x, startY + i * 3 + 1, content);
            screen.setString(x, startY + i * 3 + 2, "+---+");
        }
    }
};

//画输出传送带
class OutputBelt 
{
 private:
    int x, y;

 public:
    OutputBelt(int x, int y) : x(x), y(y) {}

    void update(Screen &screen, const vector<int> &outputBelt) 
    {
        int startY = y + 2;
        screen.setString(x + 6, startY, "OUT");
        const int FIXED_POSITIONS = 4;

        for (int i = 0; i < FIXED_POSITIONS; i++) 
        {
            screen.setString(x, startY + i * 3, "+---+");

            string content = "|";
            int reverseIndex = (int)outputBelt.size() - 1 - i;
            if (reverseIndex >= 0 && reverseIndex < (int)outputBelt.size()) 
            {
                int value = outputBelt[reverseIndex];
                string valueStr = to_string(value);
                if (value >= 0 && value < 10) 
                {
                    content += " " + valueStr + " ";
                } 
                else if (value >= 10 && value < 100) 
                {
                    content += valueStr + " ";
                } 
                else if (value >= 100) 
                {
                    content += valueStr.substr(0, 3);
                } 
                else if (value < 0 && value > -10) 
                {
                    content += valueStr + " ";
                } 
                else if (value <= -10 && value > -100) 
                {
                    content += valueStr;
                } 
                else 
                {
                    content += valueStr.substr(0, 3);
                }
            } 
            else 
            {
                content += " X ";
            }
            content += "|";
            screen.setString(x, startY + i * 3 + 1, content);
            screen.setString(x, startY + i * 3 + 2, "+---+");
        }
    }
};

//画机器人
class Robot 
{
 private:
    int baseX, y;  // baseX是输入传送带的位置，y是垂直位置

 public:
    Robot(int x, int y) : baseX(x), y(y) {}

    void update(Screen &screen, bool hasBlock, int num, int workerx) {
        //横坐标：baseX + workerx * 6（每步6格）
        int x = baseX + workerx * 6;

        //画机器人
        //画头部眼睛
        screen.setChar(x, y, '@');
        screen.setChar(x + 4, y, '@');
        screen.setString(x, y + 1, "-----");
        //画身体
        screen.setString(x, y + 2, "|@ @|");
        screen.setString(x + 2, y + 3, "+");
        //画手臂
        screen.setString(x, y + 4, "/   \\");
        //画腿
        screen.setString(x + 1, y + 5, "| |");

        //有积木的话，得显示当前积木
        if (hasBlock) {
            string blockStr = to_string(num);
            string content = "|";
            if (num >= 0 && num < 10) 
            {
                content += " " + blockStr + " ";
            } 
            else if (num >= 10 && num < 100) 
            {
                content += blockStr + " ";
            } 
            else if (num >= 100) 
            {
                content += blockStr.substr(0, 3);
            } 
            else if (num < 0 && num > -10) 
            {
                content += blockStr + " ";
            } 
            else if (num <= -10 && num > -100) 
            {
                content += blockStr;
            } 
            else 
            {
                content += blockStr.substr(0, 3);
            }
            content += "|";

            //画积木
            screen.setString(x, y - 3, "+---+");
            screen.setString(x, y - 2, content);
            screen.setString(x, y - 1, "+---+");
        }
    }
};

//画空地
class FloorArea 
{
 private:
    int x, y;

 public:
    FloorArea(int x, int y) : x(x), y(y) {}

    void update(Screen &screen, const map<int, int> &floors,
                const map<int, bool> &floorOccupied, int floorCount) {
        // 根据关卡定义的空地数量来显示
        if (floorCount == 0) {
            return;  // 如果关卡中空地数量为0，不显示空地
        }

        int cellWidth = 6;

        // 显示对应数量的空地
        for (int i = 0; i < floorCount; i++) {
            int cellX = x + i * cellWidth;

            // 绘制顶部边框
            screen.setString(cellX, y, "+---+");

            // 绘制内容
            string content = "|";
            if (floorOccupied.count(i) && floorOccupied.at(i)) {
                // 有数字，显示数字
                int value = floors.at(i);
                string valueStr = to_string(value);
                // 格式化数字，确保在3个字符宽度内
                if (value >= 0 && value < 10) {
                    content += " " + valueStr + " ";
                } else if (value >= 10 && value < 100) {
                    content += valueStr + " ";
                } else if (value >= 100) {
                    content += valueStr.substr(0, 3);
                } else if (value < 0 && value > -10) {
                    content += valueStr + " ";
                } else if (value <= -10 && value > -100) {
                    content += valueStr;
                } else {
                    content += valueStr.substr(0, 3);
                }
            } else 
            {
                // 没有使用，显示大写X
                content += " X ";
            }
            content += "|";
            screen.setString(cellX, y + 1, content);

            // 绘制底部边框
            screen.setString(cellX, y + 2, "+---+");

            // 绘制索引（0, 1, 2, 3）
            string indexStr = to_string(i);
            screen.setString(cellX + 2, y + 3, indexStr);
        }
    }
};

//画右边的可用指令集
class CodeDisplay 
{
 private:
    int x, y;

 public:
    CodeDisplay(int x, int y) : x(x), y(y) {}

    void update(Screen &screen, const vector<instruction> &program,
                int currentInstruction) 
    {
        int titleY = 3;
        screen.setString(x, titleY, "===== CODE =====");

        int startY = titleY + 2;
        int maxDisplay = 18;

        for (int i = 0; i < (int)program.size() && i < maxDisplay; i++) {
            string line;
            if (i == currentInstruction) 
            {
                line = "> ";
            } 
            else 
            {
                line = "  ";
            }

            line += to_string(i + 1) + " ";
            line += program[i].name;
            if (program[i].has_num) 
            {
                line += " " + to_string(program[i].num);
            }

            if ((int)line.length() > 29) 
            {
                line = line.substr(0, 26) + "...";
            }

            screen.setString(x, startY + i, line);
        }
    }
};

//必要的说明
class InfoPanel 
{
 public:
    void update(Screen &screen, const Level &level, int currentInstruction,
                int instrexe, const instruction &currentInst,
                const string &executionStatus = "",
                const vector<int> &outputBelt = vector<int>(),
                bool isPaused = false) 
        {

        string levelInfo = "Level " + to_string(level.num) + ": " + level.name;
        if ((int)levelInfo.length() > 45)
            levelInfo = levelInfo.substr(0, 42) + "...";
        screen.setString(0, 0, levelInfo);

        //游戏按键
        string keyHelp;
        if (isPaused) 
        {
            keyHelp = "空格:继续  回车:单步  左键:重置";
        } 
        else 
        {
            keyHelp = "空格:暂停  回车:单步  左键:重置";
        }
        int helpX = 100 - (int)keyHelp.length();
        if (helpX < 50) helpX = 50;               
        screen.setString(helpX, 0, keyHelp);

        string status = "执行指令: " + to_string(currentInstruction + 1) +
                        " (" + currentInst.name;
        if (currentInst.has_num) 
        {
            status += " " + to_string(currentInst.num);
        }
        status += ")    已执行: " + to_string(instrexe);

        if (!executionStatus.empty()) 
        {
            status += "    状态: " + executionStatus;
        }
        if ((int)status.length() > 80) status = status.substr(0, 77) + "...";
        screen.setString(0, 22, status);

        string targetStr = "目标输出: [";
        size_t maxShow = 8;
        for (size_t i = 0; i < level.out.size() && i < maxShow; i++) 
        {
            targetStr += to_string(level.out[i]);
            if (i < level.out.size() - 1 && i < maxShow - 1)
                targetStr += ", ";
        }
        if (level.out.size() > maxShow) targetStr += "...";
        targetStr += "]";
        if ((int)targetStr.length() > 80)
            targetStr = targetStr.substr(0, 77) + "...";
        screen.setString(0, 23, targetStr);

        string outputStr = "已输出: [";
        for (size_t i = 0; i < outputBelt.size() && i < maxShow; i++) 
        {
            outputStr += to_string(outputBelt[i]);
            if (i < outputBelt.size() - 1 && i < maxShow - 1) outputStr += ", ";
        }
        if (outputBelt.size() > maxShow) outputStr += "...";
        outputStr += "]";

        if ((int)outputStr.length() > 80)
            outputStr = outputStr.substr(0, 77) + "...";
        screen.setString(0, 24, outputStr);
    }
};

//先集成一些要用的函数

void initialize(); //游戏初始化
void loadgame(); //加载进度
void savegame(); //保存进度
void levelselection(); //关卡选择界面——功能1
bool selectinput(int &selectedLevel); //防止用户奇奇怪怪的输入
void levelinfo(Level &level); //关卡信息
vector<instruction>keyboard(); //键盘输入
vector<instruction>file(string &filename); //文件输入

//指令（逻辑+动画）
void animation(Level &level, const vector<instruction> &program);
//游戏主界面——功能2
void maingame(const Game &state, const Level &level,
                      int currentInstruction, const string &lastCommand,
                      const vector<instruction> &program);
//绘制界面
void renderFrame(Screen &screen, const Game &state, const Level &level,
                 const vector<instruction> &program, int currentInstruction);
void waitForInput(Screen *screen = nullptr, const Game *state = nullptr,
                  const Level *level = nullptr,
                  const vector<instruction> *program = nullptr,
                  int currentInstruction = -1);
//动画渲染
void animation(Level &level, const vector<instruction> &program);
bool checkout(const vector<int> &output, const vector<int> &target); //检查输出的对不对


//主函数在此
int main(void)
{
    SetConsoleOutputCP(65001); 
    SetConsoleCP(65001); //防乱码

    initialize();
    loadgame();

    while (true) {
        clearScreen();
        levelselection();

        int selectedLevel;
        if (!selectinput(selectedLevel)) 
        {
            // 退出前保存进度
            savegame();
            cout << "GOODBYE!!" << endl;
            break;
        }
        //如果关卡无效，设置为-1，继续循环
        if (selectedLevel < 0 || selectedLevel >= (int)levels.size()) {
            continue;
        }

        clearScreen();
        levelinfo(levels[selectedLevel]);

        cout <<endl<<"请选择输入方式"<<endl;
        cout <<"1.键盘输入"<< endl;
        cout <<"2.文件输入"<< endl;
        cout <<"请输入选项 (1/2): ";

        int choice;
        cin>>choice;
        cin.ignore();

        vector<instruction> program;

        if (choice == 1) {
            program=keyboard();
        } else if (choice == 2) {
            cout << "请输入文件路径:";
            string filename;
            getline(cin, filename);
            program=file(filename);
        } else {
            cout << "无效选择!返回主菜单。" << endl;
            cin.ignore();
            continue;
        }

        if (program.empty()) {
            cout << "程序为空!返回主菜单。" << endl;
            cin.ignore();
            continue;
        }

        animation(levels[selectedLevel], program);

        savegame();
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

void levelinfo(Level &level) 
{
    cout << "========================================"<< endl;
    cout << "  关卡 " << level.num<< ": " <<level.name<< endl;
    cout << "========================================"<< endl;
    cout << endl;

    cout << "输入序列: ";
    for (size_t i = 0; i < level.in.size(); i++) 
    {
        cout << level.in[i];
        if (i < level.in.size() - 1) cout << ", ";
    }
    cout << endl;

    cout << "目标输出: ";
    for (size_t i = 0; i < level.out.size(); i++) 
    {
        cout << level.out[i];
        if (i < level.out.size()-1) cout << ", ";
    }
    cout << endl;

    cout << "可用空地数: " << level.drawercnt<< endl;

    cout << "可用指令: ";
    for (size_t i = 0; i < level.instructions.size(); i++) 
    {
        cout << level.instructions[i];
        if (i < level.instructions.size() - 1) cout<<", ";
    }
    cout << endl;
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

void waitForInput(Screen *screen, const Game *state, const Level *level,
                  const vector<instruction> *program, int currentInstruction) 
    {
    auto rerenderIfNeeded = [&]() 
    {
        if (screen && state && level && program && currentInstruction >= 0) 
        {
            renderFrame(*screen, *state, *level, *program, currentInstruction);
        }
    };

    if (isAutoPlay && !isPaused) 
    {
        Sleep(1500);
        if (_kbhit()) 
        {
            int ch = _getch();
            if (ch == 224 || ch == 0) 
            {
                ch = _getch();
            }
            if (ch == 32) {
                isPaused = true;
                rerenderIfNeeded();
            }

            else if (ch == 75) {
                shouldRestart = true;
                return;
            }
        }

        if (isPaused) 
        {
            while (isPaused) 
            {
                int ch = _getch();
                if (ch == 224 || ch == 0) {
                    ch = _getch();
                }

                if (ch == 32) {
                    isPaused = false;
                    rerenderIfNeeded();
                    return;
                }

                else if (ch == 13) {
                    return;
                }

                else if (ch == 80 || ch == 77) {
                    isPaused = false;
                    rerenderIfNeeded();
                    return;
                }

                else if (ch == 75) {
                    shouldRestart = true;
                    isPaused = false;
                    return;
                }
            }
        }
        return;
    }


    int ch = _getch();
    if (ch == 224 || ch == 0) 
    {
        ch = _getch();
        if (ch == 80 || ch == 77) 
        {
            return;
        }
 
        if (ch == 75) 
        {
            shouldRestart = true;
            return;
        }
    }

    if (ch == 32) 
    {
        isPaused = !isPaused;
        rerenderIfNeeded();
        if (isPaused) 
        {

            while (isPaused) 
            {
                ch = _getch();
                if (ch == 224 || ch == 0) 
                {
                    ch = _getch();
                }

                if (ch == 32) 
                {
                    isPaused = false;
                    rerenderIfNeeded();
                    return;
                }
                else if (ch == 13) 
                {
                    return;
                }
                else if (ch == 80 || ch == 77) 
                {
                    isPaused = false;
                    rerenderIfNeeded(); 
                    return;
                }

                else if (ch == 75) 
                {
                    shouldRestart = true;
                    isPaused = false;
                    return;
                }
            }
        }
        return;
    }
    if (ch == 13) 
    {
        return;
    }
    while (true) 
    {
        ch = _getch();
        if (ch == 224 || ch == 0) 
        {
            ch = _getch();
            if (ch == 80 || ch == 77) 
            {
                return;
            }
        } 
        else if (ch == 13 || ch == 32) 
        {
            if (ch == 32) 
            {
                isPaused = !isPaused;
                rerenderIfNeeded();
                if (isPaused) 
                {
                    while (isPaused) {
                        ch = _getch();
                        if (ch == 224 || ch == 0) 
                        {
                            ch = _getch();
                        }
                        if (ch == 32) 
                        {
                            isPaused = false;
                            rerenderIfNeeded();
                            return;
                        } 
                        else if (ch == 13) 
                        {
                            return;
                        } 
                        else if (ch == 80 || ch == 77) 
                        {
                            isPaused = false;
                            rerenderIfNeeded();
                            return;
                        }

                        else if (ch == 75) 
                        {
                            shouldRestart = true;
                            isPaused = false;
                            return;
                        }
                    }
                }
            }
            return;
        }
    }
}

bool checkout(vector<int> &output,vector<int> &target) 
{
    if (output.size() != target.size()) return false;
    for (size_t i = 0; i < output.size(); i++) 
    {
        if (output[i] != target[i]) return false;
    }
    return true;
}

void maingame(const Game &state, const Level &level,
                      int currentInstruction, const string & /* lastCommand */,
                      const vector<instruction> &program) 
    {
    Screen screen;
    renderFrame(screen, state, level, program, currentInstruction);
}

void renderFrame(Screen &screen, const Game &state, const Level &level,
                 const vector<instruction> &program, int currentInstruction) 
    {
    // 1. 清空屏幕缓冲区
    screen.clear();

    // 2. 绘制背景（固定元素）- 通过各个对象绘制

    // 3. 更新并绘制动态对象
    InputBelt inputBelt(3, 1);
    inputBelt.update(screen, state.in);

    Robot robot(
        11, 6);
    robot.update(screen, state.hand, state.num,
                 state.workerx);

    FloorArea floorArea(11, 13); 
    floorArea.update(screen, state.drawercnt, state.draweroccupy,
                     level.drawercnt);

    OutputBelt outputBelt(38, 1);
    outputBelt.update(screen, state.out);

    // 在输出传送带和代码区域之间绘制垂直分隔线
    // 分隔线高度与输入传送带保持一致
    int separatorX = 49;  // 分隔线位置（向右平移一格）
    int inputStartY = 1 + 2;  // 输入传送带第一个积木顶部位置 (y=1时，startY=3)
    int inputEndY =
        inputStartY + 6 * 3 - 1;  // 6个积木，每个3行，最后一行是底部边框
    for (int i = inputStartY; i <= inputEndY; i++) {
        screen.setChar(separatorX, i, '|');
    }

    CodeDisplay codeDisplay(51, 1); 
    codeDisplay.update(screen, program, currentInstruction);

    string executionStatus;
    if (state.instrexe == 0) {

        executionStatus = "未开始";
    } else {
        // 判断程序是否执行结束的两种情况：
        // 1. 指令指针超出程序范围（正常执行完成）
        // 2. 需要执行inbox指令但输入传送带为空（输入耗尽导致结束）
        bool isFinished = false;

        // 情况1：指令指针超出程序范围
        if (state.instrptr>= (int)program.size()) {
            isFinished = true;
        }
        // 情况2：当前指令（传入的currentInstruction）是inbox且输入传送带为空
        // 注意：当inputBelt为空时，renderFrame传入的是originalIP（执行前的指令位置）
        else if (currentInstruction >= 0 &&
                 currentInstruction < (int)program.size() &&
                 program[currentInstruction].name== "inbox" &&
                 state.in.empty()) {
            isFinished = true;
        }

        if (isFinished) {
            // 执行结束：检查输出结果
            if (checkout(state.out, level.out)) 
            {
                executionStatus = "执行结束 - Success";
            } else {
                executionStatus = "执行结束 - Fail";
            }
        } else if (state.instrptr>= 0 &&
                   state.instrptr<(int)program.size()) {
            // 执行中：指令指针还在程序范围内
            executionStatus = isPaused ? "执行中(手动)" : "执行中(自动)";
        } else {
            // 其他情况，默认为未开始
            executionStatus = "未开始";
        }
    }

    InfoPanel infoPanel;
    if (currentInstruction >= 0 && currentInstruction < (int)program.size()) 
    {
        infoPanel.update(screen, level, currentInstruction,
                         state.instrexe,
                         program[currentInstruction], executionStatus,
                         state.out, isPaused);
    } 
    else if (!program.empty() && currentInstruction >= 0) 
    {
        // 如果指令指针超出范围，使用最后一个指令
        instruction lastInst = program.back();
        infoPanel.update(screen, level, (int)program.size() - 1,
                         state.instrexe, lastInst, executionStatus,
                         state.out, isPaused);
    } 
    else if (!program.empty()) 
    {
        // 如果currentInstruction无效但程序不为空，使用第一个指令
        infoPanel.update(screen, level, 0, state.instrexe,
                         program[0], executionStatus, state.out,
                         isPaused);
    } 
    else 
    {
        // 如果程序为空，使用默认指令
        instruction defaultInst;
        defaultInst.name="N/A";
        defaultInst.has_num = false;
        infoPanel.update(screen, level, 0, state.instrexe,
                         defaultInst, executionStatus, state.out,
                         isPaused);
    }

    // 4. 一次性渲染到屏幕
    screen.render();
}

void animation(Level &level, const vector<instruction> &program) {
    // 初始化动画控制状态
    isAutoPlay = true;  // 默认自动播放
    isPaused = false;   // 初始未暂停

    Game state;
    state.in=level.in;
    state.hand=false;
    state.instrptr = 0;
    state.instrexe = 0;
    state.workerx= 0;  // 初始位置在输入传送带（workerx=0）

    for (int i = 0; i < level.drawercnt; i++) 
    {
        state.draweroccupy[i] = false;
    }

    Screen screen;

    // 初始渲染
    if (!program.empty()) 
    {
        renderFrame(screen, state, level, program, 0);
        // 在屏幕缓冲区中显示提示信息
        screen.setString(0, 25, "按回车键开始模拟执行...");
        screen.render();
        // 等待用户按回车键开始模拟执行
        int ch;
        while (true) {
            ch = _getch();
            if (ch == 13) {  // 回车键
                break;
            }
        }
        // 清空提示信息
        renderFrame(screen, state, level, program, 0);
    }

    // 检查可用指令集
    auto isInstructionAvailable = [&](const string &cmd) 
    {
        return find(level.instructions.begin(),
                    level.instructions.end(),
                    cmd) != level.instructions.end();
    };

    // 主执行循环：支持从头开始模拟执行
    while (true) 
    {
        // 检查是否需要从头开始播放
        if (shouldRestart) {
            // 重新初始化状态
            state.in=level.in;
            state.out.clear();
            state.hand = false;
            state.instrptr = 0;
            state.instrexe = 0;
            state.workerx = 0;  // 重置机器人位置到输入传送带
            for (int i = 0; i < level.drawercnt; i++) {
                state.drawercnt[i] = false;
            }
            shouldRestart = false;
            // 重新渲染初始状态
            renderFrame(screen, state, level, program, 0);
            waitForInput();
            // 如果再次按了左键，继续循环
            if (shouldRestart) {
                continue;
            }
        }

        // 如果指令执行完成，显示结果并等待重置
        if (state.instrptr >= (int)program.size()) 
        {
            // 最终渲染
            renderFrame(screen, state, level, program,
                        state.instrptr);

            // 检查输出并设置通过状态
            if (checkout(state.out, level.out)) 
            {
                level.passed = true;
            }
            // 显示总共执行的指令数（显示在第25行，避免覆盖其他内容）
            string resultLine =
                "总共执行的指令数: " + to_string(state.instrexe);
            screen.setString(0, 25, resultLine);
            screen.render();

            // 等待用户按键，支持左键重置
            while (true) {
                waitForInput();
                // 如果按了向左键重启，继续循环重新开始
                if (shouldRestart) 
                {
                    break;  // 跳出内层循环，继续外层循环重新开始
                }
                // 按了其他键，显示返回主菜单提示（显示在第25行，覆盖之前的提示）
                screen.setString(0, 25, "按回车键返回主菜单（左键重置）...");
                screen.render();
                // 再次等待按键
                int ch = _getch();
                if (ch == 224 || ch == 0) {
                    ch = _getch();
                }
                // 回车键：返回主菜单
                if (ch == 13) {
                    return;  // 退出函数，返回主菜单
                }
                // 左键：重置动画
                else if (ch == 75) {
                    shouldRestart = true;
                    break;  // 跳出内层循环，继续外层循环重新开始
                }
                // 其他键：继续等待
            }
            // 如果按了向左键重置，继续循环重新开始
            if (shouldRestart) 
            {
                continue;
            }
            // 按了其他键，退出循环
            break;
        }

        int originalIP = state.instrptr;
        const instruction &instr = program[state.instrptr];

        // 构造显示的命令字符串
        string displayCommand = instr.name;
        if (instr.has_num) 
        {
            displayCommand += " " + to_string(instr.num);
        }

        // 检查指令是否在可用集中
        if (!isInstructionAvailable(instr.name)) 
        {
            string errorMsg =
                "Error on instruction " + to_string(originalIP + 1);
            screen.setString(0, 23, errorMsg);
            screen.render();
            waitForInput();
            if (shouldRestart) {
                continue;
            }
            return;
        }

        state.instrexe++;
        bool error = false;

        if (instr.name== "inbox") 
        {
            if (state.in.empty()) 
            {
                state.instrptr++;
                renderFrame(screen, state, level, program, originalIP);

                if (checkout(state.out, level.out)) 
                {
                    level.passed = true;
                }
 
                string resultLine = "总共执行的指令数: " +
                                    to_string(state.instrexe);
                screen.setString(0, 25, resultLine);
                screen.render();

                while (true) 
                {
                    waitForInput();

                    if (shouldRestart) 
                    {
                        break;
                    }
                    screen.setString(0, 25,
                                     "按回车键返回主菜单...");
                    screen.render();
                    int ch = _getch();
                    if (ch == 224 || ch == 0) 
                    {
                        ch = _getch();
                    }
                    if (ch == 13) 
                    {
                        return;
                    }

                    else if (ch == 75) 
                    {
                        shouldRestart = true;
                        break;
                    }
                }

                if (shouldRestart) 
                {
                    continue;
                }

                return;
            }

            if (state.workerx != 0) 
            {

                for (int step = state.workerx; step > 0; step--) 
                {
                    state.workerx = step - 1;
                    renderFrame(screen, state, level, program, originalIP);
                    waitForInput(&screen, &state, &level, &program, originalIP);
                    if (shouldRestart) break;
                }
                if (shouldRestart) continue;
            }
            // 取积木
            state.num = state.in[0];
            state.in.erase(state.in.begin());
            state.hand = true;
            state.instrptr++;
        } 
        else if (instr.name=="outbox") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else 
            {

                if (state.workerx != 3) 
                {

                    for (int step = state.workerx; step < 3; step++) 
                    {
                        state.workerx = step + 1;
                        renderFrame(screen, state, level, program, originalIP);
                        waitForInput(&screen, &state, &level, &program,
                                     originalIP);
                        if (shouldRestart) break;
                    }
                    if (shouldRestart) continue;
                }
                // 放下积木
                state.out.push_back(state.num);
                state.hand = false;
                state.instrptr++;
            }
        } 
        else if (instr.name== "add") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num< 0 || instr.num>= level.drawercnt ||
                       !state.draweroccupy[instr.num]) 
            {
                error = true;
            } else 
            {

                int targetX = instr.num;

                if (state.workerx != targetX) {

                    if (state.workerx < targetX) {

                        for (int step = state.workerx; step < targetX; step++) 
                        {
                            state.workerx = step + 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    } 
                    else 
                    {

                        for (int step = state.workerx; step > targetX; step--) 
                        {
                            state.workerx = step - 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    }
                }
                state.num += state.drawercnt[instr.num];
                state.instrptr++;
            }
        } 
        else if (instr.name=="sub") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num< 0 || instr.num>= level.drawercnt ||
                       !state.draweroccupy[instr.num]) 
            {
                error = true;
            } else 
            {

                int targetX = instr.num;
                //机器人不在目标空地，要先走过去
                if (state.workerx != targetX) {
                    // 根据当前位置和目标位置决定移动方向
                    if (state.workerx < targetX) 
                    {
                        // 向右移动
                        for (int step = state.workerx;step <targetX; step++) 
                        {
                            state.workerx = step + 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    } 
                    else 
                    
                    {
                        // 向左移动
                        for (int step = state.workerx; step > targetX; step--) 
                        {
                            state.workerx = step - 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    }
                }
                // 执行减法操作
                state.num -= state.drawercnt[instr.num];
                state.instrptr++;
            }
        } 
        else if (instr.name== "copyto") 
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
                if (state.workerx != targetX) 
                {

                    if (state.workerx < targetX) 
                    {
                        for (int step = state.workerx; step < targetX; step++) 
                        {
                            state.workerx = step + 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    } 
                    else
                    {

                        for (int step = state.workerx; step > targetX; step--) 
                        {
                            state.workerx = step - 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    }
                }

                state.drawercnt[instr.num] = state.num;
                state.draweroccupy[instr.num] = true;

                state.hand = false;
                state.instrptr++;
            }
        } 
        else if (instr.name== "copyfrom") 
        {
            if (instr.num < 0 || instr.num >= level.drawercnt ||
                !state.draweroccupy[instr.num]) 
            {
                error = true;
            }
            else 
            {
                int targetX = instr.num;

                if (state.workerx != targetX) 
                {
                    if (state.workerx < targetX) 
                    {

                        for (int step = state.workerx; step < targetX; step++) 
                        {
                            state.workerx = step + 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    } 
                    else 
                    {

                        for (int step = state.workerx; step > targetX; step--) 
                        {
                            state.workerx = step - 1;
                            renderFrame(screen, state, level, program,
                                        originalIP);
                            waitForInput(&screen, &state, &level, &program,
                                         originalIP);
                            if (shouldRestart) break;
                        }
                        if (shouldRestart) continue;
                    }
                }

                state.num = state.drawercnt[instr.num];
                state.hand = true;
                state.instrptr++;
            }
        } 
        else if (instr.name== "jump") 
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
        else if(instr.name=="jumpifzero") {
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
                if (state.num == 0) {
                    state.instrptr = instr.num - 1;
                } else {
                    state.instrptr++;
                }
            }
        } 
        else 
        {
            error = true;
        }

        renderFrame(screen, state, level, program, state.instrptr);

        if (error) 
        {
            string errorMsg =
                "Error on instruction " + to_string(originalIP + 1);
            screen.setString(0, 23, errorMsg);
            screen.render();
            waitForInput();
            return;
        }

        waitForInput(&screen, &state, &level, &program,
                     state.instrptr);

        if (shouldRestart) 
        {
            continue;
        }
    }
}

pair<int, int> animationOJ(Level &level,
                                vector<instruction> &program) 
    {
    Game state;
    state.in= level.in;
    state.hand= false;
    state.instrptr=0;
    state.instrexe=0;
    state.workerx=0;

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
    while (true) 
    {
        if (state.instrptr>= (int)program.size()) 
        {
            if (checkout(state.out,level.out)) 
            {
                return make_pair(0, 0);  // Success
            } else {
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

        if (instr.name=="inbox") {
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
            // 如果机器人不在输入传送带，需要先走回去
            if (state.workerx!= 0) {
                state.workerx=0;
            }
            // 取积木
            state.hand= state.in[0];
            state.in.erase(state.in.begin());
            state.hand= true;
            state.instrptr++;
        } 
        else if (instr.name== "outbox") 
        {
            if (!state.hand) 
            {
                error = true;
            } else {
                if (state.workerx!= 3) 
                {
                    state.workerx= 3;
                }
                // 放下积木
                state.out.push_back(state.num);
                state.hand = false;
                state.instrptr++;
            }
        } 
        else if (instr.name== "add") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num< 0 || instr.num>= level.drawercnt ||
                       !state.draweroccupy[instr.num]) 
            {
                error = true;
            } 
            else 
            {
                // 移动到目标空地
                int targetX=instr.num;
                state.workerx=targetX;
                // 执行加法操作
                state.num+= state.drawercnt[instr.num];
                state.instrptr++;
            }
        } 
        else if (instr.name=="sub") 
        {
            if (!state.hand) 
            {
                error = true;
            } 
            else if (instr.num < 0 || instr.num >= level.drawercnt ||
                       !state.drawercnt[instr.num]) 
            {
                error = true;
            } 
            else 
            {
                // 移动到目标空地
                int targetX = instr.num;
                state.workerx= targetX;
                // 执行减法操作
                state.num-= state.drawercnt[instr.num];
                state.instrptr++;
            }
        } 
        else if (instr.name== "copyto") 
        {
            if (!state.hand) {
                error = true;
            } else if (instr.num< 0 || instr.num>= level.drawercnt) 
            {
                error = true;
            }
            else 
            {
                // 移动到目标空地
                int targetX = instr.num;
                state.workerx= targetX;
                // 执行复制到空地操作
                state.drawercnt[instr.num] = state.num;
                state.draweroccupy[instr.num] = true;
                state.hand=false;
                state.instrptr++;
            }
        } 
        else if (instr.name== "copyfrom") 
        {
            if (instr.num < 0 || instr.num >= level.drawercnt ||
                !state.drawercnt[instr.num]) 
            {
                error = true;
            } 
            else 
            {
                // 移动到目标空地
                int targetX=instr.num;
                state.workerx= targetX;
                // 执行从空地复制操作
                state.num= state.drawercnt[instr.num];
                state.hand= true;
                state.instrptr++;
            }
        } 
        else if (instr.name== "jump") 
        {
            if (instr.num < 1 || instr.num > (int)program.size()) 
            {
                error = true;
            } 
            else 
            {
                state.instrptr = instr.num-1;
            }
        } 
        else if (instr.name== "jumpifzero") 
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
                if (state.num== 0) 
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

bool checkout(const vector<int> &output, const vector<int> &target) {
    if (output.size() != target.size()) return false;
    for (size_t i = 0; i < output.size(); i++) 
    {
        if (output[i] != target[i]) return false;
    }
    return true;
}

void clearScreen() { system("cls"); }
