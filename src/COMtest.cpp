#include<windows.h>
#include<iostream>
#include<fstream>
using namespace std;

class COMcontroler{
    HANDLE com;
    DWORD err;

    void set_com(){

    }
public:
    COMcontroler(){
        com = INVALID_HANDLE_VALUE;
    }
    bool open(string com_name){
        // 打开串口
        com = CreateFile(com_name.c_str(),                       // 串口号，如 COM1
                         GENERIC_READ|GENERIC_WRITE,     // 允许读和写
                         0,                              // 独占方式
                         NULL,                           // 串口句柄不能继承
                         OPEN_EXISTING,                  // 打开而不是创建
                         FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,  // 把具体IO放在后台执行
                         NULL
                         );
        if(com == INVALID_HANDLE_VALUE){
            err = GetLastError();
            return false;
        }
        set_com();
        return true;
    }

};
