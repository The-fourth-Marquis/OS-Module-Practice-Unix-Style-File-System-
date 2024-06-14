#include "Components.h"
#include <string.h>

Address::Address(/* args */)
{
	memset(Address_info, 0, 3 * sizeof(*Address_info));
}

Address::~Address()
{
}

int Address::addressToint(){
    int size = sizeof(unsigned char) * 8;
    return int((((Address_info[0]<<size)|Address_info[1])<<size)|Address_info[2]);
}


int Address::getblockID(){
	return int((this->addressToint())>>BLOCKOFFSET_SIZE);
}

int Address::getOffset(){
	
	int all=this->addressToint();
	return int(all^((all>>BLOCKOFFSET_SIZE)<<BLOCKOFFSET_SIZE));
	
}

void Address::intToaddress(int it, int pid, unsigned char (&tempid)[2]) {
    int x[16] = {0};
    int temp = it;

    for(int i = 0; temp != 0; i++) {
        if(temp % 2 == 1) {
            x[i + pid] = 1;    
        } else {
            x[i + pid] = 0;            
        }
        temp = temp / 2;
    }

    for(int i = 0; i < 8; i++) {
        tempid[0] = (tempid[0] << 1) | x[15 - i];
    }
    
    for(int i = 0; i < 8; i++) {
        tempid[1] = (tempid[1] << 1) | x[7 - i];
    }
}

void Address::setblockID(int id){
	unsigned char tempid[2] = {0};
	intToaddress(id, 2, tempid);
	unsigned char AND_ITEM2=0b11;
	Address_info[0]=tempid[0];
	Address_info[1]=tempid[1]|(Address_info[1]&AND_ITEM2);
}

void Address::setOffset(int offset){
	unsigned char tempid[2] = {0};
	intToaddress(offset, 0, tempid);
	unsigned char AND_ITEM6=0b11111100;
	Address_info[2]=tempid[1];
	Address_info[1]=tempid[0]|(Address_info[1]&AND_ITEM6);
}