// parserCombinator.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>


#include <mapbox/variant.hpp>
#include <stdexcept>

using ParserLabel = std::string;
using ParserError = std::string;
using ParserPosition = std::string;

template <typename T>
struct Success{
	T a;
};

struct Error {
	 ParserLabel l;
	 ParserError e;
	 ParserPosition p;
};


//type Result < 'a> =
//	| Success of 'a
//	| Failure of ParserLabel * ParserError * ParserPosition

template <typename T>
using Result = mapbox::util::variant<Error, Success<T>>;



//type Parser < 'a> = {
//	parseFn : (Input->Result < 'a * Input>)
//		label:  ParserLabel
//	}

template <typename TResult, typename TInput>

struct Parser
{
	std::function<std::pair<TResult, TInput>(TInput)> parseFn;
	ParserLabel label;
};


//let runOnInput parser input =
//// call inner function with input
//parser.parseFn input

template <typename TResult, typename TInput>
TResult runOnInput(Parser<TResult, TInput> parser, TInput t)
{
	return parser.parseFn(t);
}


int main()
{




    std::cout << "Hello World!\n"; 
	return 0;
}


