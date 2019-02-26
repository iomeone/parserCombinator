// parserCombinator.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//  https://gist.github.com/swlaschin/485f418fede6b6a36d89#file-understanding_parser_combinators-3-fsx

#include <iostream>

#include <sstream>
#include <mapbox/variant.hpp>
#include <stdexcept>


using ParserLabel = std::string;
using ParserError = std::string;
using ParserPosition = std::string;

template <typename TValue, typename TInput>
struct Success{
	std::pair<TValue, TInput> value;
};

struct Error {
	 //ParserLabel l;
	 ParserError error;
	 //ParserPosition p;
};


//type Result < 'a> =
//	| Success of 'a
//	| Failure of ParserLabel * ParserError * ParserPosition

template <typename TValue, typename TInput>
using TResult = mapbox::util::variant<Error, Success<TValue, TInput>>;



//type Parser < 'a> = {
//	parseFn : (Input->Result < 'a * Input>)
//		label:  ParserLabel
//	}

template <typename TToken, typename TInput>
struct Parser
{
	//Parser(std::function<TResult<TToken> (TInput)> p) : parseFn(std::move(p))
	Parser(std::function<TResult<TToken, TInput>(TInput)> p) : parseFn(p){}

	std::function<TResult<TToken, TInput>(TInput)> parseFn;
	//ParserLabel label;
};


//let runOnInput parser input =
//// call inner function with input
//parser.parseFn input

template <typename TToken, typename TInput>
TResult<TToken, TInput> runOnInput(Parser<TToken, TInput> parser, TInput t)
{
	return parser.parseFn(t);
}


Parser<char, std::string> pchar(char charToMatch)
{
	std::function <TResult<char, std::string>(std::string)> parseFn =
		[charToMatch](std::string strinput)
		{  
			if (strinput.empty())
				return TResult <char, std::string>(Error{ "no more input" });
			else
			{
				char first = strinput.at(0);
				if (charToMatch == first)
				{
					std::string remaining = strinput.substr(1, strinput.length() - 1);
					Success<char, std::string> v;

					return TResult <char, std::string> (Success<char, std::string>{std::make_pair(first, remaining)});
				}
				else
				{
					std::stringstream s; 
					s << "Expecting \'" << charToMatch <<"\'. Got" << "\'"<< first <<"\' ";
					std::string r = s.str();
					return TResult <char, std::string>(Error{  r });
					 
				}
					
			}
		};
	return Parser<char, std::string>(parseFn );
}

template <typename TTokena, typename TTokenb, typename TInput>
Parser<TTokenb, TInput>  bindP(Parser<TTokena, TInput> pa, std::function<Parser<TTokenb, TInput>(TTokena)> f )
{
	using TResultVal = std::pair<TTokena, TTokenb>;


	std::function <TResult<TResultVal, TInput>(TInput)> parseFn =
		[f, pa](TInput input)
	{

		TResult<TTokena, TInput> ret = runOnInput<TTokena, TInput>(pa, input);

		ret.match(
			[](Success<TTokena, TInput> r) {
												TTokena v= ret.token.first;
												TInput remaining = ret.token.second;
												Parser<TTokenb, TInput> pb = f(v);
												
												TResult<TTokenb, TInput> ret = runOnInput<TTokenb, TInput>(pb, remaining);
												return ret;
										  },
				

			[](Error e) { return TResult <TTokenb, TInput>(Error{ e }); }
		);
	};
	Parser<TTokenb, TInput> pb{ parseFn };
	return pb;

}

template<typename TToken, typename TInput>
Parser<TToken, TInput> returnP(TToken v)
{

	std::function <TResult<TToken, TToken>(TInput)> fn =
		[v](TInput input)
	{
		return TResult <TToken>(Success<TToken, TInput>{ std::make_pair(v, input) });
	};
}



template <typename TTokena, typename TTokenb, typename TInput>
Parser<std::pair<TTokena, TTokenb>, TInput>  andThen( Parser<TTokena, TInput> pa, Parser<TTokena, TInput> pb)
{

	using TResultValAB = std::pair<TTokena, TTokenb>;

	bindP(pa, [](TTokena paResult)->std::function<TResult<TTokena, TInput>(TTokena)>
	{
		bindP(pb, [paResult](TTokenb pbResult) -> std::function<TResult<TResultValAB, TInput>(TTokenb)>
		{
			returnP(std::make_pair<paResult, pbResult>);

		});
	});
}


template <typename TToken1, typename TToken2, typename TInput>
Parser<std::pair<TToken1, TToken2>, TInput> andThen(Parser<TToken1, TInput> p1, Parser<TToken2, TInput> p2)
{


	return andThen(p1, p2); // result type is  Parser<std::pair<TToken1, TToken2>, TInput> 
 }

int main()
{
 
 
	Parser<char, std::string> pa = pchar('b');
	Parser<char, std::string> pb = pchar('a');
	TResult<char, std::string> r1 = runOnInput<char, std::string>(pa, "baa");
	r1.match([](Success<char, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << " "; },
			  [](Error e) { std::cout << e.error << " "; });


	using TPairToken = std::pair<char, char>;


	Parser<TPairToken, std::string> pab = andThen(pa, pb);

	TResult<TPairToken, std::string> r2 = runOnInput<TPairToken, std::string>(pab, "aba");

	r2.match([](Success<TPairToken, std::string> r) { std::cout << "(" << r.value.first.first << ", " << r.value.first.second << ")" << " "; },
		[](Error e) { std::cout << e.error << " "; });



	getchar();
 
	return 0;
}


