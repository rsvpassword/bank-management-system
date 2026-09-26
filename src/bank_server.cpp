// ============================================================
//  bank_server.cpp  ——  银行管理系统（多线程 TCP 服务器版）
// ------------------------------------------------------------
//  编译：
//    Linux   : g++ -O2 -std=c++17 -pthread bank_server.cpp -o bank_server
//    Windows : g++ -O2 -std=c++17 bank_server.cpp -o bank_server.exe -lws2_32
//              (MinGW / MSYS2)
//  运行：
//    ./bank_server [端口]          默认端口 8888
//
//  通信协议：
//    - 请求：一行文本，字段之间用 '|' 分隔，以 '\n' 结尾
//    - 响应：多行文本，最后以单独一行 "###END###" 结束
//    - 响应首行以 "OK|" 或 "ERR|" 开头表示成功/失败
//
//  命令一览：
//    PING
//    REGADMIN |银行名称|用户名|密码                       注册管理员(第一个无需登录)
//    LOGIN    |管理员ID|密码                               登录
//    QUIT
//    OPEN     |发卡商1/2/3|卡类别1/2|身份证号|姓名|密码     开户
//    CLOSE    |卡号|密码|身份证号|姓名|CVV|有效期          销户
//    TRANSFER |付款卡|付款密码|收款卡|收款密码|金额         汇款
//    CHANGEPWD|卡号|原密码|身份证号|姓名|新密码            改密码
//    FORGOT   |卡号|身份证号|姓名|新密码                   忘记密码
//    ACTIVATE |卡号|密码|CVV|有效期                        激活
//    INFO     |卡号|密码                                   查询
//    DEPOSIT  |卡号|密码|金额                              存款/还款
//    WITHDRAW |卡号|密码|身份证号|姓名|金额                取款
//    FREEZE   |卡号|密码|身份证号|姓名                     冻结
//    UNFREEZE |卡号|密码|身份证号|姓名                     解冻
//    SAVE                                                  手动落盘
// ============================================================

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#define CLOSE_SOCKET closesocket
#define SEND_FLAGS   0
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
using socket_t = int;
#define CLOSE_SOCKET close
#define SEND_FLAGS   MSG_NOSIGNAL
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif
#endif

#include <bits/stdc++.h>

using namespace std;

// ============================================================
//  常量 / 全局
// ============================================================
static const char*  RESP_END   = "###END###";
static const size_t MAX_LINE   = 1 << 16;     // 单行请求最大长度
static const long long MAX_REC = 1000000;     // 记录数上限

static mutex g_mtx;      // 保护所有共享数据（people / aadmin / 计数器）
static mutex g_logMtx;   // 保护日志文件

// ============================================================
//  数据结构
// ============================================================
struct Card {
	long long bb        = 0;   // 卡类别: 1=借记卡 2=信用卡
	long long jibie     = 1;   // 级别: 1/2/3
	long long money     = 0;   // 余额
	long long qiankuan  = 0;   // 欠款
	long long qkmaxx    = 0;   // 欠款上限（授信额度）
	
	string cvv;                // CVV
	string youxiaoqi;          // 有效期 MM/YY
	string idd;                // 身份证号
	string name;               // 姓名(英文)
	string bankcardid;         // 卡号
	string password;           // 密码
	string sendbank;           // 发卡商 UnionPay / VISA / MasterCard
	string opencard;           // 开卡银行
	
	long long zt = 0;          // 3冻结 2已激活 1未激活 -1已注销 0无
};

struct Admin {
	long long jb     = 0;      // 权限级别
	long long iddd   = 0;      // 工号
	long long zt     = 0;      // 状态
	string bankname;           // 银行名称
	string name;               // 用户名
	string password;           // 密码
};

// 下标从 1 开始，0 号元素弃用
static vector<Card>  people(1);
static long long     peoplecnt  = 0;
static vector<Admin> aadmin(1);
static long long     aadmincnt  = 0;

static long long mastercardkehu = 0, visakehu = 0, unionkehu = 0;
static const string mastercardhd = "5100";

static const double lilv = 0.06;   // 利率(百分数)，保留原样

// ============================================================
//  会话（每个 TCP 连接一份，取代原来的全局变量）
// ============================================================
struct Session {
	bool      logged  = false;
	long long adminId = 0;
};

