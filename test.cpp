#include <sys/fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <bits/stdc++.h>
#include "src/util/utils_declaration.h"

#define POOLSIZE 32

std::unordered_map<std::string,std::string> mp;

int main(int argc,char *argv[])
{
    getOption(argc,argv,&mp);
    std::cout<<mp["port"]<<std::endl;
}