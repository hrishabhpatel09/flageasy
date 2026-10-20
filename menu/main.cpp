#include<iostream>
#include<flageasy/flageasy.hpp>
#include<format>
#include<string>


int main(int argc, char const *argv[])
{
    flageasy::Parser cli("app","A cli based code editer like Vim");
    cli.add_switch("verbose",'v',"Enables Verbose Output");
    cli.add_switch("extended_regex",'E',"Enable extended regex notation");
    cli.add_int("count",'c',"Prints lines After the match");

    flageasy::ParseResult r = cli.parse(argc, argv);

    int count = r.as<int>("count");
    bool isVerbose = r.as<bool>("verbose");
    bool extended = r.as<bool>("extended_regex");

    if(isVerbose && extended){
        std::cout<<"Verbose is Enabled"<<std::endl;
    }

    std::string s = std::format("grep -C {}  -{} 'flageasy' ./README.md ",count,extended?"E":"");
    const char* command = s.c_str();
    FILE* fptr = popen(command,"r");

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fptr) != nullptr) {
        std::cout << buffer;
    }

    pclose(fptr);
    return 0;
}
