#include "Filesystem.h"

int FILE_PER_BLOCK = BLOCK_SIZE/sizeof(File);
int SUM_OF_DIRECTORY = FILE_PER_BLOCK*NUM_DIRECT_ADDRESS;
char buffer[SYSTEM_SIZE];

Filesystem::Filesystem()
{
    strcpy(curpath,root_dir);
    flag = true;
}

Filesystem::~Filesystem()
{
}

bool Filesystem::getFlag(){
    return this->flag;
}

void Filesystem::modifyBitmap(int pos, int start){
    /*
        在磁盘上的块位图中修改指定位置（pos）的块状态。块位图是一种跟踪文件系统中数据块使用情况的方法，
        通常每个比特（bit）对应文件系统中的一个数据块，1表示该块已分配，0表示未分配。
    */
    int origin = pos/8;
    int offset = pos%8;
    unsigned char byte;

    fseek(fp, start+origin, SEEK_SET);// in byte
    fread(&byte, sizeof(unsigned char), 1, fp);

    byte = (byte ^ (1<<offset));//change the state

    fseek(fp, start+origin, SEEK_SET);
    fwrite(&byte, sizeof(unsigned char), 1, fp);
}

void Filesystem::writeInode(int pos, INode inode) {
    fseek(fp, INODE_TABLE_START+sizeof(INode)*pos, SEEK_SET);
    fwrite(&inode, sizeof(INode), 1, fp);
}

INode Filesystem::readInode(int pos) {
    INode inode;
    fseek(fp, INODE_TABLE_START+sizeof(INode)*pos, SEEK_SET);
    fread(&inode, sizeof(INode), 1, fp);
    return inode;
}

int Filesystem::numberOfAvailableBlock() {
    int unused = 0;
    fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
    for(int i = 0; i < BLOCK_BITMAP_SIZE; ++i) {
        unsigned char byte;
        fread(&byte, sizeof(unsigned char), 1, fp);
        for(int j = 0; j < 8; ++j) {
            if(((byte>>j)&1) == 0) unused++;
        }
    }
    return unused;
}

INode Filesystem::findNextInode(INode inode, std::string fileName) {// to find corresponding file in inode, the pass in inode is just a copy
    int cnt = inode.mcount;
    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(cnt == 0) break;
        fseek(fp, BLOCK_SIZE*inode.dir_address[i], SEEK_SET);
        for(int j = 0; j < FILE_PER_BLOCK; ++j) {
            if(cnt == 0) break;
            cnt--;
            File file;
            fread(&file, sizeof(File), 1, fp);
            if(file.inode_id == -1) continue;
            if(strcmp(file.filename, fileName.c_str()) == 0) {
                return readInode(file.inode_id);
            }
        }
    }
    inode.id = -1;// to repersent unfind
    return inode;
}

int Filesystem::findAvailablePos(std::string type) {
    fseek(fp, (type =="inode"? INODE_BITMAP_START : BLOCK_BITMAP_START), SEEK_SET);
    int pos = -1;
    for(int i = 0; i < (type =="inode" ? INODE_BITMAP_SIZE : BLOCK_BITMAP_SIZE); ++i) {
        unsigned char byte;
        fread(&byte, sizeof(unsigned char), 1, fp);
        for(int j = 0; j < 8; ++j) {
            if(((byte>>j)&1) == 0) {
                pos = i*8+j;
                break;
            }
        }
        if(pos != -1) break;
    }
    return pos;
}

