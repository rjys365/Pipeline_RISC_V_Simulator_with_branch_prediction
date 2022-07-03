#include"PipelineComputer.hpp"
ppca::PipeLineComputer computer;
//ppca::PipeLineComputer computer("testcases/qsort.data");
//ppca::PipeLineComputer computer("array_test1.data");
//ppca::SequentialComputer computer("array_test1.data");
//ppca::SequentialComputer computer("pi.data");
//ppca::SequentialComputer computer("sample.data");
int main(){
    computer.run();
    return 0;
}