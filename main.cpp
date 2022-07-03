#include"PipelineComputer.hpp"
ppca::PipeLineComputer computer;
//ppca::PipeLineComputer computer("testcases/magic.data");
//ppca::PipeLineComputer computer("array_test1.data");
//ppca::SequentialComputer computer("array_test1.data");
//ppca::SequentialComputer computer("pi.data");
//ppca::SequentialComputer computer("sample.data");
int main(){
    //if(argc!=2)std::cout<<"aa"<<std::endl;
    //std::cout<<argv[1]<<"\t";
    //ppca::PipeLineComputer computer(argv[1]);
    computer.run();
    return 0;
}