void Filesystem::writeFileToDentry(File file, INode inode) {
    int cnt = inode.count;

    if(inode.mcount == cnt) {
        if(cnt % FILE_PER_BLOCK == 0) {
            inode.dir_address[cnt/FILE_PER_BLOCK] = findAvailablePos("block");
            modifyBitmap(inode.dir_address[cnt/FILE_PER_BLOCK],BLOCK_BITMAP_START);//keypoint,new 
        }
        fseek(fp, BLOCK_SIZE*inode.dir_address[cnt/FILE_PER_BLOCK]+sizeof(File)*(cnt%FILE_PER_BLOCK), SEEK_SET);// put in the last pos
        fwrite(&file, sizeof(File), 1, fp);

        inode.mcount++;
    }
    else {//reuse the vacancy
        bool ok = false;
        int temp = inode.mcount;
        for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {//step into the buffer 
            if(temp == 0) break;
            for(int j = 0; j < FILE_PER_BLOCK; ++j) {
                if(temp == 0) break;
                temp--;
                File tempfile;
                fseek(fp, BLOCK_SIZE*inode.dir_address[i]+sizeof(File)*j, SEEK_SET);
                fread(&tempfile, sizeof(File), 1, fp);
                if(tempfile.inode_id == -1) {// find the unused pos
                    ok = true;
                    fseek(fp, BLOCK_SIZE*inode.dir_address[i]+sizeof(File)*j, SEEK_SET);
                    fwrite(&file, sizeof(File), 1, fp);
                }
                if(ok) break;
            }
            if(ok) break;
        }
    }

    inode.count++;
    writeInode(inode.id, inode);//update
    //keep the consistency of the buffer
    if(inode.id == cur_Inode.id) cur_Inode = readInode(cur_Inode.id);
    if(inode.id == root_Inode.id) root_Inode = readInode(root_Inode.id);
}

void Filesystem::deleteFileFromDentry(INode inode, std::string fileName) {
    int cnt = inode.mcount;
    bool ok = false;

    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(cnt == 0) break;
        fseek(fp, BLOCK_SIZE*inode.dir_address[i], SEEK_SET);
        for(int j = 0; j < FILE_PER_BLOCK; ++j) {
            if(cnt == 0) break;
            cnt--;
            File file;
            fread(&file, sizeof(File), 1, fp);
            if(file.inode_id == -1) continue;
            if(strcmp(file.filename, fileName.c_str()) == 0) {
                
                fseek(fp, -int(sizeof(File)), SEEK_CUR);
                file.inode_id = -1;// set the pos to unused
                fwrite(&file, sizeof(File), 1, fp);
                ok = true;
            }
            if(ok) break;
        }
        if(ok) break;
    }

    inode.count--;
    writeInode(inode.id, inode);
}

void Filesystem::writeRandomStringToBlock(int blockid) {
    srand(time(0));
    fseek(fp, blockid*BLOCK_SIZE, SEEK_SET);
    char randomChar[BLOCK_SIZE];
    for(int i = 0; i < BLOCK_SIZE; ++i) randomChar[i] = (rand() % 26) + 'a';
    fwrite(&randomChar, sizeof(char)*BLOCK_SIZE, 1, fp);
}

void Filesystem::writeAddressToBlock(Address address, int blockid, int offset) {
    fseek(fp, blockid*BLOCK_SIZE+offset*sizeof(Address), SEEK_SET);
    fwrite(&address, sizeof(Address), 1, fp);
}

State Filesystem::fileNameProcess(std::string fileName, std::vector<std::string> &v, INode &inode){
    int namelen = (int)fileName.size();
    
    if(fileName[0] == '/') inode = root_Inode;
    else inode = cur_Inode;
    
    std::string temp = "";
    for(int i = 0; i < namelen; ++i) {
        if(fileName[i] == '/') {
            if(temp != "") {
                v.push_back(temp);
                temp = "";
            }
            continue;
        }
        temp += fileName[i];
    }
    
    /*warranty*/
    if(temp == "") return NO_FILENAME;
    if((int)temp.size() >= MAX_FILENAME_SIZE) return LENGTH_EXCEED;

    v.push_back(temp);

    for(int i = 0; i < (int)v.size() - 1; ++i) {
        INode nextInode = findNextInode(inode, v[i]);
        if(nextInode.id == -1 || nextInode.fmode != DENTRY_MODE) return DIR_NOT_EXIST;

        inode = nextInode;
    }
    
    return SUCCESS;
}

void Filesystem::welcome(){
    std::cout << std::endl;

    std::cout<<"Welcome :P" << std::endl;
    std::cout << std::endl;

    std::cout<<"The information of our group is as followed: " << std::endl;
    std::cout<<"Name:\t\tHou Wang\t\tQixiang Xu\t\tPeilin Yue" << std::endl;
    std::cout<<"Student ID:\t202130430249\t\t202130430409\t\t202130430096" << std::endl;
    std::cout << std::endl;

    help();
}