// ============================================================
//  工具函数
// ============================================================
static vector<string> splitStr(const string& s, char d) {
	vector<string> r;
	string cur;
	for (char c : s) {
		if (c == d) { r.push_back(cur); cur.clear(); }
		else        { cur.push_back(c); }
	}
	r.push_back(cur);
	return r;
}

static string toUpper(string s) {
	for (char& c : s) c = (char)toupper((unsigned char)c);
	return s;
}

static bool toLL(const string& s, long long& out) {
	if (s.empty()) return false;
	try {
		size_t pos = 0;
		long long v = stoll(s, &pos);
		if (pos != s.size()) return false;
		out = v;
		return true;
	} catch (...) { return false; }
}

static bool toInt(const string& s, int& out) {
	long long v;
	if (!toLL(s, v)) return false;
	out = (int)v;
	return true;
}

// ============================================================
//  Luhn 校验（保留原实现）
// ============================================================
static int luhn1(const string& num) {          // 生成校验位
	int sum = 0;
	bool dbl = true;
	for (int i = (int)num.size() - 1; i >= 0; --i) {
		int d = num[i] - '0';
		if (dbl) { d *= 2; if (d > 9) d -= 9; }
		sum += d;
		dbl = !dbl;
	}
	return (10 - sum % 10) % 10;
}

static bool luhn2(const string& num) {         // 校验完整卡号
	int sum = 0;
	bool dbl = false;
	for (int i = (int)num.size() - 1; i >= 0; --i) {
		int d = num[i] - '0';
		if (dbl) { d *= 2; if (d > 9) d -= 9; }
		sum += d;
		dbl = !dbl;
	}
	return sum % 10 == 0;
}

// ============================================================
//  持久化
// ============================================================
static void Save() {
	ofstream fout("bank.in");
	if (!fout) return;
	fout << aadmincnt << '\n' << peoplecnt << '\n';
	for (long long i = 1; i <= aadmincnt; ++i) {
		fout << aadmin[i].zt << ' ' << aadmin[i].bankname << ' ' << aadmin[i].jb << ' '
		<< aadmin[i].iddd << ' ' << aadmin[i].name << ' ' << aadmin[i].password << '\n';
	}
	for (long long i = 1; i <= peoplecnt; ++i) {
		const Card& c = people[i];
		fout << c.zt << ' ' << c.opencard << ' ' << c.sendbank << ' ' << c.bb << ' '
		<< c.jibie << ' ' << c.bankcardid << ' ' << c.password << ' ' << c.money << ' '
		<< c.qiankuan << ' ' << c.idd << ' ' << c.name << ' ' << c.cvv << ' '
		<< c.youxiaoqi << ' ' << c.qkmaxx << '\n';
	}
	fout.close();
}

static void Read() {
	ifstream fin("bank.in");
	if (!fin) return;
	
	long long ac = 0, pc = 0;
	if (!(fin >> ac >> pc)) return;
	if (ac < 0 || ac > MAX_REC || pc < 0 || pc > MAX_REC) return;
	
	aadmincnt = ac; peoplecnt = pc;
	aadmin.assign(ac + 1, Admin());
	for (long long i = 1; i <= ac; ++i) {
		fin >> aadmin[i].zt >> aadmin[i].bankname >> aadmin[i].jb
		>> aadmin[i].iddd >> aadmin[i].name >> aadmin[i].password;
		if (fin.fail()) { aadmincnt = i - 1; break; }
	}
	people.assign(pc + 1, Card());
	for (long long i = 1; i <= pc; ++i) {
		Card& c = people[i];
		fin >> c.zt >> c.opencard >> c.sendbank >> c.bb >> c.jibie
		>> c.bankcardid >> c.password >> c.money >> c.qiankuan
		>> c.idd >> c.name >> c.cvv >> c.youxiaoqi >> c.qkmaxx;
		if (fin.fail()) { peoplecnt = i - 1; break; }
	}
	fin.close();
}

// ============================================================
//  日志
// ============================================================
static void logAction(const string& action, long long adminId,
					  long long money, long long from, long long to) {
	lock_guard<mutex> lk(g_logMtx);
	
	time_t now = time(nullptr);
	struct tm* utc_tm = gmtime(&now);
	char buffer[80] = {0};
	if (utc_tm) strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S UTC", utc_tm);
	
	ofstream fout("log.txt", ios::app);
	if (!fout.is_open()) return;
	
	fout << buffer << "    ";
	fout << action << ' ';
	if (action != "openingadmin") fout << adminId << ' ';
	if (action != "openingadmin" && action != "llogin")
		fout << money << ' ' << from << ' ' << to << ' ';
	fout << '\n';
	fout.close();
}

