#include "createAST.cpp"
//--------------------------Codegen Functions---------------------------------//
Value IntNumberAST::codegen(int genRule) {
	Value r;
	r.intValue=value;
	r.valueType=intType;
	if(youNeedCleanUpStack&genRule)return r;
	if(!(genRule&noCode))printASM(string("push ")+to_string(value));
	return r;
}
Value FloatNumberAST::codegen(int genRule) {
	Value r;
	r.floatValue=value;
	r.valueType=floatType;
	if(youNeedCleanUpStack&genRule)return r;
	if(!(genRule&noCode))printASM(string("push ")+to_string(value));
	return r;
}
Value UnaryAST::codegen(int genRule) {
	Value r;
	auto argV=arg->codegen(genRule);
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
				printASM("pop eax");
				printASM("cmp eax 0");
				printASM("push ZF");//if eax==0 then ZF ==1 else ZF ==0
			}
			r.intValue=!argV.intValue;
			r.valueType=intType;
		}
	}else if(argV.valueType==floatType){
		if(unaryOp=="!") {
			if(!(genRule&noCode)) {
				printASM("finit");
				printASM("fld 0.0");
				printASM("fld [esp]");
				printASM("pop eax");//we don't use eax actually
				printASM("fcompp");
				printASM("push C3");//if [esp]==0.0 then C3==1 else C3==0
			}
			r.intValue=!argV.floatValue;
			r.valueType=intType;
		}
	}else if(argV.valueType==addrIntType){//变量和函数,这样的话无法在编译的时候确定值
		if(unaryOp=="!"){
			if(!(genRule&noCode)) {
				printASM("pop eax");
				printASM("cmp eax 0");
				printASM("push ZF");//if eax==0 then ZF ==1 else ZF ==0
				r.intValue=!argV.intValue;
				r.valueType=intType;
			}else{
				r.valueType=-1;//不能在编译时确定值来返回给const类型来初始化
			}
		}
	}else if(argV.valueType==addrFloatType){//变量和函数,这样的话无法在编译的时候确定值
		if(unaryOp=="!") {
			if(!(genRule&noCode)) {
				printASM("finit");
				printASM("fld 0.0");
				printASM("fld [esp]");
				printASM("pop eax");//we don't use eax actually
				printASM("fcompp");
				printASM("push C3");//if [esp]==0.0 then C3==1 else C3==0
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
				printASM("push 0");//局部变量分配空间 初始化为0
				stackTop.variablesInt[name] = string("[ebp-") + to_string(counter * wordLen) + string("]");
			}
			continue;

		}else if(type==floatType){
			printASM("push 0");//初始化为0
			if(lastIndex==0) {
				//全局变量
				stackTop.variablesFloat[name] = string("[") + to_string(counter * wordLen) + string("]");
			}else{
				printASM("push 0");//局部变量分配空间 初始化为0
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
					printASM(string("push ")+to_string(r.intValue)+string("//float常量")+name);
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
					printASM(string("push ")+to_string(r.intValue)+string("//int常量")+name);
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
						printASM(string("push ") + r.addrValue+string("//int变量")+name);
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
						printASM(string("push ") + r.addrValue+string("//float变量")+name);
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
		printASM(funcTable[name].label+string(":"));
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
			auto V=code->codegen(genRule|youNeedCleanUpStack);//确定返回值,并 不允许break和continue
			if(V.valueType<=4&&V.valueType>=0){//if valueType is intType floatType addrIntType addrFloatType
				printASM("pop edx //清理栈顶");//清理栈顶
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
		printASM("mov esp,ebp //记得清理了栈顶再ret,这时函数的栈上已经没有任何局部变量了");
		printASM("ret");//添加返回语句
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
			a->codegen(genRule);
		}
		printASM("pop eax //print函数开始");
		printASM("out eax //print函数结束");
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
		if(var.valueType!=addrIntType||var.valueType!=addrFloatType){
			codegenError("\'input\'函数的参数应当是一个变量");
			return r;
		}
		printASM(string("in ")+var.addrValue+string(" //input函数"));
		r.valueType=nullType;
		return r;
	}

	printASM(string("//调用函数")+funcName);
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
	printASM("push ebp //压入老ebp");
	printASM("mov ebp,esp //更新ebp");
	printASM(string("call ")+label);
	printASM("pop ebp //恢复老ebp");
	printASM(string("add esp,")+to_string(args.size())+string("//删去函数形参的内存值,函数调用几乎结束了"));//删去函数形参的内存值
	if(returnValueType==intType||returnValueType==floatType){
		//有返回值
		printASM("push eax");//将返回值压入栈顶
	}
	r.valueType=search->second.returnType;
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
				printASM("pop ebx");
				printASM("pop eax");
				printASM("add eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				//至少有一边是浮点型
				r.valueType=floatType;
				printASM("pop ebx");
				printASM("pop eax");
				printASM("addf eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				printASM("pop ebx");
				printASM("pop eax");
				printASM("sub eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				//至少有一个参数是float类型
				r.valueType=floatType;
				printASM("pop ebx");
				printASM("pop eax");
				printASM("subf eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				printASM("pop ebx");
				printASM("pop eax");
				printASM("mul eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				//至少有一个浮点参数
				r.valueType=floatType;
				printASM("pop ebx");
				printASM("pop eax");
				printASM("mulf eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				printASM("pop ebx");
				printASM("pop eax");
				printASM("div eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				//至少有一个参数是浮点数
				r.valueType=floatType;
				printASM("pop ebx");
				printASM("pop eax");
				printASM("divf eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmp eax,ebx");
				printASM(string("jl ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmpf eax,ebx");
				printASM(string("jl ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmp eax,ebx");
				printASM(string("jle ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmpf eax,ebx");
				printASM(string("jle ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmp eax,ebx");
				printASM(string("jg ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmpf eax,ebx");
				printASM(string("jg ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmp eax,ebx");
				printASM(string("jge ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmpf eax,ebx");
				printASM(string("jge ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmp eax,ebx");
				printASM(string("jz ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmpf eax,ebx");
				printASM(string("jz ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmp eax,ebx");
				printASM(string("jnz ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
				}

			}else{
				r.valueType=intType;
				string trueLabel=string("CMP_TRUE_")+to_string(++cmpNum)+string(":");
				string exitLabel=string("CMP_EXIT_")+to_string(cmpNum)+string(":");
				printASM("pop ebx");
				printASM("pop eax");
				printASM("cmpf eax,ebx");
				printASM(string("jnz ")+trueLabel);
				printASM("push 0");
				printASM(string("jmp ")+exitLabel);
				printASM(trueLabel);
				printASM("push 1");
				printASM(exitLabel);
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				printASM("pop ebx");
				printASM("pop eax");
				printASM("and eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				printASM("pop ebx");
				printASM("pop eax");
				printASM("or eax,ebx");
				printASM("push eax");
				if(genRule&youNeedCleanUpStack){
					printASM("pop eax");
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
				printASM("pop eax//弹出=右侧表达式的值");
				printASM(string("mov ")+lhsAddr+string(" eax"));
				r.valueType=nullType;

			}else if(rhs.valueType==addrFloatType||rhs.valueType==floatType){
				//右边是一个浮点型变量,可能要类型转换
				//TODO. 没有类型转换
				printASM("pop eax//弹出=右侧表达式的值");
				printASM(string("mov ")+lhsAddr+string(" eax"));
				r.valueType=nullType;
			}else{
				//右边是位置的类型,报错
				codegenError("表达式右侧\'=\'值类型应当是一个int类型");
				return r;

			}

		}else{//左边是一个浮点型变量
			if(rhs.valueType==addrFloatType||rhs.valueType==floatType){
				//右边是一个浮点型变量,完美
				printASM("pop eax//弹出=右侧表达式的值");
				printASM(string("mov ")+lhsAddr+string(" eax"));
				r.valueType=nullType;

			}else if(rhs.valueType==addrIntType||rhs.valueType==intType){
				//右边是一个整型变量,可能要类型转换(其实整型转浮点没有问题)
				//TODO. 没有类型转换
				printASM("pop eax//弹出=右侧表达式的值");
				printASM(string("mov ")+lhsAddr+string(" eax"));
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
			printASM("mov esp,ebp //即使是return语句也不要忘了清理栈顶");
			printASM("ret");
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
				printASM("pop eax");
				printASM("mov esp,ebp //即使是return语句也不要忘了清理栈顶");
				printASM("ret");
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
				printASM("pop eax");
				printASM("mov esp,ebp //即使是return语句也不要忘了清理栈顶");
				printASM("ret");
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
		printASM(string("jmp ")+lastLoopEntry);
		r.valueType=nullType;
	}else if(exitType==tok_break){
		printASM(string("jmp ")+lastLoopExit);
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
	printASM("pop eax");
	printASM("cmp eax, 1");
	if(elseBody.empty())printASM(string("jl ")+endLabel);
	else printASM(string("jl ")+elseLabel);

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
		printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
	}
	symTableStack.erase(symTableStack.end()-1);
	//清理局部变量完毕


	printASM(string("jmp ")+endLabel);


	if(!elseBody.empty()){
		//为elseBody新建一个符号表(请不要忘了清理局部变量
		tableType newTable;
		symTableStack.push_back(newTable);
		//建立符号表完毕

		printASM(elseLabel+string(":"));
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
			printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
		}
		symTableStack.erase(symTableStack.end()-1);
		//清理局部变量完毕
	}
	printASM(endLabel+string(":"));
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
	printASM(entryLabel+string(":"));
	auto conditionV=condition->codegen(genRule&(~youNeedCleanUpStack));
	if(conditionV.valueType==-1){
		codegenError("while语句条件表达式代码生成错误");
		return r;
	}
	printASM("pop eax");
	printASM("cmp eax,1");
	printASM(string("jl ")+endLabel);



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
		printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
	}
	symTableStack.erase(symTableStack.end()-1);
	//清理局部变量完毕





	printASM(string("jmp ")+entryLabel);
	printASM(endLabel+string(":"));


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
	printASM(entryLabel+string(":  //for循环条件判断入口"));
	Value expr2V=expr2->codegen(genRule&(~youNeedCleanUpStack));
	if(expr2V.valueType==-1){
		codegenError("For语句第二个表达式代码生成出现错误");
		return r;
	}
	printASM("pop eax");
	printASM("cmp eax,1 //测试for循环条件真假");
	printASM(string("jl ")+endLabel+string(" //条件为假,退出循环"));



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
		printASM(string("add esp,")+to_string(varNum)+string(" 一个{}作用域结束,删去局部变量"));
	}
	symTableStack.erase(symTableStack.end()-1);
	//清理局部变量完毕




	Value expr3V=expr3->codegen(genRule|youNeedCleanUpStack);
	if(expr3V.valueType==-1){
		codegenError("For语句第三个表达式代码生成出现错误");
		return r;
	}
	printASM(string("jmp ")+entryLabel);
	printASM(endLabel+string(":  //for循环结束出口"));

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
	printASM("section .text");
	printASM("global main");
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


