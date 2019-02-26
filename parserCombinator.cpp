// parserCombinator.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//  https://gist.github.com/swlaschin/485f418fede6b6a36d89#file-understanding_parser_combinators-3-fsx

#include <iostream>
#include <sstream>

#include <mapbox/variant.hpp>

#include <list>
#include <numeric>
#include <vector>

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

	std::function <TResult<TTokenb, TInput>(TInput)> parseFn =
		[f, pa](TInput input)
	{

		TResult<TTokena, TInput> ret = runOnInput<TTokena, TInput>(pa, input);

		TResult<TTokenb, TInput> _result  ;

		ret.match(
			[f, &_result](Success<TTokena, TInput> r) {
												TTokena v= r.value.first;
												TInput remaining = r.value.second;
												Parser<TTokenb, TInput> pb = f(v);
												
												TResult<TTokenb, TInput> result = runOnInput<TTokenb, TInput>(pb, remaining);
												_result = result;
										  },
				

			[&_result](Error e) { _result =( TResult <TTokenb, TInput>(Error{ e })); }
		);

		return _result;
		
	};

	//std::function <TResult<TTokenb, TInput>(TInput)> parseFn = nullptr;
	Parser<TTokenb, TInput> pb{ parseFn };
	return pb;

}

template<typename TToken, typename TInput>
Parser<TToken, TInput> returnP(TToken v)
{

	std::function <TResult<TToken, TInput>(TInput)> fn =
		[v](TInput input)
	{
		return TResult <TToken, TInput>(Success<TToken, TInput>{ std::make_pair(v, input) });
	};

	Parser<TToken, TInput> pr{ fn };

	return pr;
}



template <typename TTokena, typename TTokenb, typename TInput>
Parser<std::pair<TTokena, TTokenb>, TInput>  andThen( Parser<TTokena, TInput> pa, Parser<TTokenb, TInput> pb)
{

	using TPair = std::pair<TTokena, TTokenb>;

	//TTokena a;
	//TTokenb b;
	//auto x = returnP<TPair, TInput>(std::make_pair(a, b));
	//return x;
	return bindP<TTokena, TPair, TInput>(pa, [pb](TTokena paResult)->Parser<TPair, TInput>
			{
				//TTokena a;
				//TTokenb b;
				//auto x =  returnP<TPair, TInput>(std::make_pair(a, b));
				//return x;
				//return returnP<std::pair<TTokena, TTokenb>, TInput>(std::make_pair(paResult, b));

				return bindP<TTokenb, TPair, TInput>(pb, [paResult](TTokenb pbResult) -> Parser<TPair, TInput>
						{
							return returnP<std::pair<TTokena, TTokenb>, TInput>(std::make_pair(paResult, pbResult));
						});
			});
}


template <typename TToken,  typename TInput>
Parser<TToken, TInput> orElse(Parser<TToken, TInput> p1, Parser<TToken, TInput> p2)
{
	std::function <TResult<TToken, TInput>(TInput)> innerFun = [p1, p2](TInput input)
	{
		TResult<TToken, TInput> result = runOnInput<TToken, TInput>(p1, input);



		TResult<TToken, TInput> _result;

		result.match(
					[&_result, result](Success<TToken, TInput> r)
					{
						_result = result;
					},


					[&_result, p2, input](Error e)
					{
						_result = runOnInput<TToken, TInput>(p2, input);
					});
		return _result;
	};

	Parser<TToken, TInput> pr{ innerFun };

	return pr;
}

template <typename Func, typename Container>
auto myreduce(Func f, Container &container)
-> decltype(Container::value_type())
{
	typedef typename Container::value_type ReturnType;
	ReturnType result = ReturnType();
	if (container.begin() != container.end())
	{
		typename Container::const_iterator it;
		it = container.begin();
		for (result = *(it++); it != container.end(); ++it)
		{
			result = f(result, *it);
		}
	}
	return result;
}

template <typename TToken, typename TInput>
Parser<TToken, TInput> choice(std::list <Parser<TToken, TInput>> parserList)
{
	//myreduce(orElse<TToken, TInput>, parserList)
	Parser<TToken, TInput> result = *parserList.begin();;
	if (parserList.begin() != parserList.end())
	{
		
		std::list<Parser<TToken, TInput>>::iterator it = parserList.begin();
		it++;
	
		for ( ;it != parserList.end(); ++it)
		{
			result = orElse(result, *it);
		}
	}
	return result;

}


int main()
{
 
	TResult<char, std::string> r1 = runOnInput<char, std::string>(pchar('b'), "baaaa");
	r1.match([](Success<char, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
			  [](Error e) { std::cout << e.error << " "; });




	using TPairToken = std::pair<char, char>;
	TResult<TPairToken, std::string> r2 = runOnInput<TPairToken, std::string>(andThen(pchar('a'), pchar('b')), "ababb");
	r2.match([](Success<TPairToken, std::string> r) { std::cout << "(" << r.value.first.first << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });



	
	TResult<char, std::string> r3 = runOnInput<char, std::string>(orElse(pchar('a'), pchar('b')), "az");
	r3.match([](Success<char, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });


	std::list < Parser<char, std::string> >l = {pchar('a'), pchar('b'), pchar('c') , pchar('z') };
	TResult<char, std::string> r4 = runOnInput<char, std::string>(choice(l), "zz");
	r4.match([](Success<char, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << " "; });


	getchar();
 
	return 0;
}