// ============================================================
//  卡号生成
// ============================================================
static string buildCardNo(long long idx, int sendbank, int type) {
	string card(16, '0');
	
	if (sendbank == 1) {                       // UnionPay 62
		card[0] = '6'; card[1] = '2'; card[2] = '0'; card[3] = '0';
	} else if (sendbank == 3) {                // VISA 4
		card[0] = '4'; card[1] = '0'; card[2] = '0'; card[3] = '0';
	} else {                                   // MasterCard 51
		card[0] = mastercardhd[0];
		card[1] = mastercardhd[1];
		card[2] = mastercardhd[2];
		card[3] = mastercardhd[3];
	}
	
	card[4] = '0';
	card[5] = (char)('0' + type);
	card[6] = '0';
	card[7] = '1';
	
	string s = to_string(idx);
	if (s.size() > 7) s = s.substr(s.size() - 7);
	int start = 15 - (int)s.size();
	for (int i = 8; i < start; ++i) card[i] = '0';
	for (size_t i = 0; i < s.size(); ++i) card[start + i] = s[i];
	
	card[15] = (char)('0' + luhn1(card.substr(0, 15)));
	return card;
}

// ============================================================
//  卡号 + 密码 查找，返回下标，失败返回 -1
// ============================================================
static long long checkCard(const string& num, const string& pwd) {
	if (!luhn2(num)) return -1;
	for (long long i = 1; i <= peoplecnt; ++i) {
		if (people[i].bankcardid == num && people[i].password == pwd)
			return i;
	}
	return -1;
}

static string stateText(long long zt) {
	if (zt == 3)  return "已冻结";
	if (zt == 2)  return "已激活";
	if (zt == 1)  return "未激活";
	if (zt == -1) return "已注销";
	return "0";
}

// ============================================================
//  各个业务命令
// ============================================================

// ---- REGADMIN -------------------------------------------------
static string cmdRegAdmin(Session& sess, const vector<string>& p) {
	if (p.size() < 4)
		return "ERR|用法: REGADMIN|银行名称|用户名|密码";
	if (aadmincnt > 0 && !sess.logged)
		return "ERR|请先登录管理员账号";
	if (aadmincnt >= MAX_REC)
		return "ERR|管理员数量已达上限";
	
	Admin a;
	a.bankname = p[1];
	a.name     = p[2];
	a.password = p[3];
	a.jb       = 1;
	a.zt       = 2;
	a.iddd     = aadmincnt + 1;
	
	aadmin.push_back(a);
	++aadmincnt;
	Save();
	logAction("openingadmin", aadmincnt, 0, 0, 0);
	
	return "OK|管理员创建成功, ID=" + to_string(aadmincnt);
}

// ---- LOGIN ----------------------------------------------------
static string cmdLogin(Session& sess, const vector<string>& p) {
	if (p.size() < 3)
		return "ERR|用法: LOGIN|管理员ID|密码";
	if (aadmincnt == 0)
		return "ERR|系统尚无管理员，请先使用 REGADMIN 注册";
	
	long long id;
	if (!toLL(p[1], id))
		return "ERR|管理员ID格式错误";
	if (id < 1 || id > aadmincnt)
		return "ERR|管理员不存在";
	if (aadmin[id].password != p[2])
		return "ERR|密码错误";
	if (aadmin[id].zt == -1)
		return "ERR|该管理员账号已注销";
	
	sess.logged  = true;
	sess.adminId = id;
	logAction("llogin", id, 0, 0, 0);
	
	return "OK|登录成功，欢迎 " + aadmin[id].name +
	"（" + aadmin[id].bankname + "）";
}

