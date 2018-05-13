#include "HJvm.h"

void setTextColor(short x) //自定义函根据参数改变颜色
{
    
}

HJvm::HJvm(vector<ASMCode> &intermediateCode, unsigned stackSize)
{
	this->StackSize = stackSize;
	stack = new stackType[stackSize];				//初始化栈空间

#ifdef DEBUG
	//初始化为0，便于测试
	for(unsigned i = 0;i<stackSize;i++)
	{
		stack[i] = 0;
	}
#endif

	this->intermediateCode = intermediateCode;		//引用赋值

#ifdef OUTPUTCONST
	cout << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << endl;
	cout << endl << "**汇编代码**************" << endl << endl;
	//输出汇编代码
	for (auto line : this->intermediateCode) {
		cout << line[0] << " " << line[1] << " " << line[2] << endl;
	}
#endif // OUTPUTCONST

	initInstructionCodeMap();	
	initRegisterCodeMap();
}


void HJvm::HJout(const registerType s) {
	cout <<"HJout"<<s<<endl;
	output+=to_string(s)+string("\n");
}

void HJvm::HJin(registerType &s)
{
	cin >> s;
}
HJvm::~HJvm()
{
#ifdef DEBUG
	cout << "删除栈空间" << endl;
#endif // DEBUG
	delete[] stack;
}

void HJvm::start()
{
  output.clear();
#ifdef DEBUG
	cout <<endl<< "**解释执行*********" << endl << endl;
#endif // DEBUG

	for (registerMap["ecs"] = 0; registerMap["ecs"] < intermediateCode.size(); registerMap["ecs"]++)
	{
		auto line = intermediateCode[registerMap["ecs"]];		//ASMCode
		const auto  instruction = instructionCodeMap[line[0]];	//INS
		if (instruction == INS::labelE)
		{
			label(line[1]);
		}
	}															//遍历一遍获得标签

#ifdef DEBUG
	//输出标签
	cout << "**所有label********" << endl;
	setTextColor(14);
	for (auto label : labelMap)
	{
		cout << label.first << ":" << label.second << endl;
	}
	setTextColor(7);
	cout << "**label********" << endl;
#endif
	for (registerMap["ecs"] = labelMap["main"]; registerMap["ecs"] < intermediateCode.size(); registerMap["ecs"]++)
	{
		auto line = intermediateCode[registerMap["ecs"]]; //ASMCode
		const auto  instruction = instructionCodeMap[line[0]]; //INS
		switch (instruction) {
		case INS::movE:
			mov(line[1], line[2]);
			break;
		case INS::addE:
			add(line[1], line[2]);
			break;
		case INS::subE:
			sub(line[1], line[2]);
			break;
		case INS::mulE:
			mul(line[1], line[2]);
			break;
		case INS::divE:
			div(line[1], line[2]);
			break;
		case INS::notE:
			notOp(line[1]);
			break;
		case INS::cmpE:
			cmp(line[1], line[2]);
			break;
		case INS::callE:
			call(line[1]);
			break;
		case INS::retE:
			ret();
			break;
		case INS::popE:
			pop(line[1]);
			break;
		case INS::pushE:
			push(line[1]);
			break;
		case INS::endE:
			end();
			break;
		case INS::inE:
			in(line[1]);
			break;
		case INS::outE:
			out(line[1]);
			break;
		case INS::jmpE:
			jmp(line[1]);
			break;
		case INS::jgE:
			jg(line[1]);
			break;
		case INS::jgeE:
			jge(line[1]);
			break;
		case INS::jlE:
			jl(line[1]);
			break;
		case INS::jleE:
			jle(line[1]);
			break;
		case INS::jzE:
			jz(line[1]);
			break;
		case INS::jnzE:
			jnz(line[1]);
			break;
		case INS::andE:
			andOp(line[1], line[2]);
			break;
		case INS::orE:
			orOp(line[1], line[2]);
			break;
		}
	}//for
	emit finished(output);
}

void HJvm::mov(const string & eax, const string & ebx)
{

#ifdef DEBUG
	cout << "mov " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::movE, eax, ebx);

}

void HJvm::add(const string& eax, const string& ebx)
{

#ifdef DEBUG
	cout << "add " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::addE,eax, ebx);

}


