//coded by zhu fei (jellyzone@gmail.com), all rights reseverd

#include <iostream>
#include <sstream>

#include <mapbox/variant.hpp>

#include <list>
#include <numeric>
#include <vector>

#include <algorithm>


// common utility function
template <class F, class G>
decltype(auto) comp(F&& f, G&& g)
{
	return [=](auto x) { return f(g(x)); };
}

template< class F > struct Flip {
	F f = F();

	constexpr Flip(F f) : f(std::move(f)) { }

	template< class X, class Y >
	constexpr auto operator () (X&& x, Y&& y)
		-> typename std::result_of< F(Y, X) >::type
	{
		return f(std::forward<Y>(y), std::forward<X>(x));
	}
};




using ParserLabel = std::string;
using ParserError = std::string;
using ParserPosition = std::string;

template <typename TValue, typename TInput>
struct Success {
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
	Parser(std::function<TResult<TToken, TInput>(TInput)> p) : parseFn(p) {}

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

				return TResult <char, std::string>(Success<char, std::string>{std::make_pair(first, remaining)});
			}
			else
			{
				std::stringstream s;
				s << "Expecting \'" << charToMatch << "\'. Got" << "\'" << first << "\' ";
				std::string r = s.str();
				return TResult <char, std::string>(Error{ r });

			}

		}
	};
	return Parser<char, std::string>(parseFn);
}

template <typename TTokena, typename TTokenb, typename TInput>
Parser<TTokenb, TInput>  bindM(Parser<TTokena, TInput> pa, std::function<Parser<TTokenb, TInput>(TTokena)> f)
{

	std::function <TResult<TTokenb, TInput>(TInput)> parseFn =
		[f, pa](TInput input)
	{

		TResult<TTokena, TInput> ret = runOnInput<TTokena, TInput>(pa, input);

		TResult<TTokenb, TInput> _result;

		ret.match(
			[f, &_result](Success<TTokena, TInput> r) {
			TTokena v = r.value.first;
			TInput remaining = r.value.second;
			Parser<TTokenb, TInput> pb = f(v);

			TResult<TTokenb, TInput> result = runOnInput<TTokenb, TInput>(pb, remaining);
			_result = result;
		},


			[&_result](Error e) { _result = (TResult <TTokenb, TInput>(Error{ e })); }
		);

		return _result;

	};

	//std::function <TResult<TTokenb, TInput>(TInput)> parseFn = nullptr;
	Parser<TTokenb, TInput> pb{ parseFn };
	return pb;

}

template<typename TToken, typename TInput>
Parser<TToken, TInput> returnM(TToken v)
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
Parser<std::pair<TTokena, TTokenb>, TInput>  andThen(Parser<TTokena, TInput> pa, Parser<TTokenb, TInput> pb)
{
	using TPair = std::pair<TTokena, TTokenb>;

	return bindM<TTokena, TPair, TInput>(pa, [pb](TTokena paResult)->Parser<TPair, TInput>
	{
		return bindM<TTokenb, TPair, TInput>(pb, [paResult](TTokenb pbResult) -> Parser<TPair, TInput>
		{
			return returnM<std::pair<TTokena, TTokenb>, TInput>(std::make_pair(paResult, pbResult));
		});
	});
}


template<typename TFuna, typename TFunb,  typename TInput>
Parser<TFunb, TInput> applyM(Parser < std::function<TFunb(TFuna)>, TInput> pf, Parser<TFuna, TInput> pa)
{
	using TFUN = std::function <TFunb(TFuna)>;

	return bindM<TFUN, TFunb,TInput>(pf, [pa](TFUN f)->Parser<TFunb, TInput>
		   {
				return bindM<TFuna, TFunb, TInput>(pa, [f](TFuna a)->Parser<TFunb, TInput>
				{
					return returnM<TFunb, TInput>(f(a));
				});
			});

}


template<typename Ta, typename Tb, typename Tc, typename Ti> 
Parser<Tc, Ti> lift2(std::function<Tc(Ta, Tb)> f, Parser<Ta, Ti> xp, Parser<Tb, Ti> yp)
{
	using TCurried2 = std::function<std::function<Tc(Tb)>(Ta)>;

	TCurried2 curriedf = [f](Ta a)->std::function<Tc(Tb)>
	{
		return [f, a](Tb b)->Tc
				 { 
					return f(a, b);
				 };

	};

	Parser<TCurried2, Ti> x = returnM<TCurried2, Ti>(curriedf);

	Parser< std::function <Tc(Tb)>, Ti> p2 = applyM<Ta, std::function <Tc(Tb)>, Ti>(x, xp);

	return applyM<Tb, Tc, Ti>(p2, yp);
}