// ---- OPEN -----------------------------------------------------
static string cmdOpen(Session& sess, const vector<string>& p) {
	if (p.size() < 6)
		return "ERR|用法: OPEN|发卡商(1银联/2万事达/3VISA)|卡类别(1借记/2信用)|身份证号|姓名|密码";
	
	int sb = 0, type = 0;
	if (!toInt(p[1], sb) || !toInt(p[2], type))
		return "ERR|发卡商或卡类别格式错误";
	if (sb != 1 && sb != 2 && sb != 3)
		return "ERR|发卡商只能是 1/2/3";
	if (type != 1 && type != 2)
		return "ERR|卡类别只能是 1/2";
	
	const string& idd  = p[3];
	const string& name = p[4];
	const string& pwd  = p[5];
	
	if (idd.empty() || name.empty())
		return "ERR|身份证号 / 姓名 不能为空";
	if (pwd.size() != 6)
		return "ERR|密码必须为 6 位";
	
	long long idx = peoplecnt + 1;
	
	Card c;
	c.bankcardid = buildCardNo(idx, sb, type);
	c.jibie      = 1;
	c.opencard   = aadmin[sess.adminId].bankname;
	
	if (sb == 1)       { c.sendbank = "UnionPay";   ++unionkehu; }
	else if (sb == 3)  { c.sendbank = "VISA";       ++visakehu;  }
	else               { c.sendbank = "MasterCard"; ++mastercardkehu; }
	
	c.bb       = type;
	c.idd      = idd;
	c.name     = name;
	c.password = pwd;
	c.zt       = 1;
	c.qkmaxx   = (type == 2) ? 50000 : 0;   // 信用卡默认授信额度 5 万
	
	// 有效期：当前日期 + 6 年
	{
		time_t t = time(nullptr);
		struct tm* lt = localtime(&t);
		int years = 2025, months = 1;
		if (lt) { years = lt->tm_year + 1900; months = lt->tm_mon + 1; }
		years += 6;
		char buf[16];
		snprintf(buf, sizeof(buf), "%02d/%02d", months, years % 100);
		c.youxiaoqi = buf;
	}
	// CVV
	{
		char buf[8];
		snprintf(buf, sizeof(buf), "%03d", rand() % 1000);
		c.cvv = buf;
	}
	
	people.push_back(c);
	++peoplecnt;
	Save();
	logAction("khh", sess.adminId, 0, idx, 0);
	
	ostringstream oss;
	oss << "OK|开户成功\n"
	<< "卡号: "      << c.bankcardid << '\n'
	<< "密码: "      << c.password   << '\n'
	<< "CVV: "       << c.cvv        << '\n'
	<< "有效期: "    << c.youxiaoqi  << '\n'
	<< "发卡商: "    << c.sendbank   << '\n'
	<< "开卡银行: "  << c.opencard   << '\n'
	<< "卡类别: "    << (c.bb == 1 ? "借记卡" : "信用卡") << '\n'
	<< "级别: "      << c.jibie      << '\n'
	<< "状态: "      << stateText(c.zt);
	return oss.str();
}

// ---- CLOSE ----------------------------------------------------
static string cmdClose(Session& sess, const vector<string>& p) {
	if (p.size() < 7)
		return "ERR|用法: CLOSE|卡号|密码|身份证号|姓名|CVV|有效期";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	if (c.idd != p[3] || c.name != p[4] || c.cvv != p[5] || c.youxiaoqi != p[6])
		return "ERR|身份证号 / 姓名 / CVV / 有效期 不匹配";
	if (c.zt == -1)
		return "ERR|该卡已注销";
	
	c.zt = -1;
	Save();
	logAction("xhh", sess.adminId, 0, a, 0);
	return "OK|销卡成功";
}

// ---- TRANSFER -------------------------------------------------
static string cmdTransfer(Session& sess, const vector<string>& p) {
	if (p.size() < 6)
		return "ERR|用法: TRANSFER|付款卡号|付款密码|收款卡号|收款密码|金额";
	
	long long a = checkCard(p[1], p[2]);
	long long b = checkCard(p[3], p[4]);
	if (a == -1) return "ERR|付款卡号或密码错误";
	if (b == -1) return "ERR|收款卡号或密码错误";
	
	long long amt;
	if (!toLL(p[5], amt)) return "ERR|金额格式错误";
	if (amt <= 0)         return "ERR|金额必须大于 0";
	if (a == b)           return "ERR|不能给自己转账";
	
	Card& src = people[a];
	Card& dst = people[b];
	
	if (src.zt != 2) return "ERR|付款卡未激活";
	if (dst.zt != 2) return "ERR|收款卡未激活";
	
	if (src.bb == 1) {                 // 借记卡
		if (src.money < amt) return "ERR|余额不足";
		src.money -= amt;
		dst.money += amt;
	} else {                           // 信用卡
		if (src.money >= amt) {
			src.money -= amt;
			dst.money += amt;
		} else if (src.money > 0) {
			long long use  = src.money;
			long long rest = amt - use;
			if (rest > src.qkmaxx - src.qiankuan)
				return "ERR|可用额度不足";
			src.money = 0;
			src.qiankuan += rest;
			dst.money += amt;
		} else {
			if (amt > src.qkmaxx - src.qiankuan)
				return "ERR|可用额度不足";
			src.qiankuan += amt;
			dst.money += amt;
		}
	}
	
	Save();
	logAction("outt", sess.adminId, amt, a, b);
	return "OK|汇款成功，金额 " + to_string(amt);
}

