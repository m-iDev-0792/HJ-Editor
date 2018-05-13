//
// Created by 何振邦 on 2018/3/19.
//
#pragma once
#include "tokenAnalysiser.h"
#include <memory>
#include <stack>
#ifndef COMPILER_CREATEAST_H
#define COMPILER_CREATEAST_H
//---------------Global Variables and Global Function------------------//
const int wordLen=1;//假定是64位系统，字长为8字节,算了,方便起见改成1
struct tableType{
	map<string,int> constInt;//const类型仅仅是值的简单替换类似#define
	map<string,float> constFloat;
	map<string,string> variablesInt;// 变量名 -> 变量的地址  的映射
	map<string,string> variablesFloat;// 变量名 -> 变量的地址  的映射
	int paraNum=0;
};
struct funcTag{
		string label;
		int returnType;
		bool achieved;
	vector<int> argType;
};
static string globalOutput;
static string globalASMOutput;
static vector<tableType> symTableStack;
static map<string,funcTag> funcTable;//函数名 -> 函数标签   的映射,函数声明的时候分配一个标签,函数名不准相同,没有函数重载
static int curTokPos=0;
static string curToken;//don't forget init curToken!!!   =tokens.at(0).tokenValue;
static int curTokType;
static int lineNum=1;//记录行号,方便返回错误位置
static int tokenNum=0;
static int errorNum=0;
static int labelNum=0;
static int globalVarNum=0;
static string lastLoopEntry;
static string lastLoopExit;
static int IfNum=0;
static int ForNum=0;
static int WhileNum=0;
static int cmpNum=0;
static int curFuncRetNum=0;
static vector<ASMCode> asmCodes;
class BasicAST;
//常量变量声明
//类型的类型
enum TypeType{
	intType=0,
	floatType=1,
	addrIntType=3,
	addrFloatType=4,
	nullType=5,
	returnInt=6,
	returnFloat=7,
	loopBreak=8,
	loopContinue=9
};
enum GenRuleType{
	noRule=0,
	noCode=1,
	noVarAndFuncCall=(1<<1),
	noLoopExit=(1<<2),
	youNeedCleanUpStack=(1<<3),
	expectReturnInt=(1<<4),
	expectReturnFloat=(1<<5),
	expectIntExpr=(1<<6),
	expectFloatExpr=(1<<7),
	expectAddr=(1<<8),
	dontGenRetButEnd=(1<<9)
};
struct Value{
	int intValue;
	float floatValue;
	string addrValue;
	int valueType;// valueType's value is in TypeType      valueType<0 critical error!!!!
};

//---------------Some useful function----------------------------------//
static string getNextToken(){

	if(curTokType==tok_enter){
		tokenNum=0;
		++lineNum;
	}else{
		++tokenNum;
	}

	++curTokPos;
	if(curTokPos<=tokens.size()) {
		curToken = tokens.at(curTokPos).tokenValue;
		curTokType=tokens.at(curTokPos).tokenType;
	}
	else {
		curToken=string("$");
		curTokType=0;//type >= 0 equals a illegal type
		//present no more token available
	}
	return curToken;
}
static int getOpsPrecedence(){
	if(curTokType!=tok_binary_op)return -1;
	else if(curToken==string("=")) return 10;
	else if(curToken==string("&")|curToken==string("|")) return 20;
	else if(curToken==string("<")|curToken==string("<=")) return 30;
	else if(curToken==string(">")|curToken==string(">=")) return 30;
	else if(curToken==string("==")|curToken==string("!=")) return 30;
	else if(curToken==string("+")|curToken==string("-")) return 40;
	else if(curToken==string("*")|curToken==string("/")) return 50;
	else return -1;
}
static unique_ptr<BasicAST> printError(string log){
	++errorNum;
//	cout<<"在第"<<lineNum<<"行 "<<"第"<<tokenNum<<"个符号之后 ";
//	cout<<"错误:"<<log<<endl;
	globalOutput+=string("在第")+to_string(lineNum)+string("行 第")
	    +to_string(tokenNum)+string("个符号之后 ");
	globalOutput+=string("错误:")+log+string("\n");
	return nullptr;
}
static void codegenError(string log){
//	cout<<"错误:"<<log<<endl;
  ++errorNum;
  globalOutput+=string("错误:")+log+string("\n");
}
static void warning(string log){
//	cout<<"警告:"<<log<<endl;
  globalOutput+=string("警告:")+log+string("\n");
}
static void replaceComment(){
	for(auto &t:tokens){
		if(t.tokenType==tok_single_comment){
			t.tokenType=tok_enter;
		}
	}
}
static void printASM(string cmd){
//	cout<<"asm code: "<<cmd<<endl;
  globalASMOutput+=cmd+string("\n");
}
static void genASM(string cmd,string arg1="#",string arg2="#"){
	asmCodes.push_back(ASMCode(cmd,arg1,arg2));
	printASM(cmd+(arg1=="#"?string(""):string(" ")+arg1)+(arg2=="#"?string(""):string(",")+arg2));
}
//--------------Data Structure Define---------------------------------//
class BasicAST{
public:
	virtual ~BasicAST()= default;
	virtual Value codegen(int genRule=0){Value t;return t;};
};
//字面值
class IntNumberAST:public BasicAST{
public:
	int value;
	virtual Value codegen(int genRule=0);
	IntNumberAST()= default;
	IntNumberAST(int v):value(v){}
};
class FloatNumberAST:public BasicAST{
public:
	float value;
	virtual Value codegen(int genRule=0);
	FloatNumberAST()= default;
	FloatNumberAST(float v):value(v){}
};
class UnaryAST:public BasicAST{
public:
	string unaryOp;
	unique_ptr<BasicAST> arg;
	virtual Value codegen(int genRule=0);
	UnaryAST()= default;
	UnaryAST(string uo,unique_ptr<BasicAST> a):unaryOp(uo),arg(move(a)){}
};

class ConstDecAST:public BasicAST{
public:
	string name;
	int type;//should be a TypeType value
	unique_ptr<BasicAST> initValue;
	virtual Value codegen(int genRule=0);
	ConstDecAST()= default;
	ConstDecAST(const string &n,int t,unique_ptr<BasicAST> iv):name(n),type(t),initValue(move(iv)){}
};
class VarDecAST:public BasicAST{
public:
	vector<string> names;
	int type;
	virtual Value codegen(int genRule=0);
	VarDecAST()= default;
	VarDecAST(const vector<string> &n,int t):names(n),type(t){}
};
//常量与变量的使用节点
class ID:public BasicAST{
public:
	string name;
	virtual Value codegen(int genRule=0);
	ID()= default;
	ID(const string &n):name(n){}
};
class StringAST:public  BasicAST{
public:
	string str;
	StringAST()= default;
	StringAST(const string &s):str(s){}
};
class FuncDeclareAST:public BasicAST{
public:
	string funcName;
	vector<int> argTypes;
	vector<string> argNames;
	int returnType;
	virtual Value codegen(int genRule=0);
	FuncDeclareAST()= default;
	FuncDeclareAST(const string &fn,int rp,vector<int> at,vector<string> an):funcName(fn),returnType(rp),argTypes(move(at)),argNames(move(an)){}
};
class FuncDefineAST:public BasicAST{
public:
	unique_ptr<FuncDeclareAST> declare;
	vector<unique_ptr<BasicAST>> funcBody;
	virtual Value codegen(int genRule=0);
	FuncDefineAST()= default;
	FuncDefineAST(unique_ptr<FuncDeclareAST> d,vector<unique_ptr<BasicAST>> fb):declare(move(d)),funcBody(move(fb)){}
};
class FuncCallAST:public BasicAST{
public:
	string funcName;
	vector<unique_ptr<BasicAST>> args;
	virtual Value codegen(int genRule=0);
	FuncCallAST()= default;
	FuncCallAST(const string &n,vector<unique_ptr<BasicAST>> as):funcName(n),args(move(as)){}
};
class BinOpAST:public BasicAST{
public:
	string op;
	unique_ptr<BasicAST> LHS;
	unique_ptr<BasicAST> RHS;
	virtual Value codegen(int genRule=0);
	BinOpAST()= default;
	BinOpAST(const string &o,unique_ptr<BasicAST> l,unique_ptr<BasicAST> r):op(o),LHS(move(l)),RHS(move(r)){}
};
class ReturnAST:public BasicAST{
public:
	unique_ptr<BasicAST> returnValue;
	virtual Value codegen(int genRule=0);
	ReturnAST()= default;
	ReturnAST(unique_ptr<BasicAST> rv):returnValue(move(rv)){}
};
class LoopExitAST:public BasicAST{
public:
	int exitType;
	virtual Value codegen(int genRule=0);
	LoopExitAST()= default;
	LoopExitAST(int et):exitType(et){};
};
class IfAST:public BasicAST{
public:
	unique_ptr<BasicAST> condition;
	vector<unique_ptr<BasicAST>> thenBody;
	vector<unique_ptr<BasicAST>> elseBody;
	virtual Value codegen(int genRule=0);
	IfAST()= default;
	IfAST(unique_ptr<BasicAST> c,vector<unique_ptr<BasicAST>> tb,vector<unique_ptr<BasicAST>> eb):condition(move(c)),thenBody(move(tb)),elseBody(move(eb)){}

};
class WhileAST:public BasicAST{
public:
	unique_ptr<BasicAST> condition;
	vector<unique_ptr<BasicAST>> body;
	virtual Value codegen(int genRule=0);
	WhileAST()= default;
	WhileAST(unique_ptr<BasicAST> c,vector<unique_ptr<BasicAST>> b):condition(move(c)),body(move(b)){}
};
class ForAST:public BasicAST{
public:
	unique_ptr<BasicAST> expr1;
	unique_ptr<BasicAST> expr2;
	unique_ptr<BasicAST> expr3;
	vector<unique_ptr<BasicAST>> body;
	virtual Value codegen(int genRule=0);
	ForAST()= default;
	ForAST(unique_ptr<BasicAST> e1,unique_ptr<BasicAST> e2,unique_ptr<BasicAST> e3,vector<unique_ptr<BasicAST>> b):expr1(move(e1)),expr2(move(e2)),expr3(move(e3)),body(move(b)){}
};
class MainAST:public BasicAST{
public:
	vector<unique_ptr<BasicAST>> mainBody;
	virtual Value codegen(int genRule=0);
	MainAST()= default;
	MainAST(vector<unique_ptr<BasicAST>> mb):mainBody(move(mb)){}
};
//-------------------Parse Function-----------------------//