template <typename TToken, typename TInput>
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


template <typename TToken, typename TInput>
Parser<TToken, TInput> choice(std::list <Parser<TToken, TInput>> parserList)
{
	return std::accumulate(std::next(parserList.begin()), parserList.end(), *parserList.begin(), orElse<TToken, TInput>);
}




template <typename TToken, typename TInput>
Parser<TToken, TInput> anyOf(std::list<char> chlst)
{
	using TLstParser = std::list <Parser <char, TInput>>;
	 
	TLstParser lstParser;
	std::transform(std::begin(chlst), std::end(chlst), std::back_inserter(lstParser), [](char c)->Parser <char, TInput> { return pchar(c); });
	//lstParser = std::accumulate(chlst.begin(), chlst.end(), lstParser, [](TLstParser lstParser, char c)->TLstParser { lstParser.push_back(pchar(c)); return lstParser; });
	return choice<char, TInput>(lstParser);
}



template<typename  TTokena, typename TTokenb, typename TInput>
Parser<TTokenb, TInput> mapM(Parser<TTokena, TInput> pa, std::function<TTokenb(TTokena)> f)
{
	return bindM<TTokena, TTokenb, TInput>(pa, (comp(returnM<TTokenb, TInput>, f)));
}


//Parser<Tc, Ti> lift2(std::function<Tc(Ta, Tb)> f, Parser<Ta, Ti> xp, Parser<Tb, Ti> yp)

template<typename Ta, typename Tb, typename Tc, typename Ti>
std::function< Parser<Tc, Ti>(Parser<Ta, Ti>, Parser<Tb, Ti>)> curriedLift2(std::function<Tc(Ta, Tb)> f)            /*Parser<Ta, Ti> xp, Parser<Tb, Ti> yp*/
{
	std::function< Parser<Tc, Ti>(Parser<Ta, Ti>, Parser<Tb, Ti>)> res = [f](Parser<Ta, Ti> xp, Parser<Tb, Ti> yp)-> Parser<Tc, Ti>
	{
		return lift2<Ta, Tb, Tc, Ti>(f, xp, yp);
	};
	return res;
	
}


template<typename TItem, typename Ti>
Parser<std::list<TItem>, Ti> sequence(std::list<Parser<TItem, Ti>> lstParser)
{
	std::function<std::list<TItem>(TItem, std::list<TItem>)> cons = [](TItem a, std::list<TItem> lst)->std::list<TItem>
	{
		 lst.push_front(a);
		 return lst;
	};

	std::function< Parser<std::list<TItem>, Ti>(Parser<TItem, Ti>, Parser<std::list<TItem>, Ti>)>  curriedCons = curriedLift2<TItem, std::list<TItem>, std::list<TItem>, Ti>(cons);
	
	std::list<TItem> emptylst = {};

	Parser<std::list<TItem>, Ti> liftedEmptyLst{ returnM<std::list<TItem>, Ti>(emptylst) };
	return std::accumulate(lstParser.rbegin(), lstParser.rend(), liftedEmptyLst, Flip( curriedCons));
}


std::list<char> strToCharList(std::string s)
{
	std::list<char> lstchar;
	std::transform(std::begin(s), std::end(s), std::back_inserter(lstchar), [](char c)->char { return c; });
	return lstchar;
}


std::string charListToStr(std::list<char> cl)
{
	std::string s;
	std::transform(std::begin(cl), std::end(cl), std::back_inserter(s), [](char c)->char { return c; });
	return s;
}