void HJvm::sub(const string & eax, const string & ebx)
{

#ifdef DEBUG
	cout << "sub " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::subE, eax, ebx);

}

void HJvm::mul(const string & eax, const string & ebx)
{

#ifdef DEBUG
	cout << "mul " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::mulE, eax, ebx);

}

void HJvm::div(const string & eax, const string & ebx)
{

#ifdef DEBUG
	cout << "div " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::divE, eax, ebx);

}

void HJvm::notOp(const string & eax)
{

#ifdef DEBUG
	cout << "not " << eax << endl;
	registerType first;
#endif // DEBUG

	const auto type = getOperatorType(eax);
	switch(type)
	{
	case OPERATORTYPE::registerValue:

#ifdef DEBUG
		first = registerMap[eax];
#endif

		registerMap[eax] = (registerMap[eax] == 0);

#ifdef DEBUG

		outputExeRes(INS::notE, eax, first, registerMap[eax]);

#endif
		break;

	case OPERATORTYPE::memoryValue:

#ifdef DEBUG
		first = stack[getMemoryAddr(eax.substr(1, eax.size() - 2))];
#endif

		stack[getMemoryAddr(eax.substr(1, eax.size() - 2))] = (stack[getMemoryAddr(eax.substr(1, eax.size() - 2))] == 0);

#ifdef DEBUG
		outputExeRes(INS::notE, eax, first, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
#endif

		break;
	}
}

void HJvm::cmp(const string & eax, const string & ebx)
{

#ifdef DEBUG
	cout << "cmp " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::cmpE, eax, ebx);

}

void HJvm::call(const string & eax)
{

#ifdef DEBUG
	cout << "call " << eax << endl;
	setTextColor(2);
#endif // DEBUG

	push("ecs");
	mov("ebp","esp");
	registerMap["ecs"] = labelMap[eax]-1;

#ifdef DEBUG
	setTextColor(14);
	cout << "-->ecs = " << registerMap["ecs"]+1 << endl;
	setTextColor(7);
#endif

}

void HJvm::ret(void)
{

#ifdef DEBUG
	cout << "ret"<< endl;
	setTextColor(2);
#endif // DEBUG

	pop("ecs");

#ifdef DEBUG
	setTextColor(2);
#endif // DEBUG

	mov("ebp", "esp");
	
}

