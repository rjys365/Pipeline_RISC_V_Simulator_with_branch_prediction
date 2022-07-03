#ifndef SEQUENTIALCOMPUTER_HPP
#define SEQUENTIALCOMPUTER_HPP

#include<string>
#include<iostream>
#include "processing.hpp"
#include "register.hpp"
namespace ppca{

class SequentialComputer{
    Register reg;
    Memory mem;
    PCounter pc;
    enum class seqStage{IF,ID,EX,MEM,WB}stage=seqStage::IF;
    uint32_t IFIDBuf;//TODO: change to register-like objects
    Command IDEXBuf;
    ExeResult EXMEMBuf;
    ExeResult MEMWBBuf;
    //uint8_t ret=0;
    void seqStep(){//step for sequential units
        reg.step();
        mem.step();
        pc.step();
    }
    bool runACycle(){//true means end
        seqStep();
        pc.setInc(stage==seqStage::WB);
        /*if(stage==seqStage::EX){
            std::cout<<"pc=0x"<<std::hex<<pc.get()<<std::endl<<std::dec<<IDEXBuf
                     ;//<<"Before:\tx[rd]="<<reg[IDEXBuf.rd]<<", x[rs1]="<<reg[IDEXBuf.rs1]<<", x[rs2]="<<reg[IDEXBuf.rs2]<<std::endl;
        }
        if(stage==seqStage::IF){
            std::cout<<"After:\tx[rd]="<<reg[IDEXBuf.rd]<<", x[rs1]="<<reg[IDEXBuf.rs1]<<", x[rs2]="<<reg[IDEXBuf.rs2]<<std::endl;
        }*/
        switch(stage){
            case seqStage::IF:
                instructionFetch(pc,mem,IFIDBuf);
                stage=seqStage::ID;
                break;
            case seqStage::ID:
                instructionDecode(IFIDBuf,IDEXBuf);
                stage=seqStage::EX;
                break;
            case seqStage::EX:
                if(instructionEx(IDEXBuf,reg,EXMEMBuf,pc))return true;
                stage=seqStage::MEM;
                break;
            case seqStage::MEM:
                if(memOp(EXMEMBuf,mem,MEMWBBuf))stage=seqStage::WB;
                break;
            case seqStage::WB:
                writeBack(MEMWBBuf,reg);
                stage=seqStage::IF;
                break;
        }
        return false;
    }
public:
    SequentialComputer(std::string fn=""){
        mem.load(fn);
    }
    void run(){
        while(!runACycle());
    }
};

}
#endif