static unique_ptr<FloatNumberAST> parseFloatNumber(){
	float v=atof(curToken.c_str());
	getNextToken();//eat float number
	return make_unique<FloatNumberAST>(v);
}
static unique_ptr<IntNumberAST> parseIntNumber(){
	int v=atoi(curToken.c_str());
	getNextToken();//eat int number
	return make_unique<IntNumberAST>(v);
}
static unique_ptr<BasicAST> parseExpression();
static unique_ptr<BasicAST> parseParenExpression(){
	getNextToken();
	auto expr=parseExpression();
	if(curTokType!=tok_right_paren){
		return printError("缺少右括号\')\'");
	}
	getNextToken();
	return move(expr);
}
//parse constant variables and function call
static unique_ptr<BasicAST> parseID(){
	//assume that current token is a ID
	string name=curToken;
	getNextToken();//eat ID
	if(curTokType==tok_left_paren){//this is a function call
		vector<unique_ptr<BasicAST>> args;
		getNextToken();//eat (
		if(curTokType!=tok_right_paren) {
			while (true) {
				auto ag = parseExpression();
				if (ag == nullptr) {
					printError("函数参数非法或者缺少参数");
					return nullptr;
				}
				args.push_back(move(ag));

				if (curTokType == tok_right_paren) {
					break;//find a ) ,now exit while code blocks
				} else if (curTokType == tok_comma) {
					getNextToken();//eat ,
				} else {
					printError("函数参数应用 \',\' 隔开");
					return nullptr;
				}
			}
		}
		//now current token must be a )
		getNextToken();//eat )
		return make_unique<FuncCallAST>(name,move(args));

	}else{
		//仅仅是引用一个常量或者变量
		return make_unique<ID>(name);
		//麻烦的向下指针转换,如果 上一行代码不能用就用下面这些代码
		auto p= make_unique<ID>(name);
		BasicAST* temp= dynamic_cast<BasicAST*>(p.get());
		unique_ptr<BasicAST> returnValue;
		if(temp!= nullptr){
			p.release();
			returnValue.reset(temp);
			return returnValue;
		}
	}
}
//ConstDec := const type ID = Expression
static unique_ptr<ConstDecAST> parseConstDec(){
	//assume 'const' is current token
	getNextToken();//eat 'const'
	//then we should get the constant type
	if(curTokType!=tok_type){
		printError("在const之后应有一个常量类型");//
		return nullptr;
	}
	int type=((curToken==string("int"))?0:1);

	getNextToken();//eat type
	//curToken should be a  ID
	if(curTokType!=tok_id){
		printError("应有一个合法的常量名");
		return nullptr;
	}
	string name=curToken;
	getNextToken();//eat ID
	if(curToken!=string("=")){
		printError("常量名后应有 \'=\' 用以初始化常量");//
		return nullptr;
	}
	getNextToken();//eat '='
	auto initValue=parseExpression();
	return make_unique<ConstDecAST>(name,type,move(initValue));
}
// VarDec:= type ID [, ID]*
static unique_ptr<VarDecAST> parseVarDec(){
	//assume current token is a 'type'
	int type=((curToken==string("int"))?0:1);
	getNextToken();//eat type
	//curToken should be a ID
	vector<string> names;
	if(curTokType!=tok_id){
		printError("应有一个合法的变量名");
		return nullptr;
	}
	names.push_back(curToken);
	getNextToken();//eat ID
	while(curTokType!=tok_enter){
		if(curTokType!=tok_comma){
			printError("批量声明变量应用 \',\'隔开");
			return nullptr;
		}
		getNextToken();//eat ,
		if(curTokType!=tok_id){
			printError("应有一个合法的变量名");
			return nullptr;
		}
		names.push_back(curToken);
		getNextToken();//eat ID
	}
	return make_unique<VarDecAST>(move(names),type);
}
static unique_ptr<BasicAST> parseAtomExpression(){
	switch(curTokType){
		case tok_int:
			return parseIntNumber();
		case tok_float:
			return parseFloatNumber();
		case tok_id:
			return parseID();
		case tok_left_paren:
			return parseParenExpression();
		case tok_single_op: {
			auto op = curToken;
			getNextToken();//eat unary operator
			auto arg = parseAtomExpression();
			if (arg == nullptr) {
				printError(string("运算符") + op + string("之后缺少表达式"));
				return nullptr;
			}
			return make_unique<UnaryAST>(op, move(arg));
		}
		default:
			printError(string("无法处理\'")+curToken+string("\' 因为在此处不是一个合法的表达式或者语句"));
			getNextToken();//eat error token
			return nullptr;
	}
}
static unique_ptr<BasicAST> parseBinOpsRHS(int exprPrec,unique_ptr<BasicAST> LHS){
	while(true){
		int tokPrec=getOpsPrecedence();
		if(tokPrec<exprPrec)
			return LHS;
		string binOp=curToken;
		getNextToken();//eat binOp

		//判断运算符右侧的token是否合法
		if(curTokType!=tok_int&&curTokType!=tok_float&&curTokType!=tok_id&&curTokType!=tok_left_paren&&curTokType!=tok_single_op){
			printError(string("运算符")+binOp+string("之后缺少表达式"));
			return LHS;//这样就不会出现连锁错误,如果这样效果不好,删掉这行,用下面一个语句是最稳妥的
			return nullptr;
		}
		auto RHS=parseAtomExpression();
		if(RHS== nullptr)
			return nullptr;

		// Example
		//start->|  binOp  RHS  nextOp
		//       a    +    b     *     c
		int nextPrec=getOpsPrecedence();
		if(tokPrec<nextPrec){
			RHS=parseBinOpsRHS(tokPrec+1,move(RHS));
			if(RHS==nullptr)return nullptr;
		}
		LHS=make_unique<BinOpAST>(binOp,move(LHS),move(RHS));
	}

}
static unique_ptr<BasicAST> parseExpression(){
	auto LHS=parseAtomExpression();
	if(LHS== nullptr)return nullptr;
	else
		return parseBinOpsRHS(0,move(LHS));
}
static unique_ptr<ReturnAST> parseReturn(){
	//assume current token is return
	getNextToken();
	if(curTokType==tok_enter)return make_unique<ReturnAST>(nullptr);
	else
		return make_unique<ReturnAST>(parseExpression());
}
static unique_ptr<BasicAST> parseStatement();
static vector<unique_ptr<BasicAST>> parseStatementBlock();
static unique_ptr<IfAST> parseIf(){
	vector<unique_ptr<BasicAST>> thenBody;
	vector<unique_ptr<BasicAST>> elseBody;
	//assume current token is 'if'
	getNextToken();//eat if
	if(curTokType!=tok_left_paren){
		printError("if后缺少\'(\'");
		return nullptr;
	}
	getNextToken();//eat (
	auto condition=parseExpression();
	if(condition==nullptr){
		printError("缺少条件表达式");
		return nullptr;
	}
	if(curTokType!=tok_right_paren){
		printError("条件表达式后缺少\')\'");
		return nullptr;
	}
	getNextToken();//eat )

	//删除 ) 和 { 直接的回车
	while(curTokType==tok_enter){
		getNextToken();

	}


	if(curTokType!=tok_left_brace){
		printError("缺少\'{\'");
		return nullptr;
	}
	getNextToken();//eat {
	thenBody=parseStatementBlock();
	if(curTokType!=tok_right_brace){
		printError("缺少\'}\'");
		return nullptr;
	}
	getNextToken();//eat }
	if(curTokType!=tok_else) {
		return make_unique<IfAST>(move(condition), move(thenBody),move(elseBody));
	}
	getNextToken();//eat else

	//删除 else 和 { 直接的回车
	while(curTokType==tok_enter){
		getNextToken();

	}

	if(curTokType!=tok_left_brace){
		printError("缺少\'{\'");
		return nullptr;
	}
	getNextToken();//eat {
	elseBody=parseStatementBlock();
	if(curTokType!=tok_right_brace){
		printError("缺少\'}\'");
		return nullptr;
	}
	getNextToken();//eat }
	return make_unique<IfAST>(move(condition), move(thenBody), move(elseBody));
}
static unique_ptr<WhileAST> parseWhile(){
	//assume current token is while
	getNextToken();//eat while
	if(curTokType!=tok_left_paren){
		printError("if后缺少\'(\'");
		return nullptr;
	}
	getNextToken();//eat (
	auto condition=parseExpression();
	if(condition==nullptr){
		printError("缺少条件表达式");
		return nullptr;
	}
	if(curTokType!=tok_right_paren){
		printError("条件表达式后缺少\')\'");
		return nullptr;
	}
	getNextToken();//eat )

	//删除 ) 和 { 直接的回车
	while(curTokType==tok_enter){
		getNextToken();

	}

	if(curTokType!=tok_left_brace){
		printError("缺少\'{\'");
		return nullptr;
	}
	getNextToken();//eat {
	auto body=parseStatementBlock();
	if(curTokType!=tok_right_brace){
		printError("缺少\'}\'");
		return nullptr;
	}
	getNextToken();//eat }
	return make_unique<WhileAST>(move(condition),move(body));
}
static unique_ptr<ForAST> parseFor(){
	//assume current token is for
	getNextToken();//eat for
	if(curTokType!=tok_left_paren){
		printError("在for之后缺少\'(\'");
		return nullptr;
	}
	getNextToken();//eat (
	auto e1=parseExpression();
	if(e1== nullptr){
		printError("在\'(\'之后缺少表达式");
		return nullptr;
	}
	if(curTokType!=tok_comma){
		printError("在第一表达式后缺少,");
		return nullptr;
	}
	getNextToken();//eat ,

	auto e2=parseExpression();
	if(e2== nullptr){
		printError("在\',\'之后缺少表达式");
		return nullptr;
	}
	if(curTokType!=tok_comma){
		printError("在第二表达式后缺少,");
		return nullptr;
	}
	getNextToken();//eat ,

	auto e3=parseExpression();
	if(e3== nullptr){
		printError("在\',\'之后缺少表达式");
		return nullptr;
	}
	if(curTokType!=tok_right_paren){
		printError("在第三个表达式之后缺少\')\'");
		return nullptr;
	}
	getNextToken();//eat )

	//删除 ) 和 { 直接的回车
	while(curTokType==tok_enter){
		getNextToken();

	}

	if(curTokType!=tok_left_brace){
		printError("\')\'之后缺少\'{\'");
		return nullptr;
	}
	getNextToken();//eat {

	auto body=parseStatementBlock();

	if(curTokType!=tok_right_brace){
		printError("缺少\'}\'");
		return nullptr;
	}
	getNextToken();//eat }
	return make_unique<ForAST>(move(e1),move(e2),move(e3),move(body));
}
static unique_ptr<BasicAST> parseStatement(){
	//语句有 if for while 表达式 变量声明

	//you don't need worry about enter token in this function
	if(curTokType==tok_if){
		auto r=parseIf();
		return move(r);
	}else if(curTokType==tok_for){
		auto r=parseFor();
		return move(r);
	}else if(curTokType==tok_while){
		auto r=parseWhile();
		return move(r);
	}else if(curTokType==tok_type){
		auto r=parseVarDec();
		return move(r);
	}else if(curTokType==tok_const){
		auto r=parseConstDec();
		return move(r);
	}else if(curTokType==tok_continue){
		return make_unique<LoopExitAST>(tok_continue);
	}else if(curTokType==tok_break){
		return make_unique<LoopExitAST>(tok_break);
	}else if(curTokType==tok_return){
		return parseReturn();
	}else{
		auto r=parseExpression();
		return move(r);
	}
}
static vector<unique_ptr<BasicAST>> parseStatementBlock(){//处理回车统一放在这里处理
	//assume last token is {
	vector<unique_ptr<BasicAST>> block;
	while(true){
		if(curTokType==tok_end){
			printError("缺少 } ");
			return move(block);
		}
		if(curTokType==tok_right_brace)break;//end parsing when we meet }
		if(curTokType==tok_enter){

			getNextToken();//eat enter

			continue;//go for a new round
		}
		//在调用parseStatement之前所有的回车已经被处理,所以当到parseStatement时,当前token一定不是回车
		auto b=parseStatement();

		if(b!= nullptr)block.push_back(move(b));
		else {
			//break; //原来是break 但是遇到nullptr说明一条语句解析失败,我们直接跳过这个语句,而不应该直接让statementBlock结束


			//应该continue 当做这条语句没有发生
			continue;
		}
	}
	return move(block);
}
static unique_ptr<BasicAST> parseFunc(){
	//assume current token is 'func'
	getNextToken();//eat 'func'
	if(curTokType!=tok_id){
		printError("\'func\'之后缺少函数名");
		return nullptr;
	}
	string name=curToken;
	getNextToken();//eat function name
	if(curTokType!=tok_left_paren){
		printError(string("函数名")+name+string("之后缺少右括号"));
		return nullptr;
	}
	getNextToken();//eat (
	vector<int> argType;
	vector<string> argName;
	bool needOneMoreArg=false;
	while(curTokType!=tok_right_paren){
		if(curTokType!=tok_type){
			if(needOneMoreArg){
				printError("\',\'后缺少参数");
			}else{
				printError("缺少参数或者\')\'");
			}
			return nullptr;
		}
		int at=((curToken==string("int"))?intType:floatType);
		string typeStr=curToken;
		getNextToken();//eat type
		if(curTokType!=tok_id){
			printError(string("参数类型")+typeStr+string("后缺少参数名"));
			return nullptr;
		}
		string an=curToken;
		argType.push_back(at);
		argName.push_back(an);
		getNextToken();//eat name
		needOneMoreArg=false;
		if(curTokType==tok_right_paren)break;
		if(curTokType==tok_comma){
			getNextToken();
			needOneMoreArg=true;
		}else{
			printError("在一个完整的参数声明之后应有\',\' 来衔接下一个参数,或者\')\'来结束参数输入");
		}
	}
	if(needOneMoreArg){
		printError("\',\'后缺少参数");
		return nullptr;
	}
	getNextToken();//eat )
	int returnType=-1;//-1 == void
	if(curTokType==tok_type){
		returnType=((curToken==string("int"))?intType:floatType);
		getNextToken();//eat type
	}

	int enterMet=0;
	while(curTokType==tok_enter){
		getNextToken();
		++enterMet;

	}


	if((curTokType!=tok_left_brace)&&(enterMet>0)){//回车结束 这是一个函数声明
		return make_unique<FuncDeclareAST>(name,returnType,move(argType),move(argName));
	}else if(curTokType!=tok_left_brace){
		printError("函数声明语句后出现非法字符");
		return nullptr;
	}
	//这是一个函数定义,下面开始解析函数
	//current token is {
	getNextToken();//eat {
	auto v=parseStatementBlock();
	if(curTokType==tok_end){
//		printError("函数体结束缺少}");//you don't need to handle error here cause it's done in parseStatementBlock()
		return nullptr;
	}
	getNextToken();//eat }
	return make_unique<FuncDefineAST>(make_unique<FuncDeclareAST>(name,returnType,move(argType),move(argName)),move(v));
}

