/* ************************************************************************
> File Name:     main.cpp
> Author:        dblinux
> Created Time:  Fri Jun  2 14:04:41 2021
> Description:   Test the interfaces of skipList    (.hpp)
 ************************************************************************/
#include <iostream>
#include "skipList.hpp"
#define FILE_PATH "./store/dumpFile"

int main() {

    skipList<std::string, std::string> skipList(6);
	skipList.insert_element("1", "cpp"); 
	skipList.insert_element("3", "algorithm"); 
	skipList.insert_element("7", "data structure"); 
	skipList.insert_element("8", "network"); 
	skipList.insert_element("9", "system"); 
	skipList.insert_element("19", "database"); 
	skipList.insert_element("19", "compisition of computer"); 

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.dump_file();

    skipList.search_element("9");
    skipList.search_element("18");


    skipList.display_list();

    skipList.delete_element("3");
    skipList.delete_element("7");

    std::cout << "skipList size:" << skipList.size() << std::endl;

    skipList.display_list();
}