void HJvm::pop(const string & eax)
{

#ifdef DEBUG
	//todo:防止越界
	cout << "pop " << eax << endl;
#endif // DEBUG
	
	const auto type = getOperatorType(eax);
	switch(type)
	{
	case OPERATORTYPE::registerValue:
		registerMap[eax] = stack[registerMap["esp"]++];

#ifdef DEBUG
		outputExeRes(INS::popE, eax, registerMap[eax]);
#endif

		break;

	case OPERATORTYPE::memoryValue:

		stack[getMemoryAddr(eax.substr(1, eax.size() - 2))] = stack[registerMap["esp"]++];

#ifdef DEBUG
		outputExeRes(INS::popE, eax, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
#endif

		break;
	}
}

void HJvm::push(const string & eax)
{

#ifdef DEBUG
	cout << "push " << eax<< endl;
#endif // DEBUG

	const auto type = getOperatorType(eax);
	switch(type)
	{
	case OPERATORTYPE::registerValue:
		stack[--registerMap["esp"]] = registerMap[eax];

#ifdef DEBUG
		outputExeRes(INS::pushE, eax, registerMap[eax]);
#endif

		break;

	case OPERATORTYPE::memoryValue:
		stack[--registerMap["esp"]] = stack[getMemoryAddr(eax.substr(1, eax.size() - 2))];

#ifdef DEBUG
		outputExeRes(INS::pushE, eax, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
#endif

		break;

	case OPERATORTYPE::immediateValue:
		stack[--registerMap["esp"]] = atoll(eax.c_str());

#ifdef DEBUG
		outputExeRes(INS::pushE, eax, atoll(eax.c_str()));
#endif

		break;
	}
}

void HJvm::end()
{

#ifdef DEBUG
	cout << "end" << endl;
#endif // DEBUG

}

void HJvm::in(const string & eax)
{

#ifdef DEBUG
	cout << "in " << eax<< endl;
#endif // DEBUG

	auto type = getOperatorType(eax);
	switch (type)
	{
	case OPERATORTYPE::registerValue:
		HJin(registerMap[eax]);
		break;

	case OPERATORTYPE::memoryValue:
		HJin(stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
		break;
	}
}

void HJvm::out(const string & eax)
{

#ifdef DEBUG
	cout << "out " << eax << endl;
#endif // DEBUG

	auto type = getOperatorType(eax);
	switch(type)
	{
	case OPERATORTYPE::registerValue:
		HJout(registerMap[eax]);
		break;

	case OPERATORTYPE::memoryValue:
		HJout(stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
		break;

	case OPERATORTYPE::immediateValue:
		HJout(atoll(eax.c_str()));
		break;
	}

}

void HJvm::jmp(const string & eax)
{

#ifdef DEBUG
	cout << "jmp " << eax << endl;
#endif // DEBUG

	registerMap["ecs"] = labelMap[eax] - 1;

}

void HJvm::jg(const string & eax)
{

#ifdef DEBUG
	cout << "jg " << eax << endl;
#endif // DEBUG

	if(registerMap["eflag"] &(1<<GF))
	{
		registerMap["ecs"] = labelMap[eax]-1;
	}
}

void HJvm::jge(const string & eax)
{

#ifdef DEBUG
	cout << "jge " << eax << endl;
#endif // DEBUG

	if (!(registerMap["eflag"] & (1 << LF)))
	{
		registerMap["ecs"] = labelMap[eax] - 1;
	}
}

void HJvm::jl(const string & eax)
{

#ifdef DEBUG
	cout << "jl " << eax << endl;
#endif // DEBUG

	if (registerMap["eflag"] & (1 << LF))
	{
		registerMap["ecs"] = labelMap[eax] - 1;
	}

}

void HJvm::jle(const string & eax)
{

#ifdef DEBUG
	cout << "jle " << eax << endl;
#endif // DEBUG

	if (!(registerMap["eflag"] & (1 << GF)))
	{
		registerMap["ecs"] = labelMap[eax] - 1;
	}
}

void HJvm::jz(const string & eax)
{

#ifdef DEBUG
	cout << "jz " << eax << endl;
#endif // DEBUG

	if (registerMap["eflag"] & (1 << ZF))
	{
		registerMap["ecs"] = labelMap[eax] - 1;
	}
}

void HJvm::jnz(const string & eax)
{

#ifdef DEBUG
	cout << "jnz " << eax << endl;
#endif // DEBUG

	if (!(registerMap["eflag"] & (1 << ZF)))
	{
		registerMap["ecs"] = labelMap[eax] - 1;
	}
}

void HJvm::label(const string & eax)
{

	labelMap[eax] = registerMap["ecs"] + 1;
	
}

void HJvm:: andOp(const string & eax, const string & ebx) 
{

#ifdef DEBUG
	cout << "and " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::andE, eax, ebx);

}
void HJvm::orOp(const string & eax, const string & ebx)
{

#ifdef DEBUG
	cout << "or " << eax << " " << ebx << endl;
#endif // DEBUG

	arithmeticOperation(INS::orE, eax, ebx);
}


HJvm::OPERATORTYPE HJvm::getOperatorType(const string & eax)
{
	if (eax[0] == '[') {
		return OPERATORTYPE::memoryValue;
	}
	else if (registerMap.find(eax) == registerMap.end())
	{
		return OPERATORTYPE::immediateValue;
	}else{
		return OPERATORTYPE::registerValue;
	}
}


void HJvm::initInstructionCodeMap()
{
	instructionCodeMap["mov"] = INS::movE;
	instructionCodeMap["add"] = INS::addE;
	instructionCodeMap["sub"] = INS::subE;
	instructionCodeMap["mul"] = INS::mulE;
	instructionCodeMap["div"] = INS::divE;
	instructionCodeMap["not"] = INS::notE;
	instructionCodeMap["and"] = INS::andE;
	instructionCodeMap["or"] = INS::orE;
	instructionCodeMap["cmp"] = INS::cmpE;
	instructionCodeMap["call"] = INS::callE;
	instructionCodeMap["ret"] = INS::retE;
	instructionCodeMap["pop"] = INS::popE;
	instructionCodeMap["push"] = INS::pushE;
	instructionCodeMap["end"] = INS::endE;
	instructionCodeMap["in"] = INS::inE;
	instructionCodeMap["out"] = INS::outE;
	instructionCodeMap["jmp"] = INS::jmpE;
	instructionCodeMap["jg"] = INS::jgE;
	instructionCodeMap["jge"] = INS::jgeE;
	instructionCodeMap["jl"] = INS::jlE;
	instructionCodeMap["jle"] = INS::jleE;
	instructionCodeMap["jz"] = INS::jzE;
	instructionCodeMap["jnz"] = INS::jnzE;
	instructionCodeMap["label"] = INS::labelE;

#ifdef OUTPUTCONST
	/*
	输出instructionCodeMap
	*/
	cout << "**instructionCodeMap**********:" << endl << endl;
	for (auto pair : instructionCodeMap) {
		cout << pair.first << ":" << pair.second << endl;
	}
	cout << endl;
#endif // OUTPUTCONST

}

void HJvm::initRegisterCodeMap()
{
	registerMap["eax"] = N;
	registerMap["ebx"] = N;
	registerMap["ecx"] = N;
	registerMap["edx"] = N;
	registerMap["esp"] = StackSize;
	registerMap["ebp"] = StackSize;
	registerMap["ecs"] = N;
	registerMap["eflag"] = 0;

#ifdef OUTPUTCONST

	cout << "**registerMap**********:" << endl << endl;
	for (auto pair : registerMap) {
		cout << pair.first << ":" << pair.second << endl;
	}
#endif // OUTPUTCONST

}

void HJvm::error(ERRORTYPE)
{
	/**
	* todo:记录位置：ecs
	*/

#ifdef DEBUG
	setTextColor(4);
	cout <<"eax 为立即数！"<< endl;
	setTextColor(7);
#endif // DEBUG

}

//todo:对浮点型的支持
void HJvm::arithmeticOperation(INS type, const string & eax, const string & ebx)
{
	const auto eaxType = getOperatorType(eax); //OPERATORTYPE
	const auto ebxType = getOperatorType(ebx); //OPERATORTYPE

#ifdef DEBUG
	registerType first;		//保存eax原来的值，用于debug
	registerType second;		//保存eax原来的值，用于debug
#endif

	switch (eaxType)
	{

	case OPERATORTYPE::registerValue:

#ifdef DEBUG
		first = registerMap[eax];
#endif

		switch (ebxType)
		{

			//寄存器+寄存器
		case OPERATORTYPE::registerValue:

#ifdef DEBUG
			second = registerMap[ebx];
#endif

			exeArithmeticOperation(type, registerMap[eax], registerMap[ebx]);

#ifdef DEBUG
			outputExeRes(type, eax, first, ebx, second, registerMap[eax]);
#endif

			break;
			//寄存器+内存
		case OPERATORTYPE::memoryValue:

#ifdef DEBUG
			second = stack[getMemoryAddr(ebx.substr(1, ebx.size() - 2))];
#endif

			exeArithmeticOperation(type, registerMap[eax], stack[getMemoryAddr(ebx.substr(1, ebx.size() - 2))]);

#ifdef DEBUG
			outputExeRes(type, eax, first, ebx, second, registerMap[eax]);
#endif

			break;
			//寄存器+立即数
		case OPERATORTYPE::immediateValue:
			exeArithmeticOperation(type, registerMap[eax], atoll(ebx.c_str()));
			
#ifdef DEBUG
			outputExeRes(type, eax, first, ebx, atoll(ebx.c_str()), registerMap[eax]);
#endif

			break;
		}
		break;

	case OPERATORTYPE::memoryValue:

#ifdef DEBUG
		first = stack[getMemoryAddr(eax.substr(1, eax.size() - 2))];
#endif

		switch (ebxType)
		{
			//内存+寄存器
		case OPERATORTYPE::registerValue:

#ifdef DEBUG
			second = registerMap[ebx];
#endif

			exeArithmeticOperation(type, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))], registerMap[ebx]);
			
#ifdef DEBUG
			outputExeRes(type, eax, first, ebx, second, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
#endif

			break;
			//内存+内存
		case OPERATORTYPE::memoryValue:

#ifdef DEBUG
			second = stack[getMemoryAddr(ebx.substr(1, ebx.size() - 2))];
#endif

			exeArithmeticOperation(type, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))],
				stack[getMemoryAddr(ebx.substr(1, ebx.size() - 2))]);
			
#ifdef DEBUG
			outputExeRes(type, eax, first, ebx, second, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
#endif

			break;
			//内存+立即数
		case OPERATORTYPE::immediateValue:
			exeArithmeticOperation(type, stack[getMemoryAddr(eax.substr(1, eax.size() - 2))],
				atoll(ebx.c_str()));
			
#ifdef DEBUG
			outputExeRes(type, eax, first, ebx, atoll(ebx.c_str()), stack[getMemoryAddr(eax.substr(1, eax.size() - 2))]);
#endif

			break;
		}
		break;

	default:
		error(ERRORTYPE::eaxIsImmediateValue);

	}
}

