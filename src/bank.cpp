#include<bits/stdc++.h>
#include<windows.h>
#define ll long long
#define random(a,b) ((rand()%((b)-(a)+1))+(a))
using namespace std;
time_t now = time(nullptr);
struct tm* utc = gmtime(&now);
//srand((unsigned)time(NULL));
struct QWER{
	long long bb,jibie,money,qiankuan,qkmaxx;
	string cvv,youxiaoqi;
	string idd,name;
	string bankcardid,password;
	string sendbank,opencard;
	long long zt;
};
struct ASDF{
	long long jb,iddd;
	long long zt;
	string bankname,name,password;
};
double lilv=0.06;
QWER people[1000005];
long long peoplecnt;
ASDF aadmin[1000005];
long long aadmincnt;
string sss;
long long logining;
long long mastercardkehu,visakehu,unionkehu;
string mastercardhd="5100";
string dongzuo="";
long long money,fromname,toname;
long long loginadmin;
void Save(){
	ofstream fout("bank.in");
	fout<<aadmincnt<<endl<<peoplecnt<<endl;
	for(int i=1;i<=aadmincnt;i++){
		fout<<aadmin[i].zt<<' '<<aadmin[i].bankname<<' '<<aadmin[i].jb<<' '<<aadmin[i].iddd<<' '<<aadmin[i].name<<' '<<aadmin[i].password<<endl;
	}
	for(int i=1;i<=peoplecnt;i++){
		fout<<people[i].zt<<' '<<people[i].opencard<<' '<<people[i].sendbank<<' '<<people[i].bb<<' '<<people[i].jibie<<' '<<people[i].bankcardid<<' '<<people[i].password<<' '<<people[i].money<<' '<<people[i].qiankuan<<' '<<people[i].idd<<' '<<people[i].name<<' '<<people[i].cvv<<' '<<people[i].youxiaoqi<<' '<<people[i].qkmaxx<<endl;
	}
	fout<<endl;
	fout.close();
}
void Read(){
	ifstream fin("bank.in");
	if (!fin) return;
	fin>>aadmincnt>>peoplecnt;
	for(int i=1;i<=aadmincnt;i++){
		fin>>aadmin[i].zt>>aadmin[i].bankname>>aadmin[i].jb>>aadmin[i].iddd>>aadmin[i].name>>aadmin[i].password;
	}
	for(int i=1;i<=peoplecnt;i++){
		fin>>people[i].zt>>people[i].opencard>>people[i].sendbank>>people[i].bb>>people[i].jibie>>people[i].bankcardid>>people[i].password>>people[i].money>>people[i].qiankuan>>people[i].idd>>people[i].name>>people[i].cvv>>people[i].youxiaoqi>>people[i].qkmaxx;
	}
	fin.close();
}
int luhn1(string num) {
	int sum = 0;
	bool doubleDigit = true;
	for (int i = num.size() - 1; i >= 0; --i) {
		int digit = num[i] - '0';
		if (doubleDigit) {
			digit *= 2;
			if (digit > 9) digit -= 9;
		}
		sum += digit;
		doubleDigit = !doubleDigit;
	}
	return (10 - sum % 10) % 10;
}
bool luhn2(string num) {
	int sum = 0;
	bool doubleDigit = false;
	for (int i = num.size() - 1; i >= 0; --i) {
		int digit = num[i] - '0';
		if (doubleDigit) {
			digit *= 2;
			if (digit > 9) digit -= 9;
		}
		sum += digit;
		doubleDigit = !doubleDigit;
	}
	return sum % 10 == 0;
}
void logg() {
	time_t now = time(nullptr);
	struct tm* utc_tm = gmtime(&now);
	char buffer[80];
	strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", utc_tm);
	string timeStr(buffer);
	ofstream fout("log.txt", ios::app);
	if (fout.is_open()) {
		fout << timeStr <<"    ";
		fout<<dongzuo<<' ';
		if(dongzuo!=""&&dongzuo!="openingadmin"){
			fout<<loginadmin<<' ';
		}
		if(dongzuo!=""&&dongzuo!="openingadmin"&&dongzuo!="llogin"){
			fout<<money<<' '<<fromname<<' '<<toname<<' ';
		}
		fout<<endl; 
		fout.close();
	}
}
int check(string num, string passwork) {
	if (luhn2(num) == false) {
		return -1;
	}
	for (int i = 1; i <= peoplecnt; i++) {
		if (people[i].bankcardid == num) {
			if (people[i].password == passwork) {
				return i;
			}
		}
	}
	return -1;
}
void list1(){
	printf("请输入卡号以及密码\n");
	string a,b;
	cin>>a>>b;
	int aa=check(a,b);
	if(aa!=-1){
		cout<<"卡号:"<<people[aa].bankcardid<<endl;
		cout<<"密码:"<<people[aa].password<<endl;
		cout<<"卡CVV"<<people[aa].cvv<<endl;
		cout<<"卡有效期:"<<people[aa].youxiaoqi<<endl;
		cout<<"余额:"<<people[aa].money<<endl;
		cout<<"欠款:"<<people[aa].qiankuan<<endl;
		cout<<"最大可借:"<<people[aa].qkmaxx<<endl;
		cout<<"发卡行:"<<people[aa].opencard<<endl;
		cout<<"开卡商:"<<people[aa].sendbank<<endl;
		cout<<"持卡人身份证号:"<<people[aa].idd<<endl;
		cout<<"持卡人姓名:"<<people[aa].name<<endl;
		cout<<"级别:"<<people[aa].jibie<<endl;
		cout<<"卡类别:"<<people[aa].bb<<endl;
		if(people[aa].zt==3) cout<<"卡状态:已冻结\n";
		else if(people[aa].zt==2) cout<<"卡状态:已激活\n";
		else if(people[aa].zt==1) cout<<"卡状态:未激活\n";
		else if(people[aa].zt==-1) cout<<"卡状态:已注销\n";
		else cout<<"卡状态:0\n";
		system("pause");
		system("cls");
	}else{
		printf("输入错误\n");
		system("pause");
		system("cls");
	}
}
void opening(){
	printf("请选择发卡商 1.UnionPay(中国银联) 2.MasterCard(万事达) 3.VISA(Tips:一旦输入不为1或2或3那将默认注册UnionPay(中国银联)卡)\n");
	printf("请选择开卡类别 1.借记卡 2.信用卡\n");
	printf("身份证号 姓名(英文)\n");
	int a,b;string c,d;
	cin>>a>>b>>c>>d;
	if((a!=1&&a!=2&&a!=3)||(b!=1&&b!=2)){
		printf("输入错误\n");
		system("pause");
		system("cls");
	}else{
		peoplecnt++;
		people[peoplecnt].bankcardid = string(16, '0');
		people[peoplecnt].jibie = 1;
		people[peoplecnt].opencard = aadmin[loginadmin].bankname;
		if (a == 1) {
			people[peoplecnt].sendbank = "UnionPay";
			people[peoplecnt].bankcardid[0] = '6';
			people[peoplecnt].bankcardid[1] = '2';
			people[peoplecnt].bankcardid[2] = '0';
			people[peoplecnt].bankcardid[3] = '0';
			unionkehu += 1;
		} else if (a == 3) {
			people[peoplecnt].sendbank = "VISA";
			people[peoplecnt].bankcardid[0] = '4';
			people[peoplecnt].bankcardid[1] = '0';
			people[peoplecnt].bankcardid[2] = '0';
			people[peoplecnt].bankcardid[3] = '0';
			visakehu += 1;
		} else if (a == 2) {
			people[peoplecnt].sendbank = "MasterCard";
			people[peoplecnt].bankcardid[0] = mastercardhd[0];
			people[peoplecnt].bankcardid[1] = mastercardhd[1];
			people[peoplecnt].bankcardid[2] = mastercardhd[2];
			people[peoplecnt].bankcardid[3] = mastercardhd[3];
			mastercardkehu += 1;
		}
		people[peoplecnt].bankcardid[4] = '0';
		people[peoplecnt].bankcardid[5] = '0' + b;
		people[peoplecnt].bankcardid[6] = '0';
		people[peoplecnt].bankcardid[7] = '1';
		people[peoplecnt].bb = b;
		people[peoplecnt].idd = c;
		people[peoplecnt].name = d;
		string peoplecntt = to_string(peoplecnt);
		int len = peoplecntt.length();
		int start = 15 - len;
		if (len > 7) {
			len = 7;
			peoplecntt = peoplecntt.substr(peoplecntt.length() - 7);
			start = 8;
		}
		for (int i = 0; i < len; i++) {
			people[peoplecnt].bankcardid[start + i] = peoplecntt[i];
		}
		for (int i = 8; i < start; i++) {
			people[peoplecnt].bankcardid[i] = '0';
		}
		string cardWithoutCheck = people[peoplecnt].bankcardid.substr(0, 15);
		int check = luhn1(cardWithoutCheck);
		people[peoplecnt].bankcardid[15] = '0' + check;
		printf("请输入6位卡密码\n");
		string a_pwd;
		cin >> a_pwd;
		people[peoplecnt].password = a_pwd;
		int years = 0, months = 0;
		std::time_t t = std::time(nullptr);
		std::tm* now = std::localtime(&t);
		if (now != nullptr) {
			years = now->tm_year + 1900;
			months = now->tm_mon + 1;
		}
		years += 6;
		int year2 = years % 100;
		char expbuf[6];
		snprintf(expbuf, sizeof(expbuf), "%02d/%02d", months, year2);
		people[peoplecnt].youxiaoqi = expbuf;
		int cv = rand() % 1000;
		char cvvbuf[4];
		snprintf(cvvbuf, sizeof(cvvbuf), "%03d", cv);
		people[peoplecnt].cvv = cvvbuf;
		people[peoplecnt].zt=1;
		cout<<"卡号:"<<people[peoplecnt].bankcardid<<endl;
		cout<<"密码:"<<people[peoplecnt].password<<endl;
		cout<<"CVV:"<<people[peoplecnt].cvv<<endl;
		cout<<"有效期:"<<people[peoplecnt].youxiaoqi<<endl;
		system("pause");
		system("cls");
	}
	Save();
}
void closeing(){
	printf("请输入卡号以及密码,还有与之所绑定的身份证号以及姓名,还有CVV和有效期\n");
	string a,b,c,d,e,f;
	cin>>a>>b>>c>>d>>e>>f;
	int aa=check(a,b);
	if(people[aa].idd!=c||people[aa].name!=d||people[aa].cvv!=e||people[aa].youxiaoqi!=f){
		printf("输入错误\n");
	}else{
		printf("您确定要销掉这张卡吗?");
		printf("Y/N");
		char cc;cin>>cc;
		if(cc=='N'){
			printf("销卡已取消\n");
		}else if(cc=='Y'){
			people[aa].zt=-1;
			printf("销卡成功\n");
		}else{
			printf("输入错误\n");
		}
	}
	system("pause");
	system("cls");
	Save();
}
void huikuan(){
	printf("请输入付款卡号以及密码和收款卡号以及密码\n");
	string a,b,c,d;cin>>a>>b>>c>>d;
	int aa=check(a,b),bb=check(c,d);
	printf("汇款多少\n");
	long long hkk;cin>>hkk;
	if(people[aa].zt!=2||people[bb].zt!=2){
		printf("未激活\n");
		system("pause");
		system("cls");
		return ;
	}
	if(people[aa].bb==1){//借记卡
		if(people[aa].money>=hkk){
			printf("汇款成功\n");
			people[aa].money-=hkk;
			people[bb].money+=hkk;
		}else{
			printf("汇款失败\n");
		}
	}
	if(people[aa].bb==2){//信用卡
		if(people[aa].money>=hkk){
			printf("汇款成功\n");
			people[aa].money-=hkk;
			people[bb].money+=hkk;
		}else if(people[aa].money>0){
			printf("汇款成功\n");
			int linshi=people[aa].money;
			people[aa].money=0;
			people[bb].money+=linshi;
			people[aa].qiankuan+=hkk-linshi;
			people[bb].money+=hkk-linshi;
		}else if(people[aa].qkmaxx-people[aa].qiankuan>=hkk){
			printf("汇款成功\n");
			people[aa].qiankuan+=hkk;
			people[bb].money+=hkk;
		}else{
			printf("汇款失败\n");
		}
	}
	system("pause");
	system("cls");
	Save();
}
void changebankcardpassword(){
	printf("请输入卡号以及密码,还有与之所绑定的身份证号以及姓名\n");
	string a,b,c,d;cin>>a>>b;
	int aa=check(a,b);
	if(people[aa].idd==c&&people[aa].name==d){
		printf("请输入更改后的6位密码\n");
		string bb;cin>>bb;
		people[aa].password=bb;
		printf("更改成功\n");
	}else{
		printf("输入失败\n");
	}
	system("pause");
	system("cls");
	Save();
}
void jihuo(){
	printf("请输入卡号以及密码,还有CVV和有效期\n");
	string a,b,c,d;cin>>a>>b>>c>>d;
	int aa=check(a,b);
	if(people[aa].cvv!=c||people[aa].youxiaoqi!=d){
		printf("输入错误\n");
		system("pause");
		system("cls");
	}else{
		printf("激活成功\n");
		people[aa].zt=2;
	}
	system("pause");
	system("cls");
	Save();
}
void dongjie(){
	printf("请输入卡号以及密码,还有与之所绑定的身份证号以及姓名\n");
	string a,b,c,d;cin>>a>>b;
	int aa=check(a,b);
	if(people[aa].idd==c&&people[aa].name==d){
		printf("确定冻结?\n");
		printf("Y/N");
		char c;cin>>c;
		if(c=='Y'){
			printf("已冻结\n");
			people[aa].zt=3;
		}else if(c=='N'){
			printf("已取消\n");
		}else{
			printf("输入错误\n");
		}
	}else{
		printf("输入错误\n");
	}
	system("pause");
	system("cls");
	Save();
}
void jiedong(){
	printf("请输入卡号以及密码,还有与之所绑定的身份证号以及姓名\n");
	string a,b,c,d;cin>>a>>b;
	int aa=check(a,b);
	if(people[aa].idd==c&&people[aa].name==d){
		printf("确定解冻?\n");
		printf("Y/N");
		char c;cin>>c;
		if(c=='Y'){
			printf("已解冻(状态已转为未激活)\n");
			people[aa].zt=1;
		}else if(c=='N'){
			printf("已取消\n");
		}else{
			printf("输入错误\n");
		}
	}else{
		printf("输入错误\n");
	}
	system("pause");
	system("cls");
	Save();
}
void inn(){
	printf("请输入卡号以及密码\n");
	string a,b;cin>>a>>b;
	int aa=check(a,b);
	if(aa!=-1||people[aa].zt==2){
		if(people[aa].bb==1){
			cout<<"余额:"<<people[aa].money<<endl;
			printf("要存多少\n");
			int ab;cin>>ab;
			printf("存款成功\n");
			people[aa].money+=ab;
		}
		if(people[aa].bb==2){
			cout<<"余额:"<<people[aa].money<<endl;
			cout<<"欠款额:"<<people[aa].qiankuan<<endl;
			printf("要存/还多少(有欠款的情况下会优先还款)\n");
			int ab;cin>>ab;
			if(people[aa].qiankuan==0){
				people[aa].money+=ab;
				printf("存款成功\n");
			}else{
				people[aa].qiankuan-=ab;
				printf("还款成功\n");
			}
		}
	}else{
		printf("未查询到该卡或密码错误\n");
		system("pause");
		system("cls");
	}
	system("pause");
	system("cls");
	Save();
}
void outt(){
	printf("请输入卡号以及密码,还有与之所绑定的身份证号以及姓名\n");
	string a,b,c,d;
	cin>>a>>b>>c>>d;
	int aa=check(a,b);
	if(aa!=-1||people[aa].zt==2){
		if(people[aa].idd!=c||people[aa].name!=d){
			printf("姓名或身份证号错误\n");
			system("pause");
			system("cls");
			return ;
		}else{
			if(people[aa].bb==1){
				cout<<"余额:"<<people[aa].money<<endl;
				printf("要取多少\n");
				int ab;cin>>ab;
				if(ab<=people[aa].money){
					printf("取款成功\n");
					people[aa].money-=ab;
				}else{
					printf("失败\n");
				}
			}
			if(people[aa].bb==2){
				cout<<"可借:"<<people[aa].qkmaxx-people[aa].qiankuan<<endl;
				printf("要取多少\n");
				int ab;cin>>ab;
				if(ab<=people[aa].money){
					printf("取款成功\n");
					people[aa].qiankuan+=ab;
				}else{
					printf("失败\n");
				}
			}
		}
	}else{
		printf("未查询到该卡或密码错误\n");
		system("pause");
		system("cls");
	}
	system("pause");
	system("cls");
	Save();
}
void messingpassword(){
	printf("请输入卡号以及密码,还有与之所绑定的身份证号以及姓名\n");
	string a,b,c,d;cin>>a>>b;
	int aa=check(a,b);
	if(people[aa].idd==c&&people[aa].name==d){
		printf("请输入更改后的6位密码\n");
		string bb;cin>>bb;
		people[aa].password=bb;
		printf("更改成功\n");
	}else{
		printf("输入失败\n");
	}
	system("pause");
	system("cls");
	Save();
}
void mainn(){
	system("pause");
	while(1){
		system("cls");
		printf("管理后台\n");
		printf("0.退出 1.开户 2.销户 3.汇款 4.更改银行卡密码 5.忘记密码 6.激活 7.开设管理账户(敬请期待) 8.查询卡信息 9.存款 10.取款 11.冻结 12.解冻 13.\n");
		int a;cin>>a;
		if(a==1){
			opening();
		}else if(a==2){
			closeing();
		}else if(a==3){
			huikuan();
		}else if(a==4){
			changebankcardpassword();
		}else if(a==5){
			messingpassword();
		}else if(a==6){
			jihuo();
		}else if(a==7){
			//TODO----------
		}else if(a==8){
			list1();
		}else if(a==9){
			inn();
		}else if(a==10){
			outt();
		}else if(a==11){
			dongjie();//
		}else if(a==12){
			jiedong();
		}else if(a==0){
			break;
		}else{
			printf("重新输入\n");
			system("pause");
			system("cls");
		}
	}
}
void login(){
	if(aadmin[1].name==""){
		printf("请先注册管理员账号\n");
		printf("请输入用户名\n");
		cin>>aadmin[1].name;
		printf("请输入密码\n");
		cin>>aadmin[1].password;
		aadmin[1].bankname="Administrator";
		aadmin[1].jb=1;
		aadmin[1].iddd=1;
		aadmincnt++;
		printf("注册成功\nID编号为:1\n");
		system("pause");
		system("cls");
	}
	Save();
	int s1;string s2;
	printf("请输入ID编号以及密码\n");
	cin>>s1>>s2;
	if(aadmin[s1].password==s2){
		loginadmin=s1;
		printf("登录成功\n");
		system("pause");
		system("cls");
		mainn();
		return ;
	}else{
		printf("用户ID输入错误或密码错误!\n");
		system("pause");
		system("cls");
	}
}
int main(){
	Read();
	Save();
	srand((unsigned)time(NULL));
	printf("欢迎来到银行管理后台\n");
	login();
	return 0;
}
