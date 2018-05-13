//
// Created by 何振邦 on 2018/4/12.
//
#include "createAST.h"
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