// ---- CHANGEPWD ------------------------------------------------
static string cmdChangePwd(Session& sess, const vector<string>& p) {
	if (p.size() < 6)
		return "ERR|用法: CHANGEPWD|卡号|原密码|身份证号|姓名|新密码";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	if (c.idd != p[3] || c.name != p[4])
		return "ERR|身份证号或姓名不匹配";
	if (p[5].size() != 6)
		return "ERR|新密码必须为 6 位";
	
	c.password = p[5];
	Save();
	logAction("changepassword", sess.adminId, 0, a, 0);
	return "OK|密码修改成功";
}

// ---- FORGOT ---------------------------------------------------
static string cmdForgot(Session& sess, const vector<string>& p) {
	if (p.size() < 5)
		return "ERR|用法: FORGOT|卡号|身份证号|姓名|新密码";
	
	if (!luhn2(p[1])) return "ERR|卡号校验失败";
	
	long long a = -1;
	for (long long i = 1; i <= peoplecnt; ++i)
		if (people[i].bankcardid == p[1]) { a = i; break; }
	if (a == -1) return "ERR|卡号不存在";
	
	Card& c = people[a];
	if (c.idd != p[2] || c.name != p[3])
		return "ERR|身份证号或姓名不匹配";
	if (p[4].size() != 6)
		return "ERR|新密码必须为 6 位";
	
	c.password = p[4];
	Save();
	logAction("changepassword", sess.adminId, 0, a, 0);
	return "OK|密码重置成功";
}

// ---- ACTIVATE -------------------------------------------------
static string cmdActivate(Session& sess, const vector<string>& p) {
	if (p.size() < 5)
		return "ERR|用法: ACTIVATE|卡号|密码|CVV|有效期";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	if (c.cvv != p[3] || c.youxiaoqi != p[4])
		return "ERR|CVV 或有效期不匹配";
	if (c.zt == -1) return "ERR|该卡已注销";
	
	c.zt = 2;
	Save();
	logAction("jihuo", sess.adminId, 0, a, 0);
	return "OK|激活成功";
}

// ---- INFO -----------------------------------------------------
static string cmdInfo(Session& sess, const vector<string>& p) {
	if (p.size() < 3)
		return "ERR|用法: INFO|卡号|密码";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	ostringstream oss;
	oss << "OK|查询成功\n"
	<< "卡号: "       << c.bankcardid << '\n'
	<< "CVV: "        << c.cvv        << '\n'
	<< "有效期: "     << c.youxiaoqi  << '\n'
	<< "余额: "       << c.money      << '\n'
	<< "欠款: "       << c.qiankuan   << '\n'
	<< "最大可借: "   << c.qkmaxx     << '\n'
	<< "发卡行: "     << c.opencard   << '\n'
	<< "开卡商: "     << c.sendbank   << '\n'
	<< "身份证号: "   << c.idd        << '\n'
	<< "姓名: "       << c.name       << '\n'
	<< "级别: "       << c.jibie      << '\n'
	<< "卡类别: "     << (c.bb == 1 ? "借记卡" : "信用卡") << '\n'
	<< "卡状态: "     << stateText(c.zt);
	return oss.str();
}

