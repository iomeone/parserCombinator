// parserCombinator.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//  https://gist.github.com/swlaschin/485f418fede6b6a36d89#file-understanding_parser_combinators-3-fsx

#include <iostream>

#include <mapbox/variant.hpp>
#include <stdexcept>

using ParserLabel = std::string;
using ParserError = std::string;
using ParserPosition = std::string;

template <typename T>
struct Success{
	T token;
};

struct Error {
	 //ParserLabel l;
	 ParserError error;
	 //ParserPosition p;
};


//type Result < 'a> =
//	| Success of 'a
//	| Failure of ParserLabel * ParserError * ParserPosition

template <typename T>
using TResult = mapbox::util::variant<Error, Success<T>>;



//type Parser < 'a> = {
//	parseFn : (Input->Result < 'a * Input>)
//		label:  ParserLabel
//	}

template <typename TToken, typename TInput>
struct Parser
{
	//Parser(std::function<TResult<TToken> (TInput)> p) : parseFn(std::move(p))
	Parser(std::function<TResult<TToken>(TInput)> p) : parseFn(p)
	{
	}

	std::function<TResult<TToken>(TInput)> parseFn;
	//ParserLabel label;
};


//let runOnInput parser input =
//// call inner function with input
//parser.parseFn input

template <typename TToken, typename TInput>
TResult<TToken> runOnInput(Parser<TToken, TInput> parser, TInput t)
{
	return parser.parseFn(t);
}

using TPcharReturn = std::pair<char, std::string>;
Parser<TPcharReturn, std::string> pchar(char charToMatch)
{
	std::function <TResult<TPcharReturn>(std::string)> parseFn =
		[charToMatch](std::string strinput)
		{  
			if (strinput.empty())
				return TResult <TPcharReturn>(Error{ "no more input" });
			else
			{
				char first = strinput.at(0);
				if (charToMatch == first)
				{
					std::string remaining = strinput.substr(1, strinput.length() - 1);
					return TResult <TPcharReturn> (Success<TPcharReturn>{ std::make_pair(first, remaining) });
				}
			}
			
		};

	return Parser<std::pair<char, std::string>, std::string>(parseFn );
}




int main()
{
	Parser<std::pair<char, std::string>, std::string> p = pchar('a');

	TResult<std::pair<char, std::string>> ret = runOnInput<std::pair<char, std::string>, std::string>(p, "aaa");

	ret.match([](Success<std::pair<char, std::string>> r) { std::cout << "(" << r.token.first << ", " << r.token.second << ")" << " "; },
			  [](Error e) { std::cout << e.error << " "; });


	getchar();
 
	return 0;
}


