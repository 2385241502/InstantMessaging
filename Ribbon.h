#include<iostream>
#include<locale>
#include<string>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<cstring>
#include"../_ooci.h"
#include<memory>
#include <codecvt>
#define MAX_MESSAGE 2048
using namespace std;
struct message{
	char flag;
	string fro,to;
	string buffer;
};
struct user{
	string acount,name;
};
string _itoa(int);
bool Recv(int clientfd,message &mess,int &ti); //收数据
bool Login(int clientfd,user &lic,message mess);		//登录
bool Send(int clientfd,message mess,int &ti);//发
bool Indatabase(message mess);
message Register(message mess);
string UnicodetoUTF8(const wstring wtr); //Unicode  至 utf8
wstring UTF8toUnicode(const string str);
string UnicodetoANSI(const wstring wtr);
wstring ANSItoUnicode(const string str);

string toString(const char *ch);
void Pakage(char*buffer,message mess);
void o_Pakage(const char *buffer,message &mess);

string _itoa(int x){
	if(x==0)return "0";
	string st;
	while(x){
		st=(char)(x%10+'0')+st;
		x/=10;
	}
	return st;
}
message Register(message mess){
	connection conn1,conn2;
	message mess1;
	mess1.fro="000000";
	mess1.to=mess.fro;
	mess1.flag='1';

	if (conn1.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){
//  	if (conn1.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){	
		mess1.buffer="1数据库连接异常...";	
		return mess1;	
	}
	if(conn2.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){	
		mess1.buffer="数据库连接异常...";
		return mess1;	
	}
	
	sqlstatement stmt1(&conn1),stmt2(&conn2);
	string sql1="insert into t_users(acount,name,password) values (:1,:2,:3)";
	string sql2="create table user_"+mess.fro+" (time date,fro char(6),data varchar2("+_itoa(MAX_MESSAGE)+"))";
	stmt1.prepare(sql1.c_str());
	char fro[10],to[15],txt[25];
	strcpy(fro,mess.fro.c_str());strcpy(to,mess.to.c_str());strcpy(txt,mess.buffer.c_str());
	stmt1.bindin(1,fro,10);stmt1.bindin(2,txt,25);stmt1.bindin(3,to,15);
	
	if(stmt1.execute()!=0){
		mess1.flag='4';
		mess1.buffer="用户已存在";
		return mess1;
	}
	conn1.commit();
	
	stmt2.prepare(sql2.c_str());
	if(stmt2.execute()!=0){
		mess1.flag='4';
		mess1.buffer="用户创建失败";
		return mess1;
	}
	mess1.flag='5';
	mess1.buffer="注册成功";

	return mess1;
	
}
bool Indatabase(message mess){
	connection conn;
	if(conn.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){
		cout<<"Indatabase:数据库连接异常"<<endl;
		return false;
	}
        string sql="insert into user_"+mess.to+"(time,fro,data) values(sysdate,'"+mess.fro+"','"+mess.buffer+"')";
	sqlstatement stmt(&conn);	
        stmt.prepare(sql.c_str());

        if(stmt.execute()!=0){
                cout<<"数据库异常"<<endl;
                return false;
        }
        conn.commit();
	return true;
}
void o_Pakage(const char *buffer,message &mess){
	string st=buffer;
	mess.flag=st[12];
	mess.fro="";
	mess.to="";
	mess.buffer="";
	int i=st.find("<o--to--o>");
	for(int tmp=25;tmp<i;tmp++)
		mess.fro=mess.fro+st[tmp];	
	int j=st.find("<o--txt--o>");
	for(int tmp=i+10;tmp<j;tmp++)
		mess.to=mess.to+st[tmp];
	for(size_t tmp=j+11;tmp<st.size();tmp++)
		mess.buffer=mess.buffer+st[tmp];
}
	
	
void Pakage(char*buffer,message mess){
	string tmp="<o--flag--o>";
	tmp=tmp+mess.flag;
	tmp+="<o--from--o>"+mess.fro;
	tmp+="<o--to--o>"+mess.to;
	tmp+="<o--txt--o>"+mess.buffer;	
	strcpy(buffer,tmp.c_str());
	
}
		
