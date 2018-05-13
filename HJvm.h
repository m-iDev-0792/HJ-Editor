#pragma once
#include<iostream>
#include<string>
#include<vector>
#include<unordered_map>
#include<QObject>
#define DEBUG		//HJvm测试输出
//#define OUTPUTCONST	//输出常量


using namespace std;

/*
汇编语句结构体
cmd为汇编命令
arg1 arg2 为参数
*/
class ASMCode {
private:
	string cmd ;
	string arg1 ;
	string arg2;
public:
	ASMCode(const string &cmd, const string &arg1 = "#", const string & arg2="#") {
		this->cmd = cmd;
		this->arg1 = arg1;
		this->arg2 = arg2;
	}

	const string & operator[](int index) {
		if (index == 0) {
			return cmd;
		}
		else if (index == 1) {
			return arg1;
		}
		else{
			return arg2;

		}
	}
};

/*
解释程序
*/
class HJvm :public QObject{
  Q_OBJECT
signals:
  void finished(string);
private:

	/*
	常量定义
	*/
	typedef long long    registerType;						//寄存器类型，64位
	typedef long long	 stackType;							//栈空间类型,64位
	static const unsigned long     defaultStackSize = 1024;			//默认栈空间大小，1024*8 Byte
	static const registerType GF = 2;
	static const registerType LF = 1;
	static const registerType ZF = 0;
	const registerType N = 0;
	/*
	指令枚举类型，E区别于函数
	*/
	enum INS {
		movE, addE, subE, mulE,
		divE, notE, andE, orE,
		cmpE, callE, retE, popE, 
		pushE, endE, inE, outE, 
		jmpE, jgE, jgeE, jlE,jleE,
		jzE,jnzE, labelE, 
	};

	/*
	 *操作数类型
	 *immediateValue:立即数
	 *registerValue：寄存器
	 *memoryValue：内存地址
	 */
	enum OPERATORTYPE {
		immediateValue,registerValue,memoryValue
	};


	/*
	 * errorType
	 */
	enum ERRORTYPE
	{
		eaxIsImmediateValue
	};

	/*
	 * 栈空间
	 */

	stackType * stack;			//栈
	int     StackSize = defaultStackSize;
	

	
	/*
	 * 存有前端生成的汇编中间代码
	 */
	vector<ASMCode>  intermediateCode;

	/*
	 * 指令代码hashmap
	 */
	unordered_map<string, INS> instructionCodeMap;

	/*
	 * 寄存器hashmap
	 */
	unordered_map<string, registerType > registerMap;

	/*
	 * 标签hashmap
	 */
	unordered_map<string, registerType >labelMap;

	/*
	 * 移动指令
	 * eax：地址和寄存器
	 * ebx：地址和寄存器和立即数
	 */
	void mov(const string & eax, const string & ebx);

	/*
	 * 加法指令
	 * eax：地址和寄存器
	 * ebx：地址和寄存器和立即数
	 */
	void add(const string & eax, const string & ebx);

	/*
	* 减法指令
	* eax：地址和寄存器
	* ebx：地址和寄存器和立即数
	*/
	void sub(const string & eax, const string & ebx);

	/*
	* 乘法指令
	* eax：地址和寄存器
	* ebx：地址和寄存器和立即数
	*/
	void mul(const string & eax, const string & ebx);

	/*
	* 除法指令
	* eax：地址和寄存器
	* ebx：地址和寄存器和立即数
	*/
	void div(const string & eax, const string & ebx);
	
	/*
	 * 取反指令
	 * eax：地址和寄存器
	 */
	void notOp(const string & eax);

	/*
	* 比较指令
	* eax：地址和寄存器
	* ebx：地址和寄存器和立即数
	* 计算eax-ebx
	* 修改寄存器eflag
	*/
	void cmp(const string & eax, const string & ebx);

	/*
	* 调用指令
	* eax：标签
	*/
	void call(const string & eax);

	/*
	* 返回指令
	*/
	void ret(void);

	/*
	* 出栈指令
	* eax：地址和寄存器
	*/
	void pop(const string & eax);

	/*
	* 入栈指令
	* eax：地址和寄存器
	*/
	void push(const string & eax);

	/*
	* 结束指令
	* 程序的结束标志
	*/
	void end();

	/*
	* 输入指令
	* eax：地址和寄存器
	*/
	void in(const string & eax);

	/*
	* 输出指令
	* eax：地址和寄存器和立即数
	*/
	void out(const string & eax);

	/*
	 *跳转指令
	 *无条件跳转到eax所指标签
	 */
	void jmp(const string & eax);

	/*
	*跳转指令
	*大于时跳转到eax所指标签
	*/
	void jg(const string & eax);

	/*
	*跳转指令
	*大于等于时跳转到eax所指标签
	*/
	void jge(const string & eax);

	/*
	*跳转指令
	*小于时跳转到eax所指标签
	*/
	void jl(const string & eax);

	/*
	*跳转指令
	*小于等于时跳转到eax所指标签
	*/
	void jle(const string & eax);

	/*
	*跳转指令
	*等于零时跳转到eax所指标签
	*/
	void jz(const string & eax);

	/*
	*跳转指令
	*不等于零时跳转到eax所指标签
	*/
	void jnz(const string & eax);

	/*
	*标签
	*记录改语句下一条语句的位置
	*/
	void label(const string & eax);

	/*
	* 按位与指令
	* eax：地址和寄存器
	* ebx：地址和寄存器和立即数
	*/
	void andOp(const string & eax, const string & ebx);

	/*
	* 按位或指令
	* eax：地址和寄存器
	* ebx：地址和寄存器和立即数
	*/
	void orOp(const string & eax, const string & ebx);

	/*
	 *获得操作数的类型：
	 *立即数、寄存器、内存
	 */
	OPERATORTYPE getOperatorType(const string & eax);

	/*
	 * arithmeticOperation
	 * 运算过程的封装
	 * type:运算类型
	 * eax:
	 * ebx:
	 */
	void arithmeticOperation(INS type, const string & eax, const string & ebx);

	/*
	 * 执行运算的细节
	 * type:运算类型
	 * eax:
	 * ebx:
	 */
	void exeArithmeticOperation(INS type, registerType &eax, const registerType &ebx);

public:

	void resetCode(vector<ASMCode> &Code){
	  intermediateCode.clear();
	  intermediateCode=Code;
	}

	/*
	 * 进行vm的初始化
	 *intermediateCode:汇编代码
	 *stackSize:栈容量
	 */
	HJvm(vector<ASMCode> &intermediateCode, unsigned stackSize = defaultStackSize);

	/*
	 *释放栈空间
	 *删除stack
	 */
	~HJvm();

	/*
	 *开始执行vm
	 */
	void start();

	/*
	 *封装cout
	 */
	void HJout(const registerType s);

	/*
	 * 封装cin
	 */
	void HJin(registerType &s);

	/*
	 *初始化instructionMap
	 */
	void initInstructionCodeMap();

	/*
	 *初始化registerCodeMap
	 */
	void initRegisterCodeMap();

	/*
	 *错误处理
	 */
	void error(ERRORTYPE);

	registerType getMemoryAddr(const string &addr);
	string output;
#ifdef DEBUG
	/*
	 *输出调试信息
	 */
	void outputExeRes(INS type, const string & eax, registerType first, const string ebx, registerType second, registerType result);
	void outputExeRes(INS type, const string & eax, registerType first, registerType result);
	void outputExeRes(INS type, const string & eax, registerType first);
#endif

};