static unique_ptr<MainAST> parseMain(){
	vector<unique_ptr<BasicAST>> mainBody;
	while(curTokType!=tok_end){
		if(curTokType==tok_enter){
			getNextToken();//eat enter

			continue;
		}
		if(curTokType==tok_const){
			auto r=parseConstDec();
			mainBody.push_back(move(r));
			continue;
		}else if(curTokType==tok_type){
			auto r=parseVarDec();
			mainBody.push_back(move(r));
			continue;
		}else if(curTokType==tok_func){
			auto r=parseFunc();
			mainBody.push_back(move(r));
			continue;
		}else{
			printError(string("函数外不能出现除函数声明,函数定义,常变量声明外的语句(\'")+curToken+string("\')"));
			getNextToken();//eat some ignore the illegal token
		}
	}
	return make_unique<MainAST>(move(mainBody));
}
//--------------------------Codegen Functions---------------------------------//
Value IntNumberAST::codegen(int genRule) {
	Value r;
	r.intValue=value;
	r.valueType=intType;
	if(youNeedCleanUpStack&genRule)return r;
	if(!(genRule&noCode))
		//printASM(string("push ")+to_string(value));
		genASM("push",to_string(value));
	return r;
}
Value FloatNumberAST::codegen(int genRule) {
	Value r;
	r.floatValue=value;
	r.valueType=floatType;
	if(youNeedCleanUpStack&genRule)return r;
	if(!(genRule&noCode))//printASM(string("push ")+to_string(value));
	genASM("push",to_string(value));
	return r;
}
Value UnaryAST::codegen(int genRule) {
	Value r;
	auto argV=arg->codegen(genRule&(~youNeedCleanUpStack));
	if(argV.valueType==nullType||argV.valueType<0){
		codegenError(string("一元运算符")+unaryOp+string("之后的表达式无法求值"));
		r.valueType=-1;//critical error
		return r;
	}
	if(youNeedCleanUpStack&genRule){
		r.valueType=nullType;
		return r;
	}
	if(argV.valueType==intType){
		if(unaryOp=="!"){
			if(!(genRule&noCode)) {
//				printASM("pop eax");
				genASM("pop","eax");
//				printASM("cmp eax 0");
				genASM("cmp","eax","0");
//				printASM("push ZF");//if eax==0 then ZF ==1 else ZF ==0
				genASM("push","ZF");
			}
			r.intValue=!argV.intValue;
			r.valueType=intType;
		}
	}else if(argV.valueType==floatType){
		if(unaryOp=="!") {
			if(!(genRule&noCode)) {
//				printASM("finit");
//				printASM("fld 0.0");
//				printASM("fld [esp]");
//				printASM("pop eax");//we don't use eax actually
//				printASM("fcompp");
//				printASM("push C3");//if [esp]==0.0 then C3==1 else C3==0

				//handle as int type
// 				printASM("pop eax");
				genASM("pop","eax");
//				printASM("cmp eax 0");
				genASM("cmp","eax","0");
//				printASM("push ZF");//if eax==0 then ZF ==1 else ZF ==0
				genASM("push","ZF");
			}
			r.intValue=!argV.floatValue;
			r.valueType=intType;
		}
	}else if(argV.valueType==addrIntType){//变量和函数,这样的话无法在编译的时候确定值
		if(unaryOp=="!"){
			if(!(genRule&noCode)) {

//				printASM("pop eax");
				genASM("pop","eax");
//				printASM("cmp eax 0");
				genASM("cmp","eax","0");
//				printASM("push ZF");//if eax==0 then ZF ==1 else ZF ==0
				genASM("push","ZF");
				r.intValue=!argV.intValue;
				r.valueType=intType;
			}else{
				r.valueType=-1;//不能在编译时确定值来返回给const类型来初始化
			}
		}
	}else if(argV.valueType==addrFloatType){//变量和函数,这样的话无法在编译的时候确定值
		if(unaryOp=="!") {
			if(!(genRule&noCode)) {
//				printASM("finit");
//				printASM("fld 0.0");
//				printASM("fld [esp]");
//				printASM("pop eax");//we don't use eax actually
//				printASM("fcompp");
//				printASM("push C3");//if [esp]==0.0 then C3==1 else C3==0

//				printASM("pop eax");
				genASM("pop","eax");
//				printASM("cmp eax 0");
				genASM("cmp","eax","0");
//				printASM("push ZF");//if eax==0 then ZF ==1 else ZF ==0
				genASM("push","ZF");
				r.intValue=!argV.floatValue;
				r.valueType=intType;
			}else{
				r.valueType=-1;//不能在编译时确定值来返回给const类型来初始化
			}
		}
	}


	return r;
}
Value ConstDecAST::codegen(int genRule) {//actually we don't generate any code in this function
	int lastIndex=symTableStack.size()-1;
	Value r;
	r.valueType=-1;//首先设置为错误值
	if(lastIndex<0){
		codegenError("符号表栈为空!");
		return r;
	}
	auto &stackTop=symTableStack.at(lastIndex);

	//检查当前栈中是否有同名的常量 变量
	for(auto &id:stackTop.constFloat){
		if(id.first==name){
			codegenError(string("\'")+name+string("\'重定义,已经在浮点型常量中出现"));
			return r;
		}
	}
	for(auto &id:stackTop.constInt){
		if(id.first==name){
			codegenError(string("\'")+name+string("\'重定义,已经在整型常量中出现"));
			return r;
		}
	}
	for(auto &id:stackTop.variablesFloat){
		if(id.first==name){
			codegenError(string("\'")+name+string("\'重定义,已经在浮点型变量中出现"));
			return r;
		}
	}
	for(auto &id:stackTop.variablesInt){
		if(id.first==name){
			codegenError(string("\'")+name+string("\'重定义,已经在整型变量中出现"));
			return r;
		}
	}
	if(type==intType){//整型常量
		auto v=initValue->codegen(noVarAndFuncCall|noCode);
		if(v.valueType==intType){//返回值是int类型
			r.valueType=nullType;
			r.intValue=v.intValue;
			stackTop.constInt[name]=r.intValue;//添加到符号表
		}else if(v.valueType==floatType){//返回值是float类型
			r.valueType=nullType;
			r.intValue=v.floatValue;
			stackTop.constInt[name]=r.intValue;//添加到符号表
			warning("float类型的值被隐式转换为int类型,小数部分将丢失");
		}else{
			codegenError("常量初始化时右值类型不匹配");
		}
		return r;
	}else if(type==floatType){//浮点型常量
		auto v=initValue->codegen(noVarAndFuncCall|noCode);
		if(v.valueType==intType){//返回值是int类型
			r.valueType=nullType;
			r.floatValue=v.intValue;
			stackTop.constFloat[name]=r.floatValue;//添加到符号表
			warning("int类型的值被隐式转换为float类型,大数的低位数值可能丢失");
		}else if(v.valueType==floatType){//返回值是float类型
			r.valueType=nullType;
			r.floatValue=v.floatValue;
			stackTop.constFloat[name]=r.floatValue;//添加到符号表
		}else{
			codegenError("常量初始化时右值类型不匹配");
		}
		return r;
	}

	return r;//this code is mean to remove warning
}
Value VarDecAST::codegen(int genRule) {//we don't need care about gen?
	int lastIndex=symTableStack.size()-1;
	Value r;
	r.valueType=-1;//首先设置为错误值
	auto &stackTop=symTableStack.at(lastIndex);
	if(lastIndex<0){
		codegenError("符号表栈为空!");
		return r;
	}

	//main code
	int counter=stackTop.variablesInt.size()+stackTop.variablesFloat.size()-stackTop.paraNum;
	for(auto name:names){
		++counter;
		//检查当前栈中是否有同名的常量 变量
		for(auto &id:stackTop.constFloat){
			if(id.first==name){
				codegenError(string("\'")+name+string("\'重定义,已经在浮点型常量中出现"));
				return r;
			}
		}
		for(auto &id:stackTop.constInt){
			if(id.first==name){
				codegenError(string("\'")+name+string("\'重定义,已经在整型常量中出现"));
				return r;
			}
		}
		for(auto &id:stackTop.variablesFloat){
			if(id.first==name){
				codegenError(string("\'")+name+string("\'重定义,已经在浮点型变量中出现"));
				return r;
			}
		}
		for(auto &id:stackTop.variablesInt){
			if(id.first==name){
				codegenError(string("\'")+name+string("\'重定义,已经在整型变量中出现"));
				return r;
			}
		}
		if(type==intType){
			if(lastIndex==0) {
				//全局变量
				stackTop.variablesInt[name] = string("[") + to_string(counter * wordLen) + string("]");
			}else{
				//4月12日该成了[ebp+counter
//				printASM("push 0");//局部变量分配空间 初始化为0
				genASM("push","0");
				stackTop.variablesInt[name] = string("[ebp-") + to_string(counter * wordLen) + string("]");
			}
			continue;

		}else if(type==floatType){
//			printASM("push 0");//初始化为0
			genASM("push","0");
			if(lastIndex==0) {
				//全局变量
				stackTop.variablesFloat[name] = string("[") + to_string(counter * wordLen) + string("]");
			}else{
//				printASM("push 0");//局部变量分配空间 初始化为0
				genASM("push","0");
				stackTop.variablesFloat[name] = string("[ebp-") + to_string(counter * wordLen) + string("]");
			}
			continue;
		}else{
			codegenError("未知的变量类型");
			return r;
		}

	}
	r.valueType=nullType;
	return r;
}
Value ID::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	int lastIndex=symTableStack.size()-1;

	for(int i=lastIndex;i>=0;--i){
		auto &curStack=symTableStack.at(i);

		//常量
		for(auto &id:curStack.constFloat){
			if(id.first==name){
				if(genRule&expectAddr){
					codegenError("常量不能作为左值被赋值!");
					return r;
				}
				r.valueType=floatType;
				r.floatValue=id.second;
				if(youNeedCleanUpStack&genRule){
					r.valueType=nullType;
					return r;
				}
				if(!(genRule&noCode)){
//					printASM(string("push ")+to_string(r.intValue)+string("//float常量")+name);
					genASM("push",to_string(r.intValue));
				}

				return r;
			}
		}
		for(auto &id:curStack.constInt){
			if(id.first==name){
				if(genRule&expectAddr){
					codegenError("常量不能作为左值被赋值!");
					return r;
				}
				r.valueType=intType;
				r.intValue=id.second;
				if(youNeedCleanUpStack&genRule){
					r.valueType=nullType;
					return r;
				}
				if(!(genRule&noCode)){
//					printASM(string("push ")+to_string(r.intValue)+string("//int常量")+name);
					genASM("push",to_string(r.intValue));
				}
				return r;
			}
		}
		//变量---返回的是地址
		if(genRule&noVarAndFuncCall){
			codegenError("变量调用生成是不被允许,可能是初始化常量时调用函数产生的");
			return r;
		}
		for(auto &id:curStack.variablesInt){
			if(id.first==name){
				r.valueType=addrIntType;
				r.addrValue=id.second;
				if(youNeedCleanUpStack&genRule){
					if(!(genRule&expectAddr)) {
						r.valueType = nullType;
						return r;
					}
				}
				if(!(genRule&noCode)){
					if(genRule&expectAddr){
						//注意:仅仅返回地址!不产生代码!!!!
						//因为这一定是表达式=左端的变量
						r.valueType=addrIntType;
						return r;
					}else {
						r.valueType=addrIntType;
//						printASM(string("push ") + r.addrValue+string("//int变量")+name);
						genASM("push",r.addrValue);
					}
				}
				return r;
			}
		}
		for(auto &id:curStack.variablesFloat){
			if(id.first==name){
				r.valueType=addrFloatType;
				r.addrValue=id.second;
				if(youNeedCleanUpStack&genRule){
					if(!(genRule&expectAddr)) {
						r.valueType = nullType;
						return r;
					}
				}
				if(!(genRule&noCode)){
					if(genRule&expectAddr){
						//注意:仅仅返回地址!不产生代码!!!!
						//因为这一定是表达式=左端的变量
						r.valueType=addrFloatType;
						return r;
					}else {
						r.valueType=addrFloatType;
//						printASM(string("push ") + r.addrValue+string("//float变量")+name);
						genASM("push",r.addrValue);
					}
				}
				return r;
			}
		}
	}
	codegenError(string("未定义的标识符\'")+name+string("\'"));
	return r;
}
Value FuncDeclareAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	for(auto &id:funcTable){
		if(id.first==funcName){
			codegenError(string("函数名\'")+funcName+string("\'重定义"));
			return r;
		}
	}
	funcTag tag;
	tag.achieved=false;
	tag.argType=argTypes;
	tag.label=string("FUNC_")+funcName;
	tag.returnType=returnType;
	funcTable[funcName]=tag;
	r.valueType=nullType;
	return r;
}
Value FuncDefineAST::codegen(int genRule) {
	Value r;
	curFuncRetNum=0;
	r.valueType=-1;
	auto name=declare->funcName;

	//检测是不是特殊函数
	if(name=="main"){
		if(declare->argNames.size()>0){
			codegenError("main函数不能有参数");
			return r;
		}
	}
	auto search=funcTable.find(name);
	if(search!=funcTable.end()){//表里有函数的声明
		if(search->second.achieved){
			codegenError(string("函数\'")+name+string("\'重复定义"));
			return r;
		}else if(search->second.argType.size()!=declare->argTypes.size()){
			codegenError("函数定义参数个数与函数声明参数个数不匹配");
			return r;
		}
		for(int i=0;i<search->second.argType.size();++i){
			if(search->second.argType.at(i)!=declare->argTypes.at(i)){
				codegenError(string("函数参数中第")+to_string(i+1)+string("个参数类型不匹配"));
				return r;
			}
		}
		search->second.achieved=true;
	}else{//表里没有函数声明
		funcTag tag;
		tag.achieved=true;
		tag.argType=declare->argTypes;

		if(name=="main")tag.label=name;
		else tag.label=string("FUNC_")+name;

		tag.returnType=declare->returnType;
		funcTable[name]=tag;
	}

	if(!(genRule&noCode)){//正式开始生成代码

		//为函数新建一个符号表
		tableType newTable;
		symTableStack.push_back(newTable);
		int stackTopNum=symTableStack.size()-1;
		auto &stackTop=symTableStack.at(stackTopNum);

		//输入形参的地址信息
		for(int i=0;i<declare->argTypes.size();++i){
			++stackTop.paraNum;
			if(declare->argTypes.at(i)==intType){
				stackTop.variablesInt[declare->argNames.at(i)]=string("[ebp+")+to_string((2+i)*wordLen)+string("]");
			}else if(declare->argTypes.at(i)==floatType){
				stackTop.variablesFloat[declare->argNames.at(i)]=string("[ebp+")+to_string((2+i)*wordLen)+string("]");
			}else{
				codegenError(string("函数\'")+name+string("\'第")+to_string(i+1)+string("个参数类型非法"));
				return r;
			}
		}

		//正式开始产生代码
//		printASM(funcTable[name].label+string(":"));
		genASM("label",funcTable[name].label);
		switch (declare->returnType){//确定函数期待的返回类型
			case intType:
				genRule=expectReturnInt|noLoopExit;
				break;
			case floatType:
				genRule=expectReturnFloat|noLoopExit;
				break;
			default:
				genRule=noLoopExit;
		}
		int i=0;
		for(auto &code:funcBody){
			++i;
			Value V;
			if(name=="main"){
			    V=code->codegen(genRule|youNeedCleanUpStack|dontGenRetButEnd);//确定返回值,并 不允许break和continue
			  }else{
			    V=code->codegen(genRule|youNeedCleanUpStack);//确定返回值,并 不允许break和continue
			  }

			if(V.valueType<=4&&V.valueType>=0){//if valueType is intType floatType addrIntType addrFloatType
//				printASM("pop edx //清理栈顶");//清理栈顶
				genASM("pop","edx");
			}
			else if(V.valueType<0){
				codegenError(string("函数")+name+string("的第")+to_string(i)+string("语句编译失败"));
				return r;
			}
		}


		if((genRule&expectReturnFloat)||(genRule&expectReturnInt)){
			if(curFuncRetNum==0&&name!="main"){
				codegenError(string("函数")+name+string("缺少返回值"));
				return r;
			}
		}
		//产生代码结束
		r.valueType=nullType;
		symTableStack.erase(symTableStack.end()-1);//符号表'出栈',函数的符号表不用手动清理局部变量
//		printASM("mov esp,ebp //记得清理了栈顶再ret,这时函数的栈上已经没有任何局部变量了");
		genASM("mov","esp","ebp");
//		printASM("ret");//添加返回语句
		if(name!="main")
		  genASM("ret");
		else
		  genASM("end");
		return r;

	}else{
		codegenError("函数定义AST不接受不生成代码的选项");
	}
	return r;

}
Value FuncCallAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	if(genRule&noVarAndFuncCall){
		codegenError("函数调用生成是不被允许,可能是初始化常量时调用函数产生的");
		return r;
	}

	//先判断是不是特殊函数
	if(funcName=="print"){
		if(args.size()!=1){
			codegenError(string("\'print\'函数只接受一个参数,却有")+to_string(args.size())+string("个"));
			return r;
		}
		for(auto &a:args){
			a->codegen(genRule&(~youNeedCleanUpStack));
		}
//		printASM("pop eax //print函数开始");
		genASM("pop","eax");
//		printASM("out eax //print函数结束");
		genASM("out","eax");
		r.valueType=nullType;
		return r;
	}else if(funcName=="input"){
		if(args.size()!=1){
			codegenError(string("\'input\'函数只接受一个参数,却有")+to_string(args.size())+string("个"));
			return r;
		}
		Value var;
		for(auto &a:args){
			var=a->codegen(genRule|expectAddr);
		}
		if(var.valueType!=addrIntType&&var.valueType!=addrFloatType){
			codegenError("\'input\'函数的参数应当是一个变量");
			return r;
		}
//		printASM(string("in ")+var.addrValue+string(" //input函数"));
		genASM("in",var.addrValue);
		r.valueType=nullType;
		return r;
	}