string toString(const char *ch){
	string tmp;
	for(int i=0;i<(int)strlen(ch);i++)tmp=tmp+ch[i];
	return tmp;
}
bool Recv(int clientfd,message &mess,int &ti){
	ti=0;
	char buffer[MAX_MESSAGE];	
	int ret=0,rett=0;
	int sum;
	int rettt=4;
	while(rettt>0){
		if((ret=recv(clientfd,&sum+rett,rettt,0))<=0){
			cout<<"接收包头异常"<<endl;
			return false;
		}
		rett+=ret;	
		rettt-=ret;	
	}
	sum=ntohl(sum);
	int len=0;
	while(sum>0){
		if((ret=recv(clientfd,buffer+len,sum,0))<=0){
			cout<<"接收包错误"<<endl;
			return false;
		}
		sum-=ret;
		len+=ret;	
	}
	buffer[len]=0x00;
	string tmp=toString(buffer);
 
	wstring tmp_w=UTF8toUnicode(tmp);
	tmp = UnicodetoANSI(tmp_w);
	strcpy(buffer,tmp.c_str());

	o_Pakage(buffer,mess);
	
	return true;
}

bool Send(int clientfd,message mess,int& ti){
	char buffer[MAX_MESSAGE];
	ti=0;
	memset(buffer,0,sizeof(buffer));

	Pakage(buffer,mess);
	string tmp = toString(buffer);
	tmp=UnicodetoUTF8(ANSItoUnicode(tmp));
	int len=tmp.size();
	int lenn=htonl(len);
	memcpy(buffer,&lenn,4);
	strcpy(buffer+4,tmp.c_str());
	int ret;
	int sen=0;
	len+=4;
	while(len){
		if((ret=send(clientfd+sen,buffer+sen,len,0))<=0){
			cout<<"发送异常"<<endl;
			return false;
		}
		len-=ret;
		sen+=ret;
	}
	return true;

}


bool Login(int clientfd,user &lic,message mess){	
	lic.acount=mess.fro;
	connection conn;
	if(conn.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){
		cout<<"login:数据库连接异常"<<endl;
		return false;
	}
	
	sqlstatement stmt(&conn);

	string sql="select name from t_users where acount='"+mess.fro+"' and password='"+mess.to+"'";
	char na[31];
	stmt.prepare(sql.c_str());
	stmt.bindout(1,na,30);
	if(stmt.execute()!=0){
		return false;
	}
	if(stmt.next()==1403){

		return false;
	}
	lic.acount=mess.fro;
	lic.name=toString(na);
	conn.disconnect();
	return true;	
}

wstring ANSItoUnicode(const string str){
	wstring ret;
	mbstate_t state={};
	const char *src=str.data();
	size_t len=mbsrtowcs(nullptr,&src,0,&state);	
		unique_ptr<wchar_t[]>buff(new wchar_t[len+1]);
		len=mbsrtowcs(buff.get(),&src,len,&state);
	
			ret.assign(buff.get(),len);

	return ret;
}

string UnicodetoUTF8(const wstring wstr) { 
	string ret;    
	try { 
		wstring_convert< codecvt_utf8<wchar_t> > wcv;        
		ret = wcv.to_bytes(wstr); 
	} 
	catch (const exception& e) { 
		std::cerr << e.what() << std::endl; 
	}    
	return ret; 
}

wstring UTF8toUnicode(const string str) { 
	wstring ret;    
	try { 
		wstring_convert< codecvt_utf8<wchar_t> > wcv;        
		ret = wcv.from_bytes(str); 
	} 
	catch (const exception& e) { 
		cerr << e.what() << endl; 
	}    
	return ret; 
}

string UnicodetoANSI(const wstring wstr) { 
	string ret;    
	mbstate_t state = {};    
	setlocale(LC_CTYPE, "");
	const wchar_t* src = wstr.data();    
	size_t len = wcsrtombs(nullptr, &src, 0, &state);    	
		unique_ptr< char[] > buff(new char[len + 1]);        
		len = wcsrtombs(buff.get(), &src, len, &state);        
	
			ret.assign(buff.get(), len); 

	return ret; 
}
