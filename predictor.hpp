#ifndef PREDICTOR_HPP
#define PREDICTOR_HPP
#include<cstdint>
#include<utility>
#include<stack>
#include<queue>
#include<iostream>
#define ENABLE_PREDICTION
#define ENABLE_RAS
class Predictor{
private:
    enum class TwoBitState{N=0,NN=1,T=2,TT=3};
    static const int PRED_SIZE=65536*8/4;
    TwoBitState cmdStates[PRED_SIZE][16]={(TwoBitState)0};
    uint8_t cmdPt[PRED_SIZE]={0};
    uint8_t jaljalr[PRED_SIZE]={0};//1:jal 2:jalr
    uint32_t topc[PRED_SIZE]={0};
    std::stack<uint32_t>retstk;
    uint32_t retstk_popval=(uint32_t)(-1);
    uint32_t correctCnt=0,totalCnt=0;
    //uint32_t hispc[2]={0};
public:
    std::pair<bool,uint32_t> get(int lastpc){
#ifndef ENABLE_PREDICTION
        return std::make_pair(false,0);
#endif
        lastpc>>=2;
        
        if(jaljalr[lastpc]==2){//jalr
#ifndef ENABLE_RAS
            return std::make_pair(false,0);
#endif
            if(retstk.empty())return std::make_pair(false,0);//unable to predict
            uint32_t ret=retstk.top();
            retstk_popval=ret;
            retstk.pop();
            return std::make_pair(true,ret);
        }
        retstk_popval=(uint32_t)(-1);
        if(jaljalr[lastpc]==1){//jal
            //retstk[]
            return std::make_pair(true,topc[lastpc]);//TODO
        }
        if(cmdStates[lastpc][cmdPt[lastpc]]==TwoBitState::T||cmdStates[lastpc][cmdPt[lastpc]]==TwoBitState::TT){
            return std::make_pair(true,topc[lastpc]);
        }
        return std::make_pair(false,0);

    }
    void feed(uint32_t frompc,uint32_t topc,bool isbn,bool isjal,bool isjalr,bool nolink){//true: jumped
#ifndef ENABLE_PREDICTION
        if(isbn){
            totalCnt++;
            if(topc==frompc+4)correctCnt++;
        }
        return;
#endif
        //if(frompc==0x10a0)
        //    std::cout<<"AAAAAAAAAA!!!!!!!!"<<std::endl;
        if(!(isbn||isjal||isjalr))return;//not a branch, don't update
        uint32_t kfrompc=(frompc>>2);
        //frompc>>=2;
        if(isjal){
            //if(topc==this->topc[frompc])correctCnt++;
            jaljalr[kfrompc]=1;
            this->topc[kfrompc]=topc;
            if(!nolink)retstk.push(frompc+4);
            return;
        }
        if(isjalr){
#ifndef ENABLE_RAS
            return;
#endif
            if(jaljalr[kfrompc]!=2&&!retstk.empty())retstk.pop();
            jaljalr[kfrompc]=2;
            return;
        }
        totalCnt++;
        uint32_t ptopc=((int)cmdStates[kfrompc][cmdPt[kfrompc]]>1)?this->topc[kfrompc]:(frompc+4);
        if(topc==ptopc)correctCnt++;
        else{
            if(retstk_popval!=(uint32_t)-1)retstk.push(retstk_popval);
        }
        bool taken=(topc!=frompc+4);
        if(taken)this->topc[kfrompc]=topc;
        auto &st=cmdStates[kfrompc][cmdPt[kfrompc]];
        switch(st){
            case TwoBitState::N:
                st=taken?TwoBitState::T:TwoBitState::NN;
                break;
            case TwoBitState::NN:
                st=taken?TwoBitState::N:TwoBitState::NN;
                break;
            case TwoBitState::T:
                st=taken?TwoBitState::TT:TwoBitState::N;
                break;
            case TwoBitState::TT:
                st=taken?TwoBitState::TT:TwoBitState::T;
                break;
        }
        cmdPt[kfrompc]=(((cmdPt[kfrompc]<<1)|taken)&15);
    }
    void printCorrectness(){
        std::cout<<""<<(double)correctCnt/(double)totalCnt<<"\t"<<correctCnt<<"\t"<<totalCnt<<std::endl;
    }
};


class PredictPC{
    uint32_t pc[4]={0},pcset=0;
    bool pause=false;
public:
    void set(uint32_t spc){pcset=spc;}
    void set(/*bool branch,*/Predictor &pd){
        auto pred=pd.get(pc[0]);
        if(pred.first){pcset=pred.second;}
        else pcset=pc[0]+4;
    }
    void hold(){pause=true;}
    void step(){
        if(pause){
            pause=false;return;
        }
        for(int i=3;i>0;i--)pc[i]=pc[i-1];
        pc[0]=pcset;
    }
    uint32_t get(int offset){
        return pc[offset];
    }
};

#endif