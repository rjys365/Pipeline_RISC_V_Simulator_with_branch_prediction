//header of Memory, Register and Program Counter

#ifndef REGISTER_HPP
#define REGISTER_HPP
#include<utility>
#include<fstream>
#include<iostream>
#include<cstdint>
#include"ppca_util.h"

namespace ppca{
using std::string;
using std::ifstream;


class Register{
    static constexpr int SIZE=32;
    private:
        int data[SIZE]={0};
        int ndata[SIZE]={0};
        bool needupd[SIZE]={0};
    public:
        int read(int pos)const{
            if(pos==0)return 0;
            return data[pos];
        }
        int operator[] (int pos)const{return read(pos);}
        void set(int pos,int val){
            if(pos==0||pos>=SIZE)return;
            needupd[pos]=true;
            ndata[pos]=val;
        }
        void step(){//called at the rising edge of the clock
            for(int i=1;i<SIZE;i++){
                if(needupd[i]){needupd[i]=false;data[i]=ndata[i];}
            }
        }
};


enum class MemoryOperation{IDLE,READ,WRITE};
enum class MemoryOperationLen{B=1,H=2,W=4,BU=9,HU=10};
class Memory{
    private:
        static constexpr int SIZE=65536*8;//unit:byte
        uint8_t data[SIZE]={0};
        MemoryOperation currentOp=MemoryOperation::IDLE;
        int opPos;uint32_t opVal;MemoryOperationLen opLen;
        int waitCycle=0;
        
    public:
        void setOp(MemoryOperation op,int pos,uint32_t val,MemoryOperationLen len){
            if(currentOp!=op||opPos!=pos||opLen!=len)waitCycle=3;
            currentOp=op;opPos=pos;opVal=val;opLen=len;
        }
        void immediateWrite(uint32_t pos,uint32_t value){
            data[pos]=(uint8_t)value;
            data[pos+1]=(uint8_t)(value>>8);
            data[pos+2]=(uint8_t)(value>>16);
            data[pos+3]=(uint8_t)(value>>24);
        }
        void immediateWrite(uint32_t pos,uint32_t value,MemoryOperationLen len){
            if((int)len>7)len=(MemoryOperationLen)((int)len&7);
            switch(len){
                case MemoryOperationLen::W:
                    data[pos+3]=(uint8_t)(value>>24);
                    data[pos+2]=(uint8_t)(value>>16);
                case MemoryOperationLen::H:
                    data[pos+1]=(uint8_t)(value>>8);
                case MemoryOperationLen::B:
                    data[pos]=(uint8_t)value;
                default:
                    break;
            }
        }
        uint32_t immediateRead(uint32_t pos)const{
            return ((uint32_t)data[pos]+((uint32_t)data[pos+1]<<8)+((uint32_t)data[pos+2]<<16)+((uint32_t)data[pos+3]<<24));
        }
        uint32_t immediateRead(uint32_t pos,MemoryOperationLen len){
            if(len==MemoryOperationLen::HU)return data[pos]+((uint32_t)data[pos+1]<<8);
            if(len==MemoryOperationLen::BU)return data[pos];
            if(len==MemoryOperationLen::W)return immediateRead(pos);
            if(len==MemoryOperationLen::H)return signExpansion(data[pos]+((uint32_t)data[pos+1]<<8),16);
            return signExpansion(data[pos],8);//B
        }
        void load(string filename){
            uint32_t pos=0;//unit:byte
            ifstream nfile(filename);
            std::istream &file=(filename.empty())?std::cin:nfile;
            string line;
            while(std::getline(file,line)){
                if(line[0]=='@'){
                    string posStr=line.substr(1);
                    pos=std::stoi(posStr,0,16);
                }
                else{
                    const auto len=line.length();
                    for(std::size_t i=0;i<len;){
                        if(!isxdigit(line[i])){i++;continue;}
                        string numstr=line.substr(i,2);
                        data[pos++]=std::stoi(numstr,0,16);
                        i+=2;
                    }
                }
            }
        }
        void dump(uint32_t siz){//for debugging purposes
            for(uint32_t i=0;i<siz;i+=4){
                std::cout<<std::hex<<immediateRead(i)<<std::endl;
            }
        }
        std::pair<bool,uint32_t>read(){
            if(currentOp!=MemoryOperation::READ||waitCycle>0)return std::make_pair(false,0);
            return std::make_pair(true,immediateRead(opPos,opLen));
        }
        bool writeOK(){
            return currentOp==MemoryOperation::WRITE&&waitCycle<=0;
        }
        bool ready(){
            return currentOp==MemoryOperation::IDLE||waitCycle<=0;
        }
        void step(){//called at the rising edge of the clock
            if(currentOp!=MemoryOperation::IDLE&&waitCycle>0)--waitCycle;
            else{
                if(currentOp==MemoryOperation::WRITE)immediateWrite(opPos,opVal,opLen);
                waitCycle=-1;
            }
        }
};

class PCounter{
private:
    uint32_t cnt=0,toset=0;
    bool setting=true,increasing=true;//increasing: used in sequential computer
public:
    uint32_t get() const{
        return cnt;
    }
    void set(uint32_t x){
        setting=true;
        toset=x;
    }
    void setInc(bool x){
        increasing=x;
    }
    void step(){
        if(!increasing)return;
        if(setting){
            cnt=toset;
            setting=false;
            return;
        }
        cnt+=4;
    }
};


}


#endif