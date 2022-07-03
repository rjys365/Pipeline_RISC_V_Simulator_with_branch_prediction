#ifndef PIPELINECOMPUTER_HPP
#define PIPELINECOMPUTER_HPP
#include "processing.hpp"
#include "predictor.hpp"
#include <utility>
namespace ppca{
using std::make_pair;

struct Buffer1{
    std::pair<uint32_t,uint32_t> instruction=make_pair(0,0),ninstruction=make_pair(0,0);
public:
    void set(bool hold,uint32_t x,uint32_t pc){ninstruction=hold?instruction:make_pair(x,pc);}
    void step(){instruction=ninstruction;ninstruction=make_pair(0,0);}
    uint32_t getins(){return instruction.first;}
    uint32_t getpc(){return instruction.second;}
};

struct Buffer2{
    std::pair<Command,uint32_t> cmd,ncmd;
public:
    void set(bool hold,Command c,uint32_t pc){ncmd=hold?cmd:make_pair(c,pc);}
    void step(){cmd=ncmd;ncmd=make_pair(Command(),0);}
    Command getcmd(){return cmd.first;}
    uint32_t getpc(){return cmd.second;}
};

struct Buffer34{
    std::pair<ExeResult,uint32_t> rs,nrs;
    void set(bool hold,ExeResult r,uint32_t pc){nrs=hold?rs:make_pair(r,pc);}
    void step(){rs=nrs;nrs=make_pair(ExeResult(),0);}
public:
    ExeResult getrs(){return rs.first;}
    uint32_t getpc(){return rs.second;}
};

struct FwdInfo{
    int val;bool enabled=false,ready;
};

class PipeLineComputer{
    Buffer1 b1;
    Buffer2 b2;
    Buffer34 b3,b4;
    Predictor pd;
    PredictPC pc;
    Memory mem;
    Register reg;
    uint64_t cycles=0;
    uint64_t jalrTotalCnt=0,jalrCorrectCnt=0;
public:
    PipeLineComputer(string fn=""){
        mem.load(fn);
    }
    void step(){
        pc.step();
        mem.step();
        reg.step();
        b1.step();
        b2.step();
        b3.step();
        b4.step();
    }
    bool runACycle(){//combinatorial logic
        step();
        bool stallBeforeEx,stallBeforeMem,clearBeforeEx;
        bool terminating=false;
        //WB
        auto b4rs=b4.getrs();
        auto b4pc=b4.getpc();
        /*std::cout<<std::hex<<"ID PC="<<b1.getpc()<<",EX PC="<<b2.getpc()<<",MEM PC="<<b3.getpc()<<",wb pc="<<b4pc<<std::dec<<std::endl;
        if(b2.getpc()==0x1180){
            std::cout<<"AAAAAAAA"<<std::endl;
        }*/
        writeBack(b4rs,reg);
        //auto wbfwdpos=b4rs.wb?b4rs.wbPos:0;auto wbfwdval=b4rs.wbVal;
        //MEM
        auto b3rs=b3.getrs();auto b3pc=b3.getpc();
        ExeResult memrs;
        bool memrdy=memOp(b3rs,mem,memrs);
        b4.set(false,memrs,b3pc);//TODO
        //int memfwdpos=memrs.wbPos,memfwdval=memrs.wbVal;
        stallBeforeMem=!memrdy;
        if(stallBeforeMem){
            b4.set(false,ExeResult(),0);
            b3.set(true,ExeResult(),0);
            b2.set(true,Command(),0);
            b1.set(true,0,0);
            pc.hold();
        }
        else{
            //EX
            auto cmdToEx=b2.getcmd();
            auto pcToEx=b2.getpc();
            auto cmdRs1=cmdToEx.rs1,cmdRs2=cmdToEx.rs2;
            uint32_t pcnval;int loadingreg;
            ExeResult exers;
            FwdInfo f1,f2;
            if(b4rs.wb){
                if(cmdRs1&&(uint32_t)b4rs.wbPos==cmdRs1){f1.enabled=true;f1.val=b4rs.wbVal;}
                if(cmdRs2&&b4rs.wbPos==cmdRs2){f2.enabled=true;f2.val=b4rs.wbVal;}
            }
            {
                if(cmdRs1&&(uint32_t)memrs.wbPos==cmdRs1){
                    f1.enabled=true;
                    f1.val=memrs.wbVal;
                }
                if(cmdRs2&&(uint32_t)memrs.wbPos==cmdRs2){
                    f2.enabled=true;
                    f2.val=memrs.wbVal;
                }
            }
            terminating=instructionEx(cmdToEx,reg,exers,pcToEx,pcnval,loadingreg,f1.enabled,f1.val,f2.enabled,f2.val,cmdToEx.rv1,cmdToEx.rv2);
            clearBeforeEx=(cmdToEx.type!=CmdType::invalid&&pcnval!=pc.get(1));
            pd.feed(pcToEx,pcnval,
            cmdToEx.type==CmdType::beq||cmdToEx.type==CmdType::bge||cmdToEx.type==CmdType::bgeu||cmdToEx.type==CmdType::blt||cmdToEx.type==CmdType::bltu||cmdToEx.type==CmdType::bne,
            cmdToEx.type==CmdType::jal,cmdToEx.type==CmdType::jalr,cmdToEx.rd==0);
            b3.set(false,exers,pcToEx);
            /*if(cmdToEx.type==CmdType::jalr&&clearBeforeEx)
                std::cout<<"jalr predict error"<<std::endl;*/
            if(cmdToEx.type==CmdType::jalr&&!clearBeforeEx)jalrCorrectCnt++;
            if(cmdToEx.type==CmdType::jalr)jalrTotalCnt++;
            if(clearBeforeEx){
                pc.set(pcnval);
                //b1,b2 should be cleared during next step() 
            }
            else{
                //ID
                Command dcmd;
                instructionDecode(b1.getins(),dcmd,reg,loadingreg,stallBeforeEx);
                //forward to ID
                if(b4rs.wb){
                    if(dcmd.rs1&&(uint32_t)b4rs.wbPos==dcmd.rs1){dcmd.rv1=b4rs.wbVal;}
                    if(dcmd.rs2&&(uint32_t)b4rs.wbPos==dcmd.rs2){dcmd.rv2=b4rs.wbVal;}
                }
                {
                    if(dcmd.rs1&&(uint32_t)memrs.wbPos==dcmd.rs1){
                        dcmd.rv1=memrs.wbVal;
                    }
                    if(dcmd.rs2&&(uint32_t)memrs.wbPos==dcmd.rs2){
                        dcmd.rv2=memrs.wbVal;
                    }
                }
                if(stallBeforeEx){

                    pc.hold();
                    b1.set(true,0,0);
                    b2.set(false,Command(),0);
                }
                else{
                    b2.set(false,dcmd,b1.getpc());
                    //IF
                    uint32_t fcmd;
                    instructionFetch(pc.get(0),mem,fcmd);
                    b1.set(false,fcmd,pc.get(0));
                    pc.set(pd);
                }
            }
            
        }
        return terminating;
    }
    void run(){
        while(1){
            cycles++;
            if(runACycle())break;
        }
        //std::cout<<cycles<<"\t"<<jalrCorrectCnt<<"\t"<<jalrTotalCnt<<std::endl;
        //pd.printCorrectness();
    }
};





}
#endif