//	printASM(string("//调用函数")+funcName);
	auto search=funcTable.find(funcName);
	if(search==funcTable.end()){
		codegenError(string("函数\'")+funcName+string("\'未定义,不能被调用"));
		return r;
	}
	auto label=search->second.label;
	auto &argTypes=search->second.argType;
	auto returnValueType=search->second.returnType;
	if(argTypes.size()!=args.size()){
		codegenError(string("函数\'")+funcName+string("\'调用参数个数不匹配"));
		return r;
	}
	//函数的参数进行codegen展开
	for(int i=args.size()-1;i>=0;--i){
		auto &curArg=args.at(i);
		int expectType=((argTypes.at(i)==intType)?expectIntExpr:expectFloatExpr);
		auto v=curArg->codegen(expectType|noLoopExit);//设定参数期待的返回值


		//由于设定了参数期待返回值,
		if(v.valueType==intType||v.valueType==addrIntType){
			if(argTypes.at(i)!=intType){
				codegenError(string("函数\'")+funcName+string("\'的第")+to_string(i+1)+string("个参数类型不匹配,应为float,却是int"));
				return r;
			}
		}else if(v.valueType==floatType||v.valueType==addrFloatType){
			if(argTypes.at(i)!=floatType){
				codegenError(string("函数\'")+funcName+string("\'的第")+to_string(i+1)+string("个参数类型不匹配,应为int,却是float"));
				return r;
			}
		}else if(v.valueType==loopBreak){
			codegenError("此处出现break不合法");
			return r;
		}else if(v.valueType==loopContinue){
			codegenError("此处出现continue不合法");
			return r;
		}else if(v.valueType==returnFloat||v.valueType==returnInt){
			codegenError("此处出现return语句不合法");
			return r;
		}else if(v.valueType==nullType){
			codegenError("需要一个表达式");
			return r;
		}else if(v.valueType<0){
			codegenError("函数语句编译产生错误,已停止编译");
			return r;
		}

	}//参数都已经压完了
