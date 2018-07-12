#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <dirent.h>
#include <string>
#include <sstream>
using namespace std;

#define readCharLimit 256

typedef struct Node{
    string name;
    string val;
    struct Node* next;
}instNode;
typedef struct{
    instNode* Nodes;
    unsigned int time;
}inst;

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
        char buf[readCharLimit] = {0};
        int i,n;
        clock_t start,cur;

        if(length > readCharLimit){
            cout<<"out of read char limit"<<endl;
            return "";
        }

        start = clock();
        double timeLimit = 0.05+0.001*length;
        for(i=0,n=0;i<length;i+=n){
            n = read(tty,buf+i,length-i);
            if(-1 == n){
                n = 0;
            }
            cur = clock();
            if((cur-start)/CLOCKS_PER_SEC > timeLimit)
                break;
        }
        if(i>0){
			string s = buf;
			cout<<"read:"<<s;
			/*if(i>3)
				cout<<"i="<<i<<","<<(int)buf[i-3]<<','<<(int)buf[i-2]<<','<<(int)buf[i-1]<<','<<(int)buf[i]<<endl;*/
        	return s;
		}
		cout<<"read:";
		return "";	
    }
    bool writeTTY(string s){
	cout<<"write:"<<s;
        if(s.length() != write(tty,s.c_str(),s.length())){
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

    bool endsWith(string s,string endsubstr){
        int slen = s.length();
        int endlen = endsubstr.length();
        if(slen>=endlen && s.substr(slen-endlen,string::npos)==endsubstr)
            return true;
        return false;
    }
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
        strftime(c,14,"%Y%m%d%H%M",t);
        string msg = "#Veri+";
        msg += c;
		//msg += " \r\n";
        TC->writeTTY(msg+" \r\n");

        msg = TC->readTTY(80);
        if(msg.substr(0,11)=="#Veri+20+OK")
            return true;
        return false;
    }
    bool singleMove(string msg){
        TC->writeTTY(msg+"\r\n");
		stringstream stream(msg.substr(msg.find("T")+1,string::npos));
		double t;
		stream>>t;
        sleep(t/1000);
		msg = TC->readTTY(readCharLimit);
        return endsWith(msg,"#CC\n\n");
    }
    bool stop(){
        TC->writeTTY("#STOP\r\n");
		sleep(0.5);
        string msg = TC->readTTY(256);
                                        // 待测
        return endsWith(msg,"\n\n");
    }
    bool storeFile(){
        return false;
    }
    bool listFile(){
        TC->writeTTY("#Flist\r\n");
        string msg = TC->readTTY(readCharLimit);
		while(!endsWith(msg,"\n\n")){
			//cout<<(int)msg[msg.length()-1]<<endl;
			msg += TC->readTTY(readCharLimit);
		}
        if(endsWith(msg,"\n\n")){
            // parser
            while(msg.length()>10){   // "Name:"+"Size:"总长
                string::size_type namePos = msg.find("Name:");
                string::size_type sizePos = msg.find("Size:");
                if(string::npos != namePos && string::npos != sizePos){
                    string filename = msg.substr(namePos+5,sizePos-7-namePos);
                    msg = msg.substr(sizePos+5,string::npos);
                    int filesize;
                    stringstream stream(msg);
                    stream>>filesize;
                    //如何送出filename和filesize信息
                }
                else
                    break;
            }
            return true;
        }
        return false;
    }
    bool deleteFile(string filename){
        TC->writeTTY("#FDel-"+filename+"\r\n");
        string msg = TC->readTTY(10);
        return endsWith(msg,"OK\n\n");
    }
    bool execFile(){
        return false;
    }
    bool fetchFile(string filename){
        TC->writeTTY("#FRead-"+filename+"\r\n");
        string msg = TC->readTTY(readCharLimit);
        if(!endsWith(msg,"End\n\n"))
            msg += TC->readTTY(readCharLimit);  // 可能文件比较长，再给一次机会
        if(endsWith(msg,"End\n\n")){
            string::size_type head = msg.find("Start")+7;
            msg = msg.substr(head,msg.find("End")-head-1);
            while(msg.length()>2){
                string::size_type interval = msg.find("\r\n");
                string singleInst = msg.substr(0,interval);
                msg = msg.substr(interval+2,string::npos);
                cout<<singleInst<<endl;
                //如何把singleInst发出去
            }
            return true;
        }
        return false;
    }
    bool renameFile(string oldName,string newName){
        TC->writeTTY("#FRName-"+oldName+'-'+newName+"\r\n");
        string msg = TC->readTTY(17);
        return endsWith(msg,"OK\n\n");
    }
    bool enable(string msg){
        TC->writeTTY("#Enable"+msg+"\r\n");
        msg = TC->readTTY(20);
        return endsWith(msg,"OK...\n\n");
    }
    bool disable(){
        TC->writeTTY("Disable\r\n");
        string msg = TC->readTTY(16);
        return (msg == "#Disable+OK...\r\n");
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
		SC.singleMove("#2P1050T100");
		SC.listFile();
		//SC.stop();
		if(SC.close())
			cout<<"close succeed"<<endl;
		else
			cout<<"close failed"<<endl;
    }
    /*if(TC.openTTY()){
        time_t curTime = time(0);
        struct tm *t = localtime(&curTime);
        char c[20] = {0};
        strftime(c,20,"%Y%m%d%H%M",t);
        string msg = "#Veri+";
        msg += c;
        TC.writeTTY(msg+" \r\n");

        receive = TC.readTTY(26);
        cout<<"receive:"<<receive;
        TC.closeTTY();
    }*/
}