void Filesystem::initialize() {
    std::fstream file;
    file.open(HOME, std::ios::in);

    if(!file) {//initializing
        std::cout << "Initializing the Operating System..." << std::endl;
        fp = fopen(HOME, "wb+");

        if(fp == NULL) {
            std::cout << "Error while creating the Operating System..." << std::endl;
            flag = false;
            return;
        }
        // create a 16MB space for OS
        fwrite(buffer, SYSTEM_SIZE, 1, fp); 

        /*overhead*/

        // write superblock
        fseek(fp, 0, SEEK_SET);
        superblock.systemsize = SYSTEM_SIZE;
        superblock.blocksize = BLOCK_SIZE;
        superblock.blocknum = BLOCK_NUM;
        superblock.address_length = ADDRESS_LENGTH;
        superblock.max_filename_size = MAX_FILENAME_SIZE;
        superblock.superblock_size = BLOCK_SIZE;
        superblock.inode_size = sizeof(INode);
        superblock.inode_bitmap_size = INODE_BITMAP_SIZE;
        superblock.block_bitmap_size = BLOCK_BITMAP_SIZE;
        superblock.inode_table_size = INODE_TABLE_SIZE;
        fwrite(&superblock, sizeof(Superblock), 1, fp); 

        // initialize inode bitmap
        fseek(fp, INODE_BITMAP_START, SEEK_SET);
        for(int i = 0; i < INODE_BITMAP_SIZE; ++i) {
            unsigned char byte = 0;
            fwrite(&byte, sizeof(unsigned char), 1, fp);
        } 

        // initialize block bitmap
        fseek(fp, BLOCK_BITMAP_START, SEEK_SET);
        for(int i = 0; i < BLOCK_BITMAP_SIZE; ++i) {
            unsigned char byte = 0;
            fwrite(&byte, sizeof(unsigned char), 1, fp);
        } 

        // set block of superblock busy
        modifyBitmap(0,BLOCK_BITMAP_START); 

        // set block of inode bitmap busy
        for(int i = 0; i < INODE_BITMAP_SIZE/1024; ++i) {
            modifyBitmap(i+1,BLOCK_BITMAP_START);
        } 

        // set block of block bitmap busy
        for(int i = 0; i < BLOCK_BITMAP_SIZE/1024; ++i) {
            modifyBitmap(i+INODE_BITMAP_SIZE/1024+1,BLOCK_BITMAP_START);
        } 

        // set block of inode table busy
        for(int i = 0; i < INODE_TABLE_SIZE/1024; ++i) {
            modifyBitmap(i+INODE_BITMAP_SIZE/1024+BLOCK_BITMAP_SIZE/1024+1,BLOCK_BITMAP_START);
        } 

        // set block of root inode busy
        memset(root_Inode.dir_address, 0, sizeof(root_Inode.dir_address));
        memset(root_Inode.indir_address, 0, sizeof(root_Inode.indir_address));
        root_Inode.count = 0;
        root_Inode.mcount = 0;
        root_Inode.ctime = time(NULL);
        root_Inode.fmode = DENTRY_MODE;
        root_Inode.id = 0;
        modifyBitmap(0,INODE_BITMAP_START);
        writeInode(0, root_Inode);

        cur_Inode = root_Inode;
    }
    else {
        std::cout << "Loading the Operating System..." << std::endl<< std::endl;
        fp = fopen(HOME, "rb+");

        if(fp == NULL) {
            std::cout << "Error while loading the Operating System..." << std::endl;
            flag = false;
            return;
        }

        fseek(fp, 0, SEEK_SET);
        fread(&superblock, sizeof(Superblock), 1, fp);

        cur_Inode = root_Inode = readInode(0);
    }

    std::cout<<"The basic information of this os is as followed: " << std::endl;
    std::cout << "System Size: " << superblock.systemsize << " Bytes" << std::endl;
    std::cout << "Block Size: " << superblock.blocksize << " Bytes" << std::endl;
    std::cout << "INode Bitmap Size: " << superblock.inode_bitmap_size << " Bytes" << std::endl;
    std::cout << "Block Bitmap Size: " << superblock.inode_bitmap_size << " Bytes" << std::endl;
    std::cout << "INode Table Size: " << superblock.inode_table_size << " Bytes" << std::endl;
    std::cout << "Number of Blocks: " << superblock.blocknum << std::endl;
    std::cout << std::endl;

    sum();
}

