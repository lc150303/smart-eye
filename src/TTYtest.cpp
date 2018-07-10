#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <string>
using namespace std;

class TTYcontroller{
    int tty;
    string _path;
    bool _is_open;

    bool setTTY(){
        struct termios ttyoptions;
        tcgetattr(tty,&ttyoptions);

        ttyoptions.c_cflag |= B9600 | CLOCAL | CREAD;  //  波特率设置，本地连接，接收使能
        ttyoptions.c_cflag &= ~CSIZE;   // 清空数据位
        ttyoptions.c_cflag |= CS8;      // 设置数据位为8
        ttyoptions.c_cflag &= ~CSTOPB;  // 1位停止位
        ttyoptions.c_cflag &= ~PARENB;  // 无奇偶校验

        tcflush(tty,TCIOFLUSH);
        if(tcsetattr(tty,TCSANOW,&ttyoptions)!=0)
            return false;

        return true;
    }
public:
    TTYcontroller(){
        tty = -1;
        _path = "";
        _is_open = false;
    }
    bool openTTY(string tty_name){
        // 打开串口
        _path = "/dev/"+tty_name;
        tty = open(_path.c_str(),                // 串口号，如 ttyS0
                    O_RDWR|O_NOCTTY|O_NDELAY);      // 允许读和写
        if(-1 == tty){
            return false;
        }
        _is_open = true;
        return setTTY();
    }
    bool openTTY(){
	DIR *dir = opendir("/dev");
	struct dirent *ent;
        while(ent = readdir(dir), NULL != ent){
            if("ttyUSB" == string(ent->d_name).substr(0,6)){
	    		string name = string(ent->d_name);
				cout<<"trying:"<<name<<endl;
	    		if(!openTTY(name)){
					closeTTY();
					continue;
				}
				cout<<name<<" opened"<<endl;
				return true;
			}
		}
	return false;
    }
    string readTTY(int length){
        char buf[256] = {0};
        int i,n;
        clock_t start,cur;

        if(length > 256){
            cout<<"out of read limit"<<endl;
            return "";
        }

        start = clock();
        double limit = 0.05+0.001*length;
        for(i=0,n=0;i<length;i+=n){
            n = read(tty,buf+i,length-i);
            if(-1 == n){
                n = 0;
            }
            cur = clock();
            if((cur-start)/CLOCKS_PER_SEC > limit)
                break;
        }
        string s = buf;
	cout<<"read:"<<s;
        return s;
    }
    bool writeTTY(string s){
	cout<<"write:"<<s;
        if(-1 == write(tty,s.c_str(),s.length())){
            cout<<"write failed"<<endl;
            return false;
        }
        return true;
    }
    bool closeTTY(){
        if(!_is_open)
            return true;
        if(0 != close(tty)){
            cout<<"CloseHandle failed"<<endl;
            return false;
        }
        tty = -1;
        _is_open = false;
        return true;
    }
    void listTTY(){
        cout<<"Valid ttys: ";
		DIR *dir = opendir("/dev");
		struct dirent *ent;
        while(ent = readdir(dir), NULL != ent){
            if("ttyUSB" == string(ent->d_name).substr(0,6)){
				cout<<(string)ent->d_name<<endl;
				string name = "/dev/"+string(ent->d_name);
				int testTTY = open(name.c_str(),O_RDWR|O_NOCTTY|O_NDELAY);
				if(testTTY == -1)
					continue;
				cout<<name<<", "<<endl;
				close(testTTY);
			}
        }
        cout<<endl;
    }
    bool isOpen(){
        return _is_open;
    }

};

class ServoController{
    TTYcontroller *TC;
public:
    ServoController(){
        TC = new TTYcontroller();
        TC->openTTY();
    }
    ServoController(string tty_name){
        TC = new TTYcontroller();
        TC->openTTY(tty_name);
    }
    bool verify(){
        if(!TC->isOpen()){
            cout<<"tty opening failed"<<endl;
            return false;
        }
        time_t curTime = time(0);
        struct tm *t = localtime(&curTime);
        char c[20] = {0};
        strftime(c,20,"%Y%m%d%H%M%S",t);
        string msg = "#Veri+";
        msg += c;
        TC->writeTTY(msg+" \r\n");

        msg = TC->readTTY(26);
        if(msg.substr(0,11)=="#Veri+20+OK")
            return true;
        return false;
    }
    bool singleMove(){
	TC->writeTTY("#1P200#5P1300T1000\r\n");
	sleep(1);
	TC->readTTY(5);
        return false;
    }
    bool stop(){
        TC->writeTTY("#STOP\r\n");
		sleep(0.5);
        string msg = TC->readTTY(50);
        //if(msg == "")                // 待测
        return true;
        return false;
    }
    bool storeFile(){
        return false;
    }
    bool listFile(){
        TC->writeTTY("#Flist\r\n");
        string msg = TC->readTTY(256);
        cout<<"files:"<<msg;

        if(2<msg.length() && msg.substr(msg.length()-2,msg.length()) == "\r\n")
            return true; 
        
        return false;
    }
    bool deleteFile(){
        return false;
    }
    bool execFile(){
        return false;
    }
    bool fetchFile(){
	
        return false;
    }
    bool renameFile(){
        return false;
    }
    bool enable(){
        return false;
    }
    bool disable(){
        TC->writeTTY("Disable\r\n");
        string msg = TC->readTTY(16);
        if(msg == "#Disable+OK...\r\n")
            return true;
        return false;
    }
    bool format(){
        TC->writeTTY("#Format+Start\r\n");
        string msg1 = TC->readTTY(16),msg2 = "";
        while(msg2 == ""){
            msg2 = TC->readTTY(12);
        }
        if(msg2.length()<12)
            msg2 += TC->readTTY(12-msg2.length());
        if(msg1 == "#Format+Start\r\n" && msg2 == "#Format+OK\r\n")
            return true;
        return false;
    }
    bool close(){
	return TC->closeTTY();
    }
};

int main(){
    ServoController SC;
    string receive;
    if(SC.verify()){
		SC.singleMove();
		SC.stop();
		SC.listFile();
		if(SC.close())
			cout<<"close succeed"<<endl;
		else
			cout<<"close failed"<<endl;
    }
    /*if(TC.openTTY()){
        time_t curTime = time(0);
        struct tm *t = localtime(&curTime);
        char c[20] = {0};
        strftime(c,20,"%Y%m%d%H%M%S",t);
        string msg = "#Veri+";
        msg += c;
        TC.writeTTY(msg+" \r\n");

        receive = TC.readTTY(26);
        cout<<"receive:"<<receive;
        TC.closeTTY();
    }*/
}