template<typename Ti>
Parser<std::string, Ti> pstring(std::string str)
{
	std::list<char> cl = strToCharList(str);
	std::list<Parser<char, Ti>> lpc;
	std::transform(cl.begin(), cl.end(), std::back_inserter(lpc), [](char c)->Parser<char, Ti> {return pchar(c); });

	Parser<std::list<char>, Ti> lstSeqParser =  sequence<char, Ti>(lpc);

	return mapM<std::list<char>, std::string, Ti>(lstSeqParser, charListToStr);

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


	std::list < Parser<char, std::string> >l = { pchar('a'), pchar('b'), pchar('c') , pchar('z') };

	Parser<char, std::string> ll = choice(l);
	TResult<char, std::string> r4 = runOnInput<char, std::string>(choice(l), "azz");
	r4.match([](Success<char, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });


	Parser<int, std::string> lm = mapM<char, int, std::string>(ll, std::function<int(char)>(toupper));
	TResult<int, std::string> r5 = runOnInput<int, std::string>(lm, "azz");
	r5.match([](Success<int, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });

	std::cout << "test anyof" << std::endl;
	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	Parser<char, std::string> parseDigit = anyOf<char, std::string>(lstDigit);
	TResult<char, std::string> r6 = runOnInput<char, std::string>(parseDigit, "1azz");
	r6.match([](Success<char, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });


	std::cout << "parse Three Digit" << std::endl;
	using ThreePair = std::pair < std::pair<char, char>, char>;

	Parser<ThreePair, std::string> parseThreeDigit = andThen(andThen(parseDigit, parseDigit), parseDigit);
	using ThreePair = std::pair < std::pair<char, char>, char>;
	TResult<ThreePair, std::string> r7 = runOnInput<ThreePair, std::string>(parseThreeDigit, "123azz");
	r7.match(
		[](Success<ThreePair, std::string> r) { std::cout << "(" << "(" << r.value.first.first.first << ", " << r.value.first.first.second << ")" << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; }
	);

	//test parseThreeDigitAsStr
	Parser<std::string, std::string> parseThreeDigitAsStr = mapM<ThreePair, std::string, std::string>(parseThreeDigit, 
			[](ThreePair x) -> std::string
				{
					std::stringstream s;
					s << x.first.first << x.first.second << x.second;
					std::string r = s.str();
					return r;
				});

	TResult<std::string, std::string> r8 = runOnInput<std::string, std::string>(parseThreeDigitAsStr, "199876azz");
	r8.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });





	//test applyM
	std::function< std::string (ThreePair)> af = [](ThreePair x) -> std::string
				{
					std::stringstream s;
					s << x.first.first << x.first.second << x.second;
					std::string r = s.str();
					return r;
	};
	using TFUN = std::function< std::string(ThreePair)>;
	Parser<std::string, std::string> ap = applyM<ThreePair, std::string>(returnM<TFUN, std::string>(af), parseThreeDigit);

	TResult<std::string, std::string> r9 = runOnInput<std::string, std::string>(ap, "199876azz");
	r9.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		[](Error e) { std::cout << e.error << std::endl; });




	//test lift2
	std::function< std::string(char a, char b)> al = [](char a, char b) -> std::string
	{
		std::stringstream s;
		s << a << b ;
		std::string r = s.str();
		return r;
	};
	 Parser<std::string, std::string> pParse2char2string = lift2<char, char, std::string>(al, pchar('n'), pchar('b'));

	 TResult<std::string, std::string> r10 = runOnInput<std::string, std::string>(pParse2char2string, "nbgaxbzz");
	 r10.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
		 [](Error e) { std::cout << e.error << std::endl; });





	 {
		 std::cout << "sequence" << std::endl;
		 std::list<Parser<char, std::string>> parsers = std::list<Parser<char, std::string>>{ pchar('a'), pchar('b'), pchar('c') };

		 Parser<std::list<char>, std::string> combined = sequence<char, std::string>(parsers);


		 TResult<std::list<char>, std::string> r11 = runOnInput<std::list<char>, std::string>(combined, "abc1azz");
		 r11.match(

			 [](Success<std::list<char>, std::string> r) { std::cout << "(";

		 std::cout << "(";
		 std::transform(r.value.first.begin(), r.value.first.end(), r.value.first.begin(),
			 [](char c) -> char {  std::cout << c << " "; return c; });
		 std::cout << ")";

		 std::cout << ", " << r.value.second << ")" << std::endl; },

			 [](Error e) { std::cout << e.error << std::endl; });
	 }

	 




	 {
		 std::cout << "test pstring" << std::endl;
	
		 TResult< std::string, std::string> ret = runOnInput< std::string, std::string>(pstring<std::string>("hahaha"), "hahaha1231");
		 ret.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
			 [](Error e) { std::cout << e.error << " "; });
	 }



	 getchar();

	return 0;
}