void Filesystem::tip() {
    std::cout << "[root@os]:" << curpath << ">";
}

void Filesystem::help(){
    std::cout << "Following commands are supported in this OS:" << std::endl;

    std::cout << "createFile <fileName> <fileSize>" << std::endl;
    std::cout << "\tCreate a file with a fixed size(in KB)." << std::endl;
    std::cout << "\te.g. createFile /dir/myFile 10" << std::endl<< std::endl;

    std::cout << "deleteFile <fileName>" << std::endl;
    std::cout << "\tDelete a file if it exists." << std::endl;
    std::cout << "\te.g. deleteFile /dir/myFile" << std::endl<< std::endl;

    std::cout << "createDir <dirPath>" << std::endl;
    std::cout << "\tCreate a directory." << std::endl;
    std::cout << "\te.g. createDir /dir/sub" << std::endl<< std::endl;

    std::cout << "deleteDir <dirPath>" << std::endl;
    std::cout << "\tDelete a directory if it exists." << std::endl;
    std::cout << "\te.g. deleteDir /dir/sub" << std::endl<< std::endl;

    std::cout << "changeDir <dirPath>" << std::endl;
    std::cout << "\tChange the current working directory." << std::endl;
    std::cout << "\te.g. changeDir /dir" << std::endl<< std::endl;

    std::cout << "dir" << std::endl;
    std::cout << "\tList all the files and sub-directories under current working directory." << std::endl<< std::endl;

    std::cout << "cp <fileName1> <fileName2>" << std::endl;
    std::cout << "\tCopy a file(file1) to another file(file2)." << std::endl;
    std::cout << "\te.g. cp file1 file2" << std::endl<< std::endl;

    std::cout << "sum" << std::endl;
    std::cout << "\tDisplay the usage of storage blocks." << std::endl<< std::endl;

    std::cout << "cat <fileName>" << std::endl;
    std::cout << "\tPrint out the contents of the file on the terminal." << std::endl;
    std::cout << "\te.g. cat /dir/myFile" << std::endl<< std::endl;

    std::cout << "help" << std::endl;
    std::cout << "\tDisplay the available commands in this OS." << std::endl<< std::endl;

    std::cout << "exit" << std::endl;
    std::cout << "\tQuit this program, but save the changes." << std::endl<< std::endl;

    std::cout << std::endl;
}

void Filesystem::sum() {
    int unused = numberOfAvailableBlock();
    int used = BLOCK_NUM - unused;

    std::cout << "The usage of storage space(in number of blocks): "<< std::endl;
    std::cout << "Number of Blocks that are used: " << used << std::endl;
    std::cout << "Number of Blocks that are available: " << unused << std::endl;
    std::cout << std::endl;
}

void Filesystem::giveState(std::string command, State st){
    if(st != SUCCESS)
        std::cout << "Command \'" << command << "\': ";

    switch (st)
    {
    case DIR_NOT_EXIST:
        std::cout << "The directory does not exist!" << std::endl;
        break;
    
    case NO_FILENAME:
        std::cout << "No file name provided!" << std::endl;
        break;

    case FILE_EXISTS:
        std::cout << "The file has existed!" << std::endl;
        break;

    case LENGTH_EXCEED:
        std::cout << "The length of the file name exceeds!" << std::endl;
        break;

    case DIRECTORY_EXCEED:
		std::cout << "Too many files in the directory!" << std::endl;
        break;

    case NO_ENOUGH_SPACE:
		std::cout << "The system has no enough space for the file!" << std::endl;
	    break;

    case NO_SUCH_FILE:
		std::cout << "No such file!" << std::endl;
	    break;

    case NO_DIRNAME:
		std::cout << "No directory name provided!" << std::endl;
        break;

    case NO_SUCH_DIR:
		std::cout << "No such directory!" << std::endl;
	    break;

    case DIR_NOT_EMPTY:
		std::cout << "The directory is not empty!" << std::endl;
	    break;

    case CAN_NOT_DELETE_TEMP_DIR:
		std::cout << "You cannot delete the current directory!" << std::endl;
	    break;

    case DIR_EXISTS:
		std::cout << "The directory has existed!" << std::endl;
        break;
    
    default:
        break;
    }
}

