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
        // �򿪴���
        com = CreateFile(com_name.c_str(),                       // ���ںţ��� COM1
                         GENERIC_READ|GENERIC_WRITE,     // �������д
                         0,                              // ��ռ��ʽ
                         NULL,                           // ���ھ�����ܼ̳�
                         OPEN_EXISTING,                  // �򿪶����Ǵ���
                         FILE_ATTRIBUTE_NORMAL|FILE_FLAG_OVERLAPPED,  // �Ѿ���IO���ں�ִ̨��
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
