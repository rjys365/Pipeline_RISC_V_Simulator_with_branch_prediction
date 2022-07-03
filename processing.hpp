#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include<iostream>
#include<cstdint>
#include<string_view>
#include"register.hpp"
#include"ppca_util.h"
namespace ppca{

enum class CmdType{
    invalid,
    lb,lh,lw,lbu,lhu,
    sb,sh,sw,
    add,addi,
    sub,
    lui,
    auipc,
    XOR,xori,OR,ori,AND,andi,
    sll,slli,srl,srli,sra,srai,
    slt,slti,sltu,sltiu,
    beq,bne,blt,bge,bltu,bgeu,jal,jalr
};

struct Command{
    CmdType type=CmdType::invalid;
    uint32_t rd=0,rs1=0,rs2=0,imm=0;
    int32_t rv1=0,rv2=0;
    Command(){};
    Command(CmdType t,uint32_t d,uint32_t s1,uint32_t s2,uint32_t im,int32_t v1,int32_t v2):type(t),rd(d),rs1(s1),rs2(s2),imm(im),rv1(v1),rv2(v2){}
};
constexpr std::string_view cmdTypeString[]={"invalid","lb","lh","lw","lbu","lhu","sb","sh","sw","add","addi","sub","lui","auipc","XOR","xori","OR","ori","AND","andi","sll","slli","srl","srli","sra","srai","slt","slti","sltu","sltiu","beq","bne","blt","bge","bltu","bgeu","jal","jalr"};
std::ostream &operator<<(std::ostream &out,const Command &cmd){
    out<<"type:\t"<<cmdTypeString[(int)cmd.type]<<std::endl;
    if(cmd.type!=CmdType::invalid)
    out<<"rd:  \t"<<cmd.rd<<std::endl
       <<"rs1:\t"<<cmd.rs1<<std::endl
       <<"rs2:\t"<<cmd.rs2<<std::endl
       ;//<<"imm:\t0x"<<std::hex<<cmd.imm<<std::endl<<std::dec;
    return out;
}


void instructionFetch(const uint32_t &pcval,const Memory &mem,uint32_t &cmd){//I:pc,rom;O:cmd
    cmd=mem.immediateRead(pcval);
}

void instructionDecode(const uint32_t &cmd,Command &dcmd,const Register &reg,const int &loadreg,bool &bubble){
    bubble=false;
    dcmd=Command();
    constexpr uint32_t mask06  =0b0000000'00000'00000'000'00000'1111111u;
    constexpr uint32_t mask711 =0b0000000'00000'00000'000'11111'0000000u;
    constexpr uint32_t mask1214=0b0000000'00000'00000'111'00000'0000000u;
    constexpr uint32_t mask1519=0b0000000'00000'11111'000'00000'0000000u;
    constexpr uint32_t mask2024=0b0000000'11111'00000'000'00000'0000000u;
    constexpr uint32_t mask2531=0b1111111'00000'00000'000'00000'0000000u;
    constexpr uint32_t mask811 =0b0000000'00000'00000'000'11110'0000000u;//only used in SB commands
    constexpr uint32_t mask2530=0b0111111'00000'00000'000'00000'0000000u;//only used in SB commands
    constexpr uint32_t mask2130=0b0111111'11110'00000'000'00000'0000000u;//only used in UJ commands
    constexpr uint32_t mask1231=mask1214|mask1519|mask2024|mask2531,mask2031=mask2024|mask2531,mask1219=mask1214|mask1519;
    uint32_t opcode=cmd&mask06,funct3=((cmd&mask1214)>>12),funct7=((cmd&mask2531)>>25),rd=((cmd&mask711)>>7),rs1=((cmd&mask1519)>>15),rs2=((cmd&mask2024)>>20);
    switch(opcode){
        case 0b0000011u:{//I
            uint32_t imm=((cmd&mask2031)>>20);
            switch(funct3){
                case 0b000u:
                    dcmd=Command(CmdType::lb,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b001u:
                    dcmd=Command(CmdType::lh,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b010u:
                    dcmd=Command(CmdType::lw,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b100u:
                    dcmd=Command(CmdType::lbu,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b101u:
                    dcmd=Command(CmdType::lhu,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                default:
                    break;
            }
            break;
        }
        case 0b0010011u:{//I
            uint32_t imm=((cmd&mask2031)>>20);
            switch(funct3){
                case 0b000u:
                    dcmd=Command(CmdType::addi,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b001u:
                    dcmd=Command(CmdType::slli,rd,rs1,0,rs2,reg.read(rs1),0);
                    break;
                case 0b010u:
                    dcmd=Command(CmdType::slti,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b011u:
                    dcmd=Command(CmdType::sltiu,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b100u:
                    dcmd=Command(CmdType::xori,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b101u:
                    if(funct7)dcmd=Command(CmdType::srai,rd,rs1,0,rs2,reg.read(rs1),0);
                    else dcmd=Command(CmdType::srli,rd,rs1,0,rs2,reg.read(rs1),0);
                    break;
                case 0b110u:
                    dcmd=Command(CmdType::ori,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                case 0b111u:
                    dcmd=Command(CmdType::andi,rd,rs1,0,imm,reg.read(rs1),0);
                    break;
                default:
                    break;
            }
            
            break;
        }
        case 0b0010111u:{//U
            uint32_t imm=(cmd&mask1231);
            dcmd=Command(CmdType::auipc,rd,0,0,imm,0,0);
            break;
        }
        case 0b0100011u:{//S
            uint32_t imm=((cmd&mask711)>>7)|((cmd&mask2531)>>20);
            switch(funct3){
                case 0b000u:
                    dcmd=Command(CmdType::sb,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b001u:
                    dcmd=Command(CmdType::sh,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b010u:
                    dcmd=Command(CmdType::sw,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                default:
                    break;
            }
            break;
        }
        case 0b0110011u:{//R
            switch(funct3){
                case 0b000u:
                    dcmd=Command(funct7?CmdType::sub:CmdType::add,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b001u:
                    dcmd=Command(CmdType::sll,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b010u:
                    dcmd=Command(CmdType::slt,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b011u:
                    dcmd=Command(CmdType::sltu,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b100u:
                    dcmd=Command(CmdType::XOR,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b101u:
                    dcmd=Command(funct7?CmdType::sra:CmdType::srl,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b110u:
                    dcmd=Command(CmdType::OR,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b111u:
                    dcmd=Command(CmdType::AND,rd,rs1,rs2,0,reg.read(rs1),reg.read(rs2));
                    break;
                default:
                    break;
            }
            break;
        }
        case 0b0110111u:{//U
            uint32_t imm=(cmd&mask1231);
            dcmd=Command(CmdType::lui,rd,0,0,imm,0,0);
            break;
        }
        case 0b1100011u:{//SB
            uint32_t imm=((cmd&mask811)>>7)|((cmd&mask2530)>>20)|((cmd&(1u<<7))<<4)|((cmd&(1u<<31))>>19);
            switch(funct3){
                case 0b000u:
                    dcmd=Command(CmdType::beq,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b001u:
                    dcmd=Command(CmdType::bne,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b100u:
                    dcmd=Command(CmdType::blt,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b101u:
                    dcmd=Command(CmdType::bge,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b110u:
                    dcmd=Command(CmdType::bltu,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                case 0b111u:
                    dcmd=Command(CmdType::bgeu,0,rs1,rs2,imm,reg.read(rs1),reg.read(rs2));
                    break;
                default:
                    break;
            }
            break;
        }
        case 0b1100111u:{//I
            uint32_t imm=((cmd&mask2031)>>20);
            dcmd=Command(CmdType::jalr,rd,rs1,0,imm,reg.read(rs1),0);
            break;
        }
        case 0b1101111u:{//UJ
            uint32_t imm=(cmd&mask1219)|((cmd&(1u<<20))>>9)|((cmd&mask2130)>>20)|((cmd&(1u<<31))>>11);
            dcmd=Command(CmdType::jal,rd,0,0,imm,0,0);
            break;
        }
        default:
            break;
        bubble=(loadreg&&(rs1==loadreg||rs2==loadreg));
    }
    
}

struct ExeResult{
    MemoryOperation memOp=MemoryOperation::IDLE;
    uint32_t memPos,memVal;
    bool wb=false;MemoryOperationLen memLen;
    int wbPos;uint32_t wbVal;
};

bool instructionEx(const Command &cmd,const Register &reg,ExeResult &result,const uint32_t &pcval,uint32_t &pcnval,int &loadingreg,bool rs1for,int rs1fval,bool rs2for,int rs2fval,int rs1bval,int rs2bval){
    //constexpr uint32_t maskb=(UINT32_MAX>>32-8),maskh=(UINT32_MAX>>32-16);
    int rs1val=rs1for?rs1fval:rs1bval/*reg.read(cmd.rs1)*/,rs2val=rs2for?rs2fval:rs2bval/*reg.read(cmd.rs2)*/;
    pcnval=pcval+4;
    result=ExeResult();
    if(cmd.type==CmdType::invalid)return false;
    switch(cmd.type){
        case CmdType::lb:
            result.memOp=MemoryOperation::READ;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memLen=MemoryOperationLen::B;
            result.wb=true;
            result.wbPos=cmd.rd;
            break;
        case CmdType::lh:
            result.memOp=MemoryOperation::READ;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memLen=MemoryOperationLen::H;
            result.wb=true;
            result.wbPos=cmd.rd;
            break;
        case CmdType::lw:
            result.memOp=MemoryOperation::READ;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memLen=MemoryOperationLen::W;
            result.wb=true;
            result.wbPos=cmd.rd;
            break;
        case CmdType::lbu://TODO:ATTENEION! Unsigned load
            result.memOp=MemoryOperation::READ;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memLen=MemoryOperationLen::BU;
            result.wb=true;
            result.wbPos=cmd.rd;
            break;
        case CmdType::lhu:
            result.memOp=MemoryOperation::READ;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memLen=MemoryOperationLen::HU;
            result.wb=true;
            result.wbPos=cmd.rd;
            break;
        case CmdType::sb:
            result.memOp=MemoryOperation::WRITE;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memVal=rs2val;
            result.memLen=MemoryOperationLen::B;
            break;
        case CmdType::sh:
            result.memOp=MemoryOperation::WRITE;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memVal=rs2val;
            result.memLen=MemoryOperationLen::H;
            break;
        case CmdType::sw:
            result.memOp=MemoryOperation::WRITE;
            result.memPos=rs1val+signExpansion(cmd.imm,12);
            result.memVal=rs2val;
            result.memLen=MemoryOperationLen::W;
            break;
        case CmdType::add:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val+rs2val;
            break;
        case CmdType::addi:
            if(cmd.rd==10&&cmd.rs1==0&&cmd.imm==255){
                std::cout<<(int)((uint8_t)reg.read(10))<<std::endl;
                return true;
            };
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val+signExpansion(cmd.imm,12);
            break;
        case CmdType::sub:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val-rs2val;
            break;
        case CmdType::lui:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=cmd.imm;
            break;
        case CmdType::auipc:
            
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=pcval+cmd.imm;//TODO: modify when implementing pipeline
            //pc.set(pc.get()+cmd.imm);
            //TODO: discard previous stage
            break;
        case CmdType::XOR:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val^rs2val;
            break;
        case CmdType::xori:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val^signExpansion(cmd.imm,12);
            break;
        case CmdType::OR:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val|rs2val;
            break;
        case CmdType::ori:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val|signExpansion(cmd.imm,12);
            break;
        case CmdType::AND:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val&rs2val;
            break;
        case CmdType::andi:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=rs1val&signExpansion(cmd.imm,12);
            break;
        case CmdType::sll:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=((uint32_t)rs1val<<(uint32_t)rs2val);
            break;
        case CmdType::slli:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=((uint32_t)rs1val<<(uint32_t)cmd.imm);
            break;
        case CmdType::srl:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=((uint32_t)rs1val>>(uint32_t)rs2val);
            break;
        case CmdType::srli:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=((uint32_t)rs1val>>(uint32_t)cmd.imm);
            break;
        case CmdType::sra:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=(rs1val>>rs2val);
            break;
        case CmdType::srai:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=(rs1val>>(int32_t)cmd.imm);
            break;
        case CmdType::slt:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=(rs1val<rs2val)?1:0;
            break;
        case CmdType::slti:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=(rs1val<signExpansion(cmd.imm,12))?1:0;
            break;
        case CmdType::sltu:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=((uint32_t)rs1val<cmd.imm)?1:0;
            break;
        case CmdType::sltiu:
            result.wb=true;
            result.wbPos=cmd.rd;
            result.wbVal=((uint32_t)rs1val<(uint32_t)signExpansion(cmd.imm,12))?1:0;
            break;
        case CmdType::beq://TODO: modify when implementing pipeline!
            if(rs1val==rs2val)pcnval=(pcval+signExpansion(cmd.imm,13));
            break;
        case CmdType::bne:
            if(rs1val!=rs2val)pcnval=(pcval+signExpansion(cmd.imm,13));
            break;
        case CmdType::blt:
            if(rs1val<rs2val)pcnval=(pcval+signExpansion(cmd.imm,13));
            break;
        case CmdType::bge:
            if(rs1val>=rs2val)pcnval=(pcval+signExpansion(cmd.imm,13));
            break;
        case CmdType::bltu:
            if((uint32_t)rs1val<(uint32_t)rs2val)pcnval=(pcval+signExpansion(cmd.imm,13));
            break;
        case CmdType::bgeu:
            if((uint32_t)rs1val>=(uint32_t)rs2val)pcnval=(pcval+signExpansion(cmd.imm,13));
            break;
        case CmdType::jal:
            result.wbVal=pcval+4;
            result.wb=true;
            result.wbPos=cmd.rd;
            pcnval=(pcval+signExpansion(cmd.imm,21));
            break;
        case CmdType::jalr:{
            result.wbVal=pcval+4;
            int res=(rs1val+signExpansion(cmd.imm,12))&(~1u);
            pcnval=(res);
            result.wb=true;
            result.wbPos=cmd.rd;
            break;
        }
        case CmdType::invalid:
            break;
        //default:
        //break;
    }
    loadingreg=(result.wb==true&&result.memOp==MemoryOperation::READ)?cmd.rd:0;
    return false;
}

bool memOp(const ExeResult& result,Memory &mem,ExeResult &out){
    out=result;
    if(result.memOp==MemoryOperation::IDLE)return true;
    mem.setOp(result.memOp,result.memPos,result.memVal,result.memLen);
    if(result.memOp==MemoryOperation::READ&&mem.ready()){
        out.wbVal=mem.read().second;
    }
    return mem.ready();
}

void writeBack(const ExeResult& result,Register &reg){
    if(result.wb)reg.set(result.wbPos,result.wbVal);
}

}

#endif