void Filesystem::dir() {
    cur_Inode = readInode(cur_Inode.id);
    int cnt = cur_Inode.mcount;

    std::cout << std::left << std::setw(20) << "Name";
    std::cout << std::left << std::setw(7) << "Type";
    std::cout << std::right << std::setw(30) << "Created Time";
    std::cout << std::right << std::setw(17) << "Length(in KB)";
    std::cout << " ";
    
    std::cout << std::endl;
    std::cout << std::left << std::setw(20) << "----";
    std::cout << std::left << std::setw(7) << "----";
    std::cout << std::right << std::setw(30) << "------------";
    std::cout << std::right << std::setw(17) << "-------------";
    std::cout << " ";
    
    std::cout << std::endl;

    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(cnt == 0) break;
        for(int j = 0; j < FILE_PER_BLOCK; ++j) {
            if(cnt == 0) break;
            cnt--;

            File file;
            fseek(fp, BLOCK_SIZE*cur_Inode.dir_address[i]+sizeof(File)*j, SEEK_SET);
            fread(&file, sizeof(File), 1, fp);
            if(file.inode_id == -1) continue;

            INode inode = readInode(file.inode_id);

            std::cout << std::left << std::setw(20) << file.filename;
            std::cout << std::left << std::setw(7) << (inode.fmode == DENTRY_MODE ? "Dentry" : "File");
            char buffer[50];
            //strftime(buffer, 40, "%a %b %d %T %G", localtime(&inode.ctime));
            strftime(buffer, 40, "%T %a %b %d %G", localtime(&inode.ctime));
            std::cout << std::right << std::setw(30) << buffer;
            std::cout << std::right << std::setw(17) << inode.fsize;
            std::cout << " ";
            
            std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

State Filesystem::createFile(std::string fileName, int fileSize) {
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(fileName,v,inode);
    if(temp != State::SUCCESS) return temp;

    int cnt = (int)v.size();
    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id != -1) return FILE_EXISTS;

    if(inode.count >= SUM_OF_DIRECTORY) return DIRECTORY_EXCEED;
    if(numberOfAvailableBlock()*BLOCK_SIZE < fileSize) return NO_ENOUGH_SPACE;
    if(fileSize > (MAX_FILE_SIZE/1024)) return NO_ENOUGH_SPACE;

    /*save*/
    File file;
    file.inode_id = findAvailablePos("inode");
    modifyBitmap(file.inode_id,INODE_BITMAP_START);
    strcpy(file.filename, v[cnt-1].c_str());

    INode newInode;
    memset(newInode.dir_address, 0, sizeof(newInode.dir_address));
    memset(newInode.indir_address, 0, sizeof(newInode.indir_address));
    newInode.fsize = fileSize;
    newInode.ctime = time(NULL);
    newInode.fmode = FILE_MODE;
    newInode.id = file.inode_id;

    writeFileToDentry(file, inode);

    int temp_fileSize = fileSize;
    //different from saving to date entry
    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(temp_fileSize == 0) break;
        
        temp_fileSize--;
        newInode.dir_address[i] = findAvailablePos("block");
        modifyBitmap(newInode.dir_address[i],BLOCK_BITMAP_START);
        writeRandomStringToBlock(newInode.dir_address[i]);
    }
    //overwhelm
    if(temp_fileSize>0) {
        newInode.indir_address[0] = findAvailablePos("block");
        modifyBitmap(newInode.indir_address[0], BLOCK_BITMAP_START); // indirect address
        int cnt = 0;
        while(temp_fileSize>0) {
            temp_fileSize--;
            int blockid = findAvailablePos("block");
            modifyBitmap(blockid, BLOCK_BITMAP_START);
            Address address;
            address.setblockID(blockid);
            address.setOffset(0);//Question
            writeRandomStringToBlock(blockid);
            writeAddressToBlock(address, newInode.indir_address[0], cnt);
            cnt++;
        }
    }

    writeInode(newInode.id, newInode);//update !!!
    return SUCCESS;
}

