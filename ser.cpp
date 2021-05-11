#include<pthread.h>
#include<cstdio>
#include<signal.h>
#include"Ribbon.h"
#include<queue>
using namespace std;

user lic;
int ti;
void *main_time(void *arg){
	ti=0;
	pthread_t pid=(long)arg;
	while(1){
		sleep(1);
		ti++;
		if(ti>=20){
			cout<<"连接超时"<<endl;
			pthread_kill(pid,2);
			pthread_exit(0);
		}
	}
}
void *main_read(void *arg){
	int clientfd =(long)arg;
	message mess;
	while(1){
		if(Recv(clientfd,mess,ti)==false){
			break;
		}
		if(mess.flag=='1')Indatabase(mess);
	
	}	
	pthread_exit(0);
}

void *main_print(void *arg){
	int clientfd=(long)arg;
	pthread_detach(pthread_self());
	message mess;
	struct Result{
		string ti,fro,na,buf;
	};
	while(1){
		queue<Result>result;
		connection conn2,conn1;
		if(conn1.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){
			cout<<"print:数据库连接异常..."<<endl;
			break;
		}

		sqlstatement stmt1(&conn1);

		string sql1="select to_char(time,'yyyy-mm-dd,hh24:mi:ss'),fro,data,name from user_"+lic.acount+" left join t_users on fro=acount";
		char tim[30];
		char fro[10];
		char buf[2049];
		char na[23];

		stmt1.prepare(sql1.c_str());

		stmt1.bindout(1,tim,29);
		stmt1.bindout(2,fro,9);
		stmt1.bindout(3,buf,2048);
		stmt1.bindout(4,na,22);
		if(stmt1.execute()!=0){
			cout<<"数据库异常..."<<endl;
		}
		while(1){
			if(stmt1.next()!=0){
				break;				
			}
			Result tmp;
			tmp.ti=tim;
			tmp.fro=fro;
			tmp.na=na;
			tmp.buf=buf;
			result.push(tmp);
		}
	
		conn1.disconnect();
	
		if(conn2.connecttodb("scott/root@snorcl11g_198","Simplified Chinese_China.ZHS16GBK")!=0){
			cout<<"连接失败..."<<endl;
			break;
		}
		int fl=0;
		sqlstatement stmt2(&conn2);
		queue<Result> res=result;
		while(!result.empty()){
			string sql2="delete from user_"+lic.acount+" where time =to_date('"+result.front().ti+"','yyyy-mm-dd,hh24:mi:ss')";
			stmt2.prepare(sql2.c_str());
			if(stmt2.execute()!=0){
				cout<<"执行异常..."<<endl;
			}	
			result.pop();
		}	
		if(fl==0)conn2.commit();
		while(!res.empty()){
			mess.fro=res.front().fro;
			mess.buffer=res.front().ti+" "+mess.fro+"("+res.front().na+"):"+res.front().buf;
			mess.to=lic.acount;
			mess.flag='2';
			if(Send(clientfd,mess,ti)==false){
				cout<<"发送异常..."<<endl;
				break;	
			}
			res.pop();
		}
	
	}
	pthread_exit(0);
}
int main(){	
	signal(SIGCHLD,SIG_IGN);
	int clientfd,listenfd;
	if((listenfd=socket(AF_INET,SOCK_STREAM,0))>0){
		cout<<"socket 资源请求成功"<<endl;
	}
	else{
		perror("socket");
		close(listenfd);
	}
	
	sockaddr_in seraddr;
	seraddr.sin_family=AF_INET;
	seraddr.sin_port=htons(5000);
	seraddr.sin_addr.s_addr=htonl(INADDR_ANY);
	

	if(bind(listenfd,(sockaddr*)&seraddr,(socklen_t)sizeof(seraddr))==0){
		cout<<"bind success"<<endl;
	}
	else {
		perror("bind");	close(listenfd);return -1;
	}
	

	if(listen(listenfd,5)==0){
		cout<<"listen start..."<<endl;
	}
	else {
		perror("listen");close(listenfd);return -1;
	}
	sockaddr_in client;
	socklen_t len=sizeof(sockaddr);
	string name;	

	message mess;

	while(1){

		if((clientfd=accept(listenfd,(sockaddr*)&client,&len))<=0){
			continue;
		}
		if(fork()>0){	
			close(clientfd);
			continue;
		}close(listenfd);

		if(Recv(clientfd,mess,ti)==false){
			cout<<"recv fail..."<<endl;
		
			break;
		}

		if(mess.flag=='2'){
			cout<<"客户开始注册"<<endl;
		
			message mess1=Register(mess);
		
		
			Send(clientfd,mess1,ti);
			while(1){
				if(mess1.flag=='5')
					break;
				if(Recv(clientfd,mess,ti)==false)break;
				if(mess.flag=='6') break;
				mess1=Register(mess);
				if(Send(clientfd,mess1,ti)==false)break;
			}
			break;
		}			
		int log=0;
		for(int i=0;i<3;i++){			
			if(!Login(clientfd,lic,mess)){
				if(lic.name==""){
					mess.buffer="账号或密码错误...请重新输入";
					mess.flag='4';
					if(Send(clientfd,mess,ti)==false){
						cout<<"发送异常"<<endl;
						return 0;
					}
					if(i!=2&&Recv(clientfd,mess,ti)==false){
						cout<<"接收异常"<<endl;
						return 0;
					}
				}
				else {
					cout<<"服务器异常...退出"<<endl;
					mess.buffer="服务器异常...退出";
					mess.flag='4';
					Send(clientfd,mess,ti);
					if(clientfd>0)close(clientfd);
					return 0;
				}
			}

			else{
				mess.buffer="欢迎回来."+lic.name;
				cout<<"登录成功"<<endl;
				mess.flag='5';
				Send(clientfd,mess,ti);
				break;
			}
			if(i==2)log=1;
		}
		
		if(log==1)break;
		cout<<"开始多线程"<<endl;
			

		pthread_t pthid1,pthid2;
		pthread_t pthid3;
		pthread_create(&pthid1,NULL,main_read,(void*)(long)clientfd);
		pthread_create(&pthid2,NULL,main_print,(void*)(long)clientfd);	
		pthread_create(&pthid3,NULL,main_time,(void*)pthid1);
		pthread_join(pthid1,NULL);
		cout<<lic.acount<<":断开连接"<<endl;
		pthread_kill(pthid2,2);
		break;	

	
	}
	if(clientfd>0)close(clientfd);
	if(listenfd>0)close(listenfd);
	return 0;
}