// ---- DEPOSIT --------------------------------------------------
static string cmdDeposit(Session& sess, const vector<string>& p) {
	if (p.size() < 4)
		return "ERR|用法: DEPOSIT|卡号|密码|金额";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	long long amt;
	if (!toLL(p[3], amt)) return "ERR|金额格式错误";
	if (amt <= 0)         return "ERR|金额必须大于 0";
	
	Card& c = people[a];
	if (c.zt != 2) return "ERR|该卡未激活";
	
	if (c.bb == 1) {
		c.money += amt;
		Save();
		logAction("inn", sess.adminId, amt, a, 888);
		return "OK|存款成功，当前余额 " + to_string(c.money);
	} else {
		if (c.qiankuan == 0) {
			c.money += amt;
			Save();
			logAction("inn", sess.adminId, amt, a, 888);
			return "OK|存款成功，当前余额 " + to_string(c.money);
		} else {
			if (amt >= c.qiankuan) {
				long long left = amt - c.qiankuan;
				c.qiankuan = 0;
				c.money += left;
				Save();
				logAction("hkk", sess.adminId, amt, a, 888);
				return "OK|还款成功，剩余 " + to_string(left) +
				" 已存入余额，当前欠款 0";
			} else {
				c.qiankuan -= amt;
				Save();
				logAction("hkk", sess.adminId, amt, a, 888);
				return "OK|还款成功，剩余欠款 " + to_string(c.qiankuan);
			}
		}
	}
}

// ---- WITHDRAW -------------------------------------------------
static string cmdWithdraw(Session& sess, const vector<string>& p) {
	if (p.size() < 6)
		return "ERR|用法: WITHDRAW|卡号|密码|身份证号|姓名|金额";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	if (c.zt != 2) return "ERR|该卡未激活";
	if (c.idd != p[3] || c.name != p[4])
		return "ERR|姓名或身份证号错误";
	
	long long amt;
	if (!toLL(p[5], amt)) return "ERR|金额格式错误";
	if (amt <= 0)         return "ERR|金额必须大于 0";
	
	if (c.bb == 1) {                     // 借记卡
		if (amt > c.money) return "ERR|余额不足";
		c.money -= amt;
	} else {                             // 信用卡
		long long avail = c.money + (c.qkmaxx - c.qiankuan);
		if (amt > avail) return "ERR|可用额度不足";
		if (amt <= c.money) {
			c.money -= amt;
		} else {
			long long rest = amt - c.money;
			c.money = 0;
			c.qiankuan += rest;
		}
	}
	
	Save();
	logAction("outt", sess.adminId, amt, a, 888);
	return "OK|取款成功，金额 " + to_string(amt);
}

// ---- FREEZE ---------------------------------------------------
static string cmdFreeze(Session& sess, const vector<string>& p) {
	if (p.size() < 5)
		return "ERR|用法: FREEZE|卡号|密码|身份证号|姓名";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	if (c.idd != p[3] || c.name != p[4])
		return "ERR|身份证号或姓名不匹配";
	if (c.zt == -1) return "ERR|该卡已注销";
	
	c.zt = 3;
	Save();
	logAction("dongjie", sess.adminId, 0, a, 0);
	return "OK|该卡已冻结";
}

// ---- UNFREEZE -------------------------------------------------
static string cmdUnfreeze(Session& sess, const vector<string>& p) {
	if (p.size() < 5)
		return "ERR|用法: UNFREEZE|卡号|密码|身份证号|姓名";
	
	long long a = checkCard(p[1], p[2]);
	if (a == -1) return "ERR|卡号或密码错误";
	
	Card& c = people[a];
	if (c.idd != p[3] || c.name != p[4])
		return "ERR|身份证号或姓名不匹配";
	if (c.zt == -1) return "ERR|该卡已注销";
	
	c.zt = 1;
	Save();
	logAction("jiedong", sess.adminId, 0, a, 0);
	return "OK|该卡已解冻（状态转为未激活）";
}

// ============================================================
//  请求分发
// ============================================================
static string dispatch(Session& sess, const string& line, bool& quit) {
	quit = false;
	
	vector<string> p = splitStr(line, '|');
	if (p.empty() || p[0].empty()) return "ERR|空请求";
	
	string cmd = toUpper(p[0]);
	
	if (cmd == "PING")   return "OK|PONG";
	if (cmd == "QUIT") { quit = true; return "OK|BYE"; }
	if (cmd == "REGADMIN") return cmdRegAdmin(sess, p);
	if (cmd == "LOGIN")    return cmdLogin(sess, p);
	
	if (!sess.logged) return "ERR|请先登录（LOGIN|管理员ID|密码）";
	
	if (cmd == "SAVE")     { Save(); return "OK|已保存"; }
	if (cmd == "OPEN")     return cmdOpen(sess, p);
	if (cmd == "CLOSE")    return cmdClose(sess, p);
	if (cmd == "TRANSFER") return cmdTransfer(sess, p);
	if (cmd == "CHANGEPWD")return cmdChangePwd(sess, p);
	if (cmd == "FORGOT")   return cmdForgot(sess, p);
	if (cmd == "ACTIVATE") return cmdActivate(sess, p);
	if (cmd == "INFO")     return cmdInfo(sess, p);
	if (cmd == "DEPOSIT")  return cmdDeposit(sess, p);
	if (cmd == "WITHDRAW") return cmdWithdraw(sess, p);
	if (cmd == "FREEZE")   return cmdFreeze(sess, p);
	if (cmd == "UNFREEZE") return cmdUnfreeze(sess, p);
	
	return "ERR|未知命令: " + cmd;
}