State Filesystem::deleteFile(std::string fileName) {
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(fileName,v,inode);
    if(temp != State::SUCCESS) return temp;

    int cnt = (int)v.size();
    
    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id == -1 || nextInode.fmode == DENTRY_MODE) return NO_SUCH_FILE;

    deleteFileFromDentry(inode, v[cnt-1]);

    /*deleting from the disk*/
    inode = nextInode;
    modifyBitmap(inode.id,INODE_BITMAP_START);// set unoccupied
    int filesize = inode.fsize;

    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(filesize == 0) break;
        filesize--;
        modifyBitmap(inode.dir_address[i],BLOCK_BITMAP_START);
    }

    if(filesize>0) {
        modifyBitmap(inode.indir_address[0],BLOCK_BITMAP_START);
        int offset = 0;

        while(filesize>0) {
            filesize--;
            Address address;
            fseek(fp, inode.indir_address[0]*BLOCK_SIZE+offset*sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);

            modifyBitmap(address.getblockID(),BLOCK_BITMAP_START);
            offset++;
        }
    }

    return SUCCESS;
}

State Filesystem::createDir(std::string dirName) {
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(dirName,v,inode);
    if(temp != State::SUCCESS){
        if(temp == State::NO_FILENAME) return State::NO_DIRNAME;
        else return temp;
    } 

    int cnt = (int)v.size();
  
    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id != -1) return DIR_EXISTS;

    if(inode.count >= SUM_OF_DIRECTORY) return DIRECTORY_EXCEED;

    File file;
    file.inode_id = findAvailablePos("Inode");
    modifyBitmap(file.inode_id, INODE_BITMAP_START);
    strcpy(file.filename, v[cnt-1].c_str());

    INode newInode;
    memset(newInode.dir_address, 0, sizeof(newInode.dir_address));
    memset(newInode.indir_address, 0, sizeof(newInode.indir_address));
    newInode.fsize = 0;
    newInode.ctime = time(NULL);
    newInode.fmode = DENTRY_MODE;
    newInode.id = file.inode_id;

    writeFileToDentry(file, inode);
    writeInode(newInode.id, newInode);
    return SUCCESS;
}

State Filesystem::deleteDir(std::string dirName) {
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(dirName,v,inode);
    if(temp != State::SUCCESS){
        if(temp == State::NO_FILENAME) return State::NO_DIRNAME;
        else return temp;
    } 

    int cnt = (int)v.size();

    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id == -1|| nextInode.fmode == FILE_MODE) return NO_SUCH_DIR;
    //check for current dir
    if(dirName[0] == '/') {
        std::vector<std::string> cur;
        std::string tempdir = "";
        int sz = strlen(curpath);

        for(int i = 1; i < sz; ++i) {
            if(curpath[i] == '/') {
                if(tempdir != "") {
                    cur.push_back(tempdir);
                    tempdir = "";
                }
                continue;
            }
            tempdir += curpath[i];
        }
        if(tempdir != "") cur.push_back(tempdir);

        int curlen = (int)cur.size();

        if(cnt <= curlen) {
            bool ok = true;
            for(int i = 0; i < cnt; ++i) {
                if(v[i] != cur[i]) {
                    ok = false;
                    break;
                }
            }
            if(ok) return CAN_NOT_DELETE_TEMP_DIR;
        }
    }

    if(nextInode.count > 0) return DIR_NOT_EMPTY;

    deleteFileFromDentry(inode, v[cnt-1]);

    inode = nextInode;
    modifyBitmap(inode.id, INODE_BITMAP_START);
    int count = inode.mcount;
    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(count <= 0) break;
        count -= FILE_PER_BLOCK;
        modifyBitmap(inode.dir_address[i],BLOCK_BITMAP_START);
    }

    return SUCCESS;
}

State Filesystem::changeDir(std::string path) {
    if(path == "/") {//case 1
        cur_Inode = root_Inode;
        strcpy(curpath, root_dir);
        return SUCCESS;
    }
    //vertification
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(path,v,inode);
    if(temp != State::SUCCESS){
        if(temp == State::NO_FILENAME) return State::NO_DIRNAME;
        else return temp;
    } 

    int cnt = (int)v.size();
    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id == -1|| nextInode.fmode != DENTRY_MODE) return DIR_NOT_EXIST;
    
    cur_Inode = nextInode;
    //reset
    if(path[0] == '/')  strcpy(curpath, root_dir);
    for(int i = 0; i < cnt; ++i) {
            strcat(curpath, "/");
            strcat(curpath, v[i].c_str());
        }

    return SUCCESS;
}

