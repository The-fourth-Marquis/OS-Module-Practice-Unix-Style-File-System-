#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "Components.h"
#include <fstream>
#include <ctime>
#include <iomanip>
#include <string>
#include <iostream>
#include <cstring>
#include <vector>
#include <cstdlib>

enum State {
    DIR_NOT_EXIST = 0,
    SUCCESS = 1,
    NO_FILENAME = 2,
    FILE_EXISTS = 3,
    LENGTH_EXCEED = 4,
    DIRECTORY_EXCEED = 5,
    NO_ENOUGH_SPACE = 6,
    NO_SUCH_FILE = 7,
    NO_DIRNAME = 8,
    NO_SUCH_DIR = 9,
    DIR_NOT_EMPTY = 10,
    CAN_NOT_DELETE_TEMP_DIR = 11,
    DIR_EXISTS = 12
};

class Filesystem
{
private:
    char curpath[MAX_PATH];
    std::FILE *fp;
    bool flag;
    Superblock superblock;
    INode root_Inode;
    INode cur_Inode;
    
public:
    Filesystem();
    ~Filesystem();

    bool getFlag();
    void modifyBitmap(int pos, int start);
    INode readInode(int pos);
    void writeInode(int pos, INode inode);
    int numberOfAvailableBlock();
    INode findNextInode(INode inode, std::string fileName);
    int findAvailablePos(std::string type);
    void tip();
    State fileNameProcess(std::string fileName, std::vector<std::string> &v, INode &inode);
    void writeFileToDentry(File file, INode inode);
    void writeRandomStringToBlock(int blockid);
    void writeAddressToBlock(Address address, int blockid, int offset);
    void deleteFileFromDentry(INode inode, std::string fileName);

    void welcome();
    void initialize();
    void giveState(std::string command, State st);
    
    void help();
    void sum();
    void dir();
    State createFile(std::string fileName, int fileSize);
    State deleteFile(std::string fileName);
    State createDir(std::string dirName);
    State deleteDir(std::string dirName);
    State changeDir(std::string path);
    State cp(std::string file1, std::string file2);
    State cat(std::string fileName);
};

#endif