void HJvm::exeArithmeticOperation(INS type,  registerType &eax, const registerType &ebx)
{
	switch (type)
	{
	case INS::addE:
		eax += ebx;
		break;
	case INS::movE:
		eax = ebx;
		break;
	case INS::subE:
		eax -= ebx;
		break;
	case INS::mulE:
		eax *= ebx;
		break;
	case INS::divE:
		eax /= ebx;
		break;
	case INS::andE:
		eax &= ebx;
		break;
	case INS::orE:
		eax |= ebx;
		break;
	case INS::cmpE:

		registerType res = eax - ebx;
		registerMap["eflag"] =0;
		//设置GF
		if(res > 0)
		{
			registerMap["eflag"] |= ((1 << GF)|registerMap["eflag"]);
		}else
		{
			registerMap["eflag"] &= ((1 << GF) | registerMap["eflag"]);
		}
		//设置LF
		if (res < 0)
		{
			registerMap["eflag"] |= ((1 << LF) | registerMap["eflag"]);
		}
		else
		{
			registerMap["eflag"] &= ((1 << LF) | registerMap["eflag"]);
		}
		//设置ZF
		if (res == 0)
		{
			registerMap["eflag"] |= ((1 << ZF) | registerMap["eflag"]);
		}
		else
		{
			registerMap["eflag"] &= ((1 << ZF) | registerMap["eflag"]);
		}
		break;

	}
}