// ============================================================
//  网络读写
// ============================================================
static bool recvLine(socket_t s, string& out) {
	out.clear();
	char ch;
	while (true) {
		int n = (int)recv(s, &ch, 1, 0);
		if (n <= 0) return false;
		if (ch == '\n') {
			if (!out.empty() && out.back() == '\r') out.pop_back();
			return true;
		}
		out.push_back(ch);
		if (out.size() > MAX_LINE) return false;
	}
}

static bool sendAll(socket_t s, const string& data) {
	size_t sent = 0;
	while (sent < data.size()) {
		int n = (int)send(s, data.data() + sent,
						  (int)(data.size() - sent), SEND_FLAGS);
		if (n <= 0) return false;
		sent += (size_t)n;
	}
	return true;
}

// ============================================================
//  客户端线程
// ============================================================
static void handleClient(socket_t sock, string ip) {
	Session sess;
	string line;
	bool quit = false;
	
	while (recvLine(sock, line)) {
		if (line.empty()) continue;
		
		string resp;
		{
			lock_guard<mutex> lk(g_mtx);
			try {
				resp = dispatch(sess, line, quit);
			} catch (const exception& e) {
				resp = string("ERR|服务器内部错误: ") + e.what();
			} catch (...) {
				resp = "ERR|服务器内部错误";
			}
		}
		
		resp += "\n";
		resp += RESP_END;
		resp += "\n";
		
		if (!sendAll(sock, resp)) break;
		if (quit) break;
	}
	
	CLOSE_SOCKET(sock);
	
	{
		lock_guard<mutex> lk(g_logMtx);
		printf("[断开] 客户端 %s 已断开\n", ip.c_str());
		fflush(stdout);
	}
}

// ============================================================
//  main
// ============================================================
int main(int argc, char** argv) {
#ifdef _WIN32
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		fprintf(stderr, "WSAStartup 失败\n");
		return 1;
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif
	
	int port = 8888;
	if (argc > 1) {
		int t = atoi(argv[1]);
		if (t > 0 && t <= 65535) port = t;
	}
	
	{
		lock_guard<mutex> lk(g_mtx);
		Read();
	}
	srand((unsigned)time(nullptr));
	
	socket_t listenSock = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSock == INVALID_SOCKET) {
		fprintf(stderr, "创建 socket 失败\n");
		return 1;
	}
	
	int opt = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR,
			   (const char*)&opt, sizeof(opt));
	
	sockaddr_in addr{};
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port        = htons((unsigned short)port);
	
	if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
		fprintf(stderr, "bind 失败，端口 %d 可能已被占用\n", port);
		CLOSE_SOCKET(listenSock);
		return 1;
	}
	if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
		fprintf(stderr, "listen 失败\n");
		CLOSE_SOCKET(listenSock);
		return 1;
	}
	
	printf("=============================================\n");
	printf("  银行管理服务器已启动\n");
	printf("  监听端口 : %d\n", port);
	printf("  数据文件 : bank.in\n");
	printf("  日志文件 : log.txt\n");
	printf("=============================================\n");
	fflush(stdout);
	
	while (true) {
		sockaddr_in cli{};
		socklen_t clilen = sizeof(cli);
		socket_t c = accept(listenSock, (sockaddr*)&cli, &clilen);
		if (c == INVALID_SOCKET) continue;
		
		char ip[64] = {0};
		inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
		
		{
			lock_guard<mutex> lk(g_logMtx);
			printf("[接入] 客户端 %s:%d\n", ip, ntohs(cli.sin_port));
			fflush(stdout);
		}
		
		thread(handleClient, c, string(ip)).detach();
	}
	
	CLOSE_SOCKET(listenSock);
#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