//	printASM("push ebp //压入老ebp");
	genASM("push","ebp");
//	printASM("mov ebp,esp //更新ebp");
	genASM("mov","ebp","esp");
//	printASM(string("call ")+label);
	genASM("call",label);
//	printASM("pop ebp //恢复老ebp");
	genASM("pop","ebp");
//	printASM(string("add esp,")+to_string(args.size())+string("//删去函数形参的内存值,函数调用几乎结束了"));//删去函数形参的内存值
	genASM("add","esp",to_string(args.size()));
	if(returnValueType==intType||returnValueType==floatType){
		//有返回值
//		printASM("push eax");//将返回值压入栈顶
		genASM("push","eax");
		r.valueType=search->second.returnType;
	}else{
	    r.valueType=nullType;
	  }

	return r;
}
Value BinOpAST::codegen(int genRule){

	Value lhs;

	if(op=="="){
		lhs=LHS->codegen((genRule|noLoopExit|expectAddr)&(~youNeedCleanUpStack));//如果是赋值表达式,期望左值返回一个地址,并且不向栈内压入左值
	}else{
		lhs=LHS->codegen((genRule|noLoopExit)&(~youNeedCleanUpStack));
	}
	Value rhs=RHS->codegen((genRule|noLoopExit)&(~youNeedCleanUpStack));
	Value r;
	r.valueType=-1;
	if(lhs.valueType<0||rhs.valueType<0){
		codegenError("子表达式编译产生错误,无法继续编译");
		return r;
	}
	//如果genRule 含有noVarAndFuncCall,那么到了这里LHS和RHS均没有变量和函数调用
	//也就是说,如果noCode 那么LHS和RHS一定会没有变量和函数调用
	if(op=="+"){
		if(genRule&noCode){//不用产生代码
			if(lhs.valueType==intType&&rhs.valueType==intType){
				r.valueType=intType;
				r.intValue=lhs.intValue+rhs.intValue;
			}else{
				r.valueType=floatType;
				r.floatValue=((rhs.valueType==intType)?rhs.intValue:rhs.floatValue)+((lhs.valueType==intType)?lhs.intValue:lhs.floatValue);
				warning("int类型的值被隐式转换为float类型,大数的低位数值可能丢失");
			}
			return r;
		}else{

		//生成代码
			//如果两边都是整型的话
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("add eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("add","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				//至少有一边是浮点型
				r.valueType=floatType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("addf eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("addf","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}
	}else if(op=="-"){
		if(genRule&noCode){//不用产生代码
			if(lhs.valueType==intType&&rhs.valueType==intType){
				r.valueType=intType;
				r.intValue=lhs.intValue-rhs.intValue;
			}else{
				r.valueType=floatType;
				r.floatValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)-((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
				warning("int类型的值被隐式转换为float类型,大数的低位数值可能丢失");
			}
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("sub eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("sub","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				//至少有一个参数是float类型
				r.valueType=floatType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("subf eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("subf","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}
	}else if(op=="*"){
		if(genRule&noCode){//不用产生代码
			if(lhs.valueType==intType&&rhs.valueType==intType){
				r.valueType=intType;
				r.intValue=lhs.intValue*rhs.intValue;
			}else{
				r.valueType=floatType;
				r.floatValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)*((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
				warning("int类型的值被隐式转换为float类型,大数的低位数值可能丢失");
			}
			return r;
		}else{
			//生成代码
			//都是整型
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("mul eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("mul","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				//至少有一个浮点参数
				r.valueType=floatType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("mulf eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("mulf","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}
	}else if(op=="/"){
		if(genRule&noCode){//不用产生代码
			if(lhs.valueType==intType&&rhs.valueType==intType){
				r.valueType=intType;
				r.intValue=(int)(lhs.intValue/rhs.intValue);
			}else{
				r.valueType=floatType;
				r.floatValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)/((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
				warning("int类型的值被隐式转换为float类型,大数的低位数值可能丢失");
			}
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("div eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("div","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				//至少有一个参数是浮点数
				r.valueType=floatType;
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("divf eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("divf","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}
	}else if(op=="<"){
		if(genRule&noCode){//不用产生代码
			r.valueType=intType;
			r.intValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)<((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmp eax,ebx");
//				printASM(string("jl ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmp","eax","ebx");
				genASM("jl",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmpf eax,ebx");
//				printASM(string("jl ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmpf","eax","ebx");
				genASM("jl",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}
	}else if(op=="<="){
		if(genRule&noCode){//不用产生代码
			r.valueType=intType;
			r.intValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)<=((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmp eax,ebx");
//				printASM(string("jle ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmp","eax","ebx");
				genASM("jle",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmpf eax,ebx");
//				printASM(string("jle ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmpf","eax","ebx");
				genASM("jle",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}

	}else if(op==">"){
		if(genRule&noCode){//不用产生代码
			r.valueType=intType;
			r.intValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)>((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmp eax,ebx");
//				printASM(string("jg ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmp","eax","ebx");
				genASM("jg",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmpf eax,ebx");
//				printASM(string("jg ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmpf","eax","ebx");
				genASM("jg",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}

	}else if(op==">="){
		if(genRule&noCode){//不用产生代码
			r.valueType=intType;
			r.intValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)>=((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmp eax,ebx");
//				printASM(string("jge ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmp","eax","ebx");
				genASM("jge",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmpf eax,ebx");
//				printASM(string("jge ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmpf","eax","ebx");
				genASM("jge",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}

	}else if(op=="=="){
		if(genRule&noCode){//不用产生代码
			r.valueType=intType;
			r.intValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)==((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmp eax,ebx");
//				printASM(string("jz ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmp","eax","ebx");
				genASM("jz",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmpf eax,ebx");
//				printASM(string("jz ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmpf","eax","ebx");
				genASM("jz",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}

	}else if(op=="!="){
		if(genRule&noCode){//不用产生代码
			r.valueType=intType;
			r.intValue=((lhs.valueType==intType)?lhs.intValue:lhs.floatValue)!=((rhs.valueType==intType)?rhs.intValue:rhs.floatValue);
			return r;
		}else{
			//生成代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmp eax,ebx");
//				printASM(string("jnz ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmp","eax","ebx");
				genASM("jnz",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum);//+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum);//+string(":");
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("cmpf eax,ebx");
//				printASM(string("jnz ")+trueLabel);
//				printASM("push 0");
//				printASM(string("jmp ")+exitLabel);
//				printASM(trueLabel);
//				printASM("push 1");
//				printASM(exitLabel);
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("cmpf","eax","ebx");
				genASM("jnz",trueLabel);
				genASM("push","0");
				genASM("jmp",exitLabel);
				genASM("label",trueLabel);
				genASM("push","1");
				genASM("label",exitLabel);
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}
			return r;
		}

	}else if(op=="&"){
		if(genRule&noCode) {//不用产生代码
			if((lhs.valueType==intType)&&(rhs.valueType==intType)){//TODO. 暂时忽略了为变量的情况(addrType)
				r.valueType=intType;
				r.intValue=lhs.intValue&rhs.intValue;
			}else{
				r.valueType=-1;
				codegenError("进行\'&\'运算的两个参数类型必须都是整型");
				return r;
			}
		}else{
			//产生代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("and eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("and","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}else{
				r.valueType=-1;
				codegenError("进行\'&\'运算的两个参数类型必须都是整型");
				return r;
			}
		}

	}else if(op=="|"){
		if(genRule&noCode) {//不用产生代码
			if((lhs.valueType==intType)&&(rhs.valueType==intType)){//TODO. 暂时忽略了为变量的情况(addrType)
				r.valueType=intType;
				r.intValue=lhs.intValue|rhs.intValue;
			}else{
				r.valueType=-1;
				codegenError("进行\'|\'运算的两个参数类型必须都是整型");
				return r;
			}
		}else{
			//产生代码
			if((lhs.valueType==intType||lhs.valueType==addrIntType)&&(rhs.valueType==intType||rhs.valueType==addrIntType)){
//				printASM("pop ebx");
//				printASM("pop eax");
//				printASM("or eax,ebx");
//				printASM("push eax");
				genASM("pop","ebx");
				genASM("pop","eax");
				genASM("or","eax","ebx");
				genASM("push","eax");
				if(genRule&youNeedCleanUpStack){
//					printASM("pop eax");
					genASM("pop","eax");
				}
			}else{
				r.valueType=-1;
				codegenError("进行\'|\'运算的两个参数类型必须都是整型");
				return r;
			}
		}

	}else if(op=="="){
		if(genRule&noCode){
			r.valueType=-1;
			codegenError("运算符\'=\'不接受不产生代码");
			return r;
		}
		if(lhs.valueType!=addrIntType&&lhs.valueType!=addrFloatType){
			r.valueType=-1;
			codegenError("运算符\'=\'左侧应当是一个变量");
			return r;
		}


		string lhsAddr=lhs.addrValue;
		if(lhs.valueType==addrIntType){//左边是一个整型变量
			if(rhs.valueType==addrIntType||rhs.valueType==intType){
				//右边是一个整型变量,完美
//				printASM("pop eax//弹出=右侧表达式的值");
				genASM("pop","eax");
//				printASM(string("mov ")+lhsAddr+string(" eax"));
				genASM("mov",lhsAddr,"eax");
				r.valueType=nullType;

			}else if(rhs.valueType==addrFloatType||rhs.valueType==floatType){
				//右边是一个浮点型变量,可能要类型转换
				//TODO. 没有类型转换
//				printASM("pop eax//弹出=右侧表达式的值");
				genASM("pop","eax");
//				printASM(string("mov ")+lhsAddr+string(" eax"));
				genASM("mov",lhsAddr,"eax");
				r.valueType=nullType;
			}else{
				//右边是位置的类型,报错
				codegenError("表达式右侧\'=\'值类型应当是一个int类型");
				return r;

			}

		}else{//左边是一个浮点型变量
			if(rhs.valueType==addrFloatType||rhs.valueType==floatType){
				//右边是一个浮点型变量,完美
//				printASM("pop eax//弹出=右侧表达式的值");
				genASM("pop","eax");
//				printASM(string("mov ")+lhsAddr+string(" eax"));
				genASM("mov",lhsAddr,"eax");
				r.valueType=nullType;

			}else if(rhs.valueType==addrIntType||rhs.valueType==intType){
				//右边是一个整型变量,可能要类型转换(其实整型转浮点没有问题)
				//TODO. 没有类型转换
//				printASM("pop eax//弹出=右侧表达式的值");
				genASM("pop","eax");
//				printASM(string("mov ")+lhsAddr+string(" eax"));
				genASM("mov",lhsAddr,"eax");
				r.valueType=nullType;
			}else{
				//右边是位置的类型,报错
				codegenError("表达式右侧\'=\'值类型应当是一个float类型");
				return r;
			}
		}

	}
	return r;
}
Value ReturnAST::codegen(int genRule) {//TODO.codeblocks的子codegen不需要cleanupstack
	Value r;
	r.valueType=-1;

	if(returnValue==nullptr){
		if((genRule&(expectReturnInt|expectReturnFloat))){
			codegenError("return语句之后不应有返回值");
			return r;
		}
		if(!(genRule&noCode)){
//			printASM("mov esp,ebp //即使是return语句也不要忘了清理栈顶");
//			printASM("ret");
			genASM("mov","esp","ebp");

			if(genRule&dontGenRetButEnd)
			  genASM("end");
			else
			  genASM("ret");
			r.valueType=nullType;
		}else{
			codegenError("return语句不接受不产生代码");
		}
		return r;

	}
	Value v=returnValue->codegen(genRule&(~youNeedCleanUpStack));//不需要清理栈顶
	if((genRule&expectReturnInt)){//如果期望int类型
		if(v.valueType==intType||v.valueType==addrIntType){
			if(!(genRule&noCode)){
//				printASM("pop eax");
//				printASM("mov esp,ebp //即使是return语句也不要忘了清理栈顶");
//				printASM("ret");
				genASM("pop","eax");
				genASM("mov","esp","ebp");
				if(genRule&dontGenRetButEnd)
				  genASM("end");
				else
				  genASM("ret");
				++curFuncRetNum;
				r.valueType=nullType;
			}else{
				codegenError("return语句不接受不产生代码");
			}
			return r;
		}else{
			codegenError("返回值不匹配,函数期望int类型");
			return r;
		}

	}else if((genRule&expectReturnFloat)){//如果期望float类型
		if(v.valueType==floatType||v.valueType==addrFloatType){
			if(!(genRule&noCode)){
//				printASM("pop eax");
//				printASM("mov esp,ebp //即使是return语句也不要忘了清理栈顶");
//				printASM("ret");
				genASM("pop","eax");
				genASM("mov","esp","ebp");
				if(genRule&dontGenRetButEnd)
				  genASM("end");
				else
				  genASM("ret");
				++curFuncRetNum;
				r.valueType=nullType;
			}else{
				codegenError("return语句不接受不产生代码");
			}
			return r;
		}else{
			codegenError("返回值不匹配,函数期望float类型");
			return r;
		}
	}
	return r;
}
Value LoopExitAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	if(genRule&noLoopExit){
		codegenError(string("此处不应出现")+((exitType==tok_continue)?string("continue"):string("break"))+string("语句"));
		return r;
	}
	if(genRule&noCode){
		codegenError("循环跳转语句不接受不生成代码");
		return r;
	}
	if(exitType==tok_continue){
//		printASM(string("jmp ")+lastLoopEntry);
		genASM("jmp",lastLoopEntry);
		r.valueType=nullType;
	}else if(exitType==tok_break){
//		printASM(string("jmp ")+lastLoopExit);
		genASM("jmp",lastLoopExit);
		r.valueType=nullType;
	}
	return r;
}
Value IfAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	++IfNum;
	string endLabel=string("IF_END_")+to_string(IfNum);
	string elseLabel=string("IF_ELSE_")+to_string(IfNum);
	if(genRule&noCode){
		codegenError("if语句不接受不产生代码");
		return r;
	}
	auto conditionV=condition->codegen(genRule&(~youNeedCleanUpStack));
	if(conditionV.valueType==-1){
		codegenError("if语句条件表达式代码生成错误");
		return r;
	}
//	printASM("pop eax");
//	printASM("cmp eax, 1");
	genASM("pop","eax");
	genASM("cmp","eax","1");
	if(elseBody.empty())
//		printASM(string("jl ")+endLabel);
		genASM("jl",endLabel);
	else
//		printASM(string("jl ")+elseLabel);
		genASM("jl",elseLabel);
	//为thenBody新建一个符号表(请不要忘了清理局部变量
	tableType newTable;
	symTableStack.push_back(newTable);
	//建立符号表完毕

	for(auto &body:thenBody){
		auto bodyV=body->codegen(genRule|youNeedCleanUpStack);
		if(bodyV.valueType==-1){
			codegenError("If语句执行体内代码生成错误");
			return r;
		}
	}

	//清理局部变量
	auto& stackTop=symTableStack.at(symTableStack.size()-1);
	int varNum=stackTop.variablesInt.size()+stackTop.variablesFloat.size();
	if(varNum>0){
//		printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
		genASM("add","esp",to_string(varNum));
	}
	symTableStack.erase(symTableStack.end()-1);
	//清理局部变量完毕


//	printASM(string("jmp ")+endLabel);
	genASM("jmp",endLabel);

	if(!elseBody.empty()){
		//为elseBody新建一个符号表(请不要忘了清理局部变量
		tableType newTable;
		symTableStack.push_back(newTable);
		//建立符号表完毕

//		printASM(elseLabel+string(":"));
		genASM("label",elseLabel);
		for(auto &body:elseBody){
			auto bodyV=body->codegen(genRule|youNeedCleanUpStack);
			if(bodyV.valueType==-1){
				codegenError("If语句else执行体内代码生成错误");
				return r;
			}
		}
		//清理局部变量
		auto& stackTop=symTableStack.at(symTableStack.size()-1);
		int varNum=stackTop.variablesInt.size()+stackTop.variablesFloat.size();
		if(varNum>0){
//			printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
			genASM("add","esp",to_string(varNum));
		}
		symTableStack.erase(symTableStack.end()-1);
		//清理局部变量完毕
	}
//	printASM(endLabel+string(":"));
	genASM("label",endLabel);
	r.valueType=nullType;
	return r;
}
Value WhileAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	++WhileNum;
	if(genRule&noCode){
		codegenError("while语句不允许不生成代码");
		return r;
	}
	if(genRule&noVarAndFuncCall){
		codegenError("while语句不允许不使用变量和函数调用");
		return r;
	}
	string entryLabel=string("WHILE_ENTRY_")+to_string(WhileNum);
	string endLabel=string("WHILE_END_")+to_string(WhileNum);

	string lastLoopEntryBackUp=lastLoopEntry;
	string lastLoopExitBackUp=lastLoopExit;
	lastLoopEntry=entryLabel;
	lastLoopExit=endLabel;

	//正式开始生成代码
//	printASM(entryLabel+string(":"));
	genASM("label",entryLabel);
	auto conditionV=condition->codegen(genRule&(~youNeedCleanUpStack));
	if(conditionV.valueType==-1){
		codegenError("while语句条件表达式代码生成错误");
		return r;
	}
//	printASM("pop eax");
//	printASM("cmp eax,1");
//	printASM(string("jl ")+endLabel);

	genASM("pop","eax");
	genASM("cmp","eax","1");
	genASM("jl",endLabel);



	//为while循环的body新建一个符号表(请不要忘了清理局部变量
	tableType newTable;
	symTableStack.push_back(newTable);
	//建立符号表完毕

	for(auto& body:this->body){
		auto bodyV=body->codegen(genRule|youNeedCleanUpStack);
		if(bodyV.valueType==-1){
			codegenError("while语句体内代码生成错误");
			return r;
		}
	}
	//清理局部变量
	auto& stackTop=symTableStack.at(symTableStack.size()-1);
	int varNum=stackTop.variablesInt.size()+stackTop.variablesFloat.size();
	if(varNum>0){
//		printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
		genASM("add","esp",to_string(varNum));
	}
	symTableStack.erase(symTableStack.end()-1);
	//清理局部变量完毕





//	printASM(string("jmp ")+entryLabel);
//	printASM(endLabel+string(":"));
	genASM("jmp",entryLabel);
	genASM("label",endLabel);

	//recover lastLoop...
	lastLoopEntry=lastLoopEntryBackUp;
	lastLoopExit=lastLoopExitBackUp;
	r.valueType=nullType;
	return r;

}
Value ForAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	++ForNum;
	if(genRule&noCode){
		codegenError("for语句不允许不生成代码");
		return r;
	}
	if(genRule&noVarAndFuncCall){
		codegenError("for语句不允许不使用变量和函数调用");
		return r;
	}
	string entryLabel=string("FOR_ENTRY_")+to_string(ForNum);
	string endLabel=string("FOR_END_")+to_string(ForNum);

	string lastLoopEntryBackUp=lastLoopEntry;
	string lastLoopExitBackUp=lastLoopExit;
	lastLoopEntry=entryLabel;
	lastLoopExit=endLabel;

	Value expr1V=expr1->codegen(genRule|youNeedCleanUpStack);
	if(expr1V.valueType==-1){
		codegenError("For语句第一个表达式代码生成出现错误");
		return r;
	}
//	printASM(entryLabel+string(":  //for循环条件判断入口"));
	genASM("label",entryLabel);
	Value expr2V=expr2->codegen(genRule&(~youNeedCleanUpStack));
	if(expr2V.valueType==-1){
		codegenError("For语句第二个表达式代码生成出现错误");
		return r;
	}
//	printASM("pop eax");
//	printASM("cmp eax,1 //测试for循环条件真假");
//	printASM(string("jl ")+endLabel+string(" //条件为假,退出循环"));

	genASM("pop","eax");
	genASM("cmp","eax","1");
	genASM("jl",endLabel);


	//为for循环的body新建一个符号表(请不要忘了清理局部变量
	tableType newTable;
	symTableStack.push_back(newTable);
	//建立符号表完毕
	for(auto &body:this->body){
		auto bodyV=body->codegen(genRule|youNeedCleanUpStack);
		if(bodyV.valueType==-1){
			codegenError("For语句体内代码生成错误");
			return r;
		}
	}
	//清理局部变量
	auto& stackTop=symTableStack.at(symTableStack.size()-1);
	int varNum=stackTop.variablesInt.size()+stackTop.variablesFloat.size();
	if(varNum>0){
//		printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
		genASM("add","esp",to_string(varNum));
	}
	symTableStack.erase(symTableStack.end()-1);
	//清理局部变量完毕




	Value expr3V=expr3->codegen(genRule|youNeedCleanUpStack);
	if(expr3V.valueType==-1){
		codegenError("For语句第三个表达式代码生成出现错误");
		return r;
	}
//	printASM(string("jmp ")+entryLabel);
//	printASM(endLabel+string(":  //for循环结束出口"));
	genASM("jmp",entryLabel);
	genASM("label",endLabel);

	//recover lastLoop...
	lastLoopEntry=lastLoopEntryBackUp;
	lastLoopExit=lastLoopExitBackUp;
	r.valueType=nullType;
	return r;

}
Value MainAST::codegen(int genRule) {
	Value r;
	r.valueType=-1;
	int i=0;
	tableType stackFirstData;
	symTableStack.push_back(stackFirstData);
//	printASM("section .text");
//	printASM("global main");
	for(auto &body:this->mainBody){
		++i;
		auto V=body->codegen();
		if(V.valueType==-1){
			codegenError(string("第")+to_string(i)+string("条主语句生成代码产生错误"));
			return r;
		}
	}
	r.valueType=nullType;
	return r;
}



#endif //COMPILER_CREATEAST_H
