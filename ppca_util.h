#ifndef PPCA_UTIL_H
#define PPCA_UTIL_H

#include<cstdint>
inline uint32_t zeroExpansion(uint32_t x,int fromDigit){
    for(int i=fromDigit;i<32;i++){
        x-=(x&(1<<i));
    }
    return x;
}
inline int32_t signExpansion(uint32_t x,int fromDigit){
    if(!fromDigit)return 0;
    bool highest=(x&(1<<(fromDigit-1)));
    if(highest){
        x|=(UINT32_MAX<<fromDigit);
    }
    return x;
}


#endif