HJvm::registerType HJvm::getMemoryAddr(const string &addr)
{

	if (registerMap.find(addr) == registerMap.end())
	{
		if(atoll(addr.c_str()))
		{
			return atoll(addr.c_str());
		}else
		{
			registerType offset = atoll(addr.substr(4, addr.size() - 4).c_str());
			string op = addr.substr(3, 1);
			if(op == "+")
			{
				return registerMap[addr.substr(0, 3)] + offset;
			}else
			{
				return registerMap[addr.substr(0, 3)] - offset;
			}
		}
		
	}else
	{
		return registerMap[addr];
		
	}
}

#ifdef DEBUG
void HJvm::outputExeRes(INS type, const string & eax, registerType first, const string ebx, registerType second, registerType result)
{
	setTextColor(14);
	switch(type)
	{
	case INS::movE:
		cout << "-->mov ";
		break;
	case INS::addE:
		cout << "-->add ";
		break;
	case INS::subE:
		cout << "-->sub ";
		break;
	case INS::mulE:
		cout << "-->mul ";
		break;
	case INS::divE:
		cout << "-->div ";
		break;
	case INS::cmpE:
		cout << "-->cmp ";
		break;
	case INS::andE:
		cout << "-->and ";
		break;
	case INS::orE:
		cout << "-->or ";
		break;
	}

	cout << eax << "(" << first << ") "
		<< ebx << "(" << second << ") = "
		<< result << endl;
	setTextColor(7);
}

void HJvm::outputExeRes(INS type, const string & eax, registerType first,  registerType result)
{
	setTextColor(14);
	switch (type)
	{
	case INS::notE:
		cout << "->not ";
	}
	cout << eax << "(" << first << ") = " << result << endl;
	setTextColor(7);
}

void HJvm::outputExeRes(INS type, const string & eax, registerType first)
{
	setTextColor(14);
	switch (type)
	{
	case INS::pushE:
		cout << "-->push ";
		cout << eax << "(" << first << ") = stack[" << registerMap["esp"] << "] (" << stack[registerMap["esp"]] << ")" << endl;
		break;
	case INS::popE:
		cout << "-->pop ";
		cout << eax << " = "<<first << endl;
		break;
	}
	
	setTextColor(7);
}
#endif
