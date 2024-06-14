#include "Filesystem.h"

using namespace std;

enum class CommandType {
    CREATE_FILE,
    DELETE_FILE,
    CREATE_DIR,
    DELETE_DIR,
    CHANGE_DIR,
    DIR,
    CP,
    SUM,
    CAT,
    HELP,
    EXIT,
    UNKNOWN
};

// 函数将字符串命令转换为枚举类型
CommandType stringToCommandType(const std::string& cmd) {
    if (cmd == "createFile") return CommandType::CREATE_FILE;
    if (cmd == "deleteFile") return CommandType::DELETE_FILE;
    if (cmd == "createDir") return CommandType::CREATE_DIR;
    if (cmd == "deleteDir") return CommandType::DELETE_DIR;
    if (cmd == "changeDir") return CommandType::CHANGE_DIR;
    if (cmd == "dir") return CommandType::DIR;
    if (cmd == "cp") return CommandType::CP;
    if (cmd == "sum") return CommandType::SUM;
    if (cmd == "cat") return CommandType::CAT;
    if (cmd == "help") return CommandType::HELP;
    if (cmd == "exit") return CommandType::EXIT;
    return CommandType::UNKNOWN;
}

bool commandNumTest(int sz, int num, const std::string cmd){
    if(sz > num) cout << "Command \'" << cmd << "\': too many parameters" << endl;
    else if(sz < num) cout << "Command \'" << cmd << "\': too few parameters" << endl;
    else return true;

    return false;
}

int main() {
    Filesystem filesystem;
    bool ifExit = false;
    filesystem.initialize();
    filesystem.welcome();

    if(!filesystem.getFlag()) {
        return 0;
    }
    while(true){
        filesystem.tip();

        string input;
        getline(cin, input);
        int len = input.size();

        vector<string> command;
        string temp = "";
        for(int i = 0; i < len; ++i) {
            if(input[i] == ' ') {
                if(temp != "") {
                    command.push_back(temp);
                    temp = "";
                }
                continue;
            }
            temp += input[i];
        }
        if(temp != "") command.push_back(temp);

        int sz = command.size();
        if(sz == 0) continue;

        CommandType cmdType = stringToCommandType(command[0]);

        switch (cmdType) {
            case CommandType::CREATE_FILE:
                if(commandNumTest(sz,3,"createFile")){
                    try {
                        int res = std::stoi(command[2]);
                        filesystem.giveState("createFile", filesystem.createFile(command[1], res));
                    } catch (const std::invalid_argument&) {
                        cout << "Command 'createFile': Not correct parameter for fileSize." << endl;  // 如果转换失败则返回-1
                    } catch (const std::out_of_range&) {
                        cout << "Command 'createFile': Not correct parameter for fileSize." << endl;  // 如果转换结果超出目标类型范围则返回-1
                    }
                }
                break;

            case CommandType::DELETE_FILE:
                if(commandNumTest(sz,2,"deleteFile"))  filesystem.giveState("deleteFile", filesystem.deleteFile(command[1]));
                break;

            case CommandType::CREATE_DIR:
                if(commandNumTest(sz,2,"createDir")) filesystem.giveState("createDir", filesystem.createDir(command[1]));
                break;

            case CommandType::DELETE_DIR:
                if(commandNumTest(sz,2,"deleteDir")) filesystem.giveState("deleteDir", filesystem.deleteDir(command[1]));
                break;
            
            case CommandType::CHANGE_DIR:
                if(commandNumTest(sz,2,"changeDir")) filesystem.giveState("changeDir", filesystem.changeDir(command[1]));
                break;

            case CommandType::DIR:
                if(commandNumTest(sz,1,"dir")) filesystem.dir();
                break;

            case CommandType::CP:
                if(commandNumTest(sz,3,"cp")) filesystem.giveState("cp", filesystem.cp(command[1], command[2]));
                break;

            case CommandType::SUM:
                if(commandNumTest(sz,1,"sum")) filesystem.sum();
                break;

            case CommandType::CAT:
                if(commandNumTest(sz,2,"cat")) filesystem.giveState("cat", filesystem.cat(command[1]));
                break;

            case CommandType::HELP:
                if(commandNumTest(sz,1,"help")) filesystem.help();
                break;

            case CommandType::EXIT:
                if(commandNumTest(sz,1,"exit")) ifExit = true;
                break;

            case CommandType::UNKNOWN:

            default:
                cout << command[0] << ": command not found" << endl;
                break;
            }

        if(ifExit) break;
    }
    return 0;
}