State Filesystem::cat(std::string fileName) {
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(fileName,v,inode);
    if(temp != State::SUCCESS) return temp;

    int cnt = (int)v.size();

    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id  == -1 || nextInode.fmode == DENTRY_MODE) return NO_SUCH_FILE;
    
    inode = nextInode;

    int fileSize = inode.fsize;
    char buffer[BLOCK_SIZE];

    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(fileSize == 0) break;
        
        fileSize--;
        fseek(fp, inode.dir_address[i]*BLOCK_SIZE, SEEK_SET);
        //output
        fread(buffer, sizeof(char), BLOCK_SIZE, fp);
        std::cout.write(buffer, BLOCK_SIZE);
    }

    Address address;
    if(fileSize > 0) {
        int offset = 0;
        while(fileSize > 0) {
            fileSize--;
            
            fseek(fp, inode.indir_address[0]*BLOCK_SIZE+offset*sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);

            fseek(fp, address.getblockID()*BLOCK_SIZE, SEEK_SET);
            fread(buffer, sizeof(char), BLOCK_SIZE, fp);
            std::cout.write(buffer, BLOCK_SIZE);

            offset++;
        }
    }
    std::cout << std::endl;

    return SUCCESS;
}

State Filesystem::cp(std::string file1, std::string file2) {
    // read content from file1
    INode inode;
    std::vector<std::string> v;

    State temp = fileNameProcess(file1,v,inode);
    if(temp != State::SUCCESS) return temp;

    int cnt = (int)v.size();

    INode nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id  == -1 || nextInode.fmode == DENTRY_MODE) return NO_SUCH_FILE;
    
    inode = nextInode;

    std::vector<char> content;
    int fileSize = inode.fsize;
    char buffer;

    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(fileSize == 0) break;
        
        fileSize--;
        fseek(fp, inode.dir_address[i]*BLOCK_SIZE, SEEK_SET);
        for(int j = 0; j < BLOCK_SIZE; ++j) {
            
            fread(&buffer, sizeof(char), 1, fp);
            content.push_back(buffer);
        }
    }

    Address address;
    if(fileSize > 0) {
        int offset = 0;
        while(fileSize > 0) {
            fileSize--;
            
            fseek(fp, inode.indir_address[0]*BLOCK_SIZE+offset*sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);

            fseek(fp, address.getblockID()*BLOCK_SIZE, SEEK_SET);
            for(int j = 0; j < BLOCK_SIZE; ++j) {
                fread(&buffer, sizeof(char), 1, fp);
                content.push_back(buffer);
            }

            offset++;
        }
    }

    // deleteFile(file2);
    State state = createFile(file2, inode.fsize);
    if(state != SUCCESS) return state;

    // write content to file2
    v.clear();

    temp = fileNameProcess(file2,v,inode);
    if(temp != State::SUCCESS) return temp;

    cnt = (int)v.size();

    nextInode = findNextInode(inode, v[cnt-1]);
    if(nextInode.id  == -1 || nextInode.fmode == DENTRY_MODE) return NO_SUCH_FILE;

    inode = nextInode;

    fileSize = inode.fsize;
    int index = 0;
    for(int i = 0; i < NUM_DIRECT_ADDRESS; ++i) {
        if(fileSize == 0) break;
        
        fileSize--;

        fseek(fp, inode.dir_address[i]*BLOCK_SIZE, SEEK_SET);
        for(int j = 0; j < BLOCK_SIZE; ++j) fwrite(&content[index++], sizeof(char), 1, fp);
        
    }

    if(fileSize > 0) {
        int offset = 0;
        while(fileSize > 0) {
            fileSize--;
            
            fseek(fp, inode.indir_address[0]*BLOCK_SIZE+offset*sizeof(Address), SEEK_SET);
            fread(&address, sizeof(Address), 1, fp);
            
            fseek(fp, address.getblockID()*BLOCK_SIZE, SEEK_SET);
            for(int j = 0; j < BLOCK_SIZE; ++j) fwrite(&content[index++], sizeof(char), 1, fp);
            
            offset++;
        }
    }

    return SUCCESS;
}
