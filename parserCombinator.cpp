//coded by zhu fei , MIT licence

#include <iostream>
#include <sstream>
#include <iomanip>

#include <mapbox/variant.hpp>

#include <list>
#include <numeric>
#include <vector>
#include <iterator>


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

template <typename TValue >
struct Success {
	std::pair<TValue, std::string> value;
};

struct Error {
	Error(std::string l, std::string e, std::string p) :label(l), error(e), pos(p)
	{

	}
	Error() 
	{
		label = "unset";
		error = "unset";
		pos = "unset";
	}
	ParserLabel label;
	ParserError  error;
	ParserPosition pos;
};


//type Result < 'a> =
//	| Success of 'a
//	| Failure of ParserLabel * ParserError * ParserPosition

template <typename TValue >
using TResult = mapbox::util::variant<Error, Success<TValue>>;


template<typename Tval>
struct TNothing
{

};

template<typename Tval>
struct TSome
{
	Tval v;
};

//


template <typename TValue>
using Maybe = mapbox::util::variant<TNothing<TValue>, TSome<TValue>>;



template<typename Tval>
std::function<Maybe<Tval>()> Nothing = []()->Maybe<Tval>
{
	return TNothing<Tval>{  };
};



template<typename Tval>
std::function <Maybe<Tval>(Tval)> Some = [](Tval v)->TSome<Tval>
{
	return TSome<Tval>{ v };
};


//type Parser < 'a> = {
//	parseFn : (Input->Result < 'a * Input>)
//		label:  ParserLabel
//	}

template <typename TToken >
struct Parser
{
	//Parser(std::function<TResult<TToken> (TInput)> p) : parseFn(std::move(p))
	Parser(std::function<TResult<TToken>(std::string)> p, ParserLabel l) : parseFn(p),label(l) {}

	std::function<TResult<TToken>(std::string)> parseFn;

	ParserLabel label;
};


//let runOnInput parser input =
//// call inner function with input
//parser.parseFn input

template <typename TToken >
TResult<TToken> runOnInput(Parser<TToken> parser, std::string t)
{
	return parser.parseFn(t);
}


std::string getPrintable(char c) {
	std::string s;
	s.push_back(c);

	if (c == '\n')
		return  "\\n";
	else if (c == '\t')
		return "\\t";
	else if (c == ' ')
		return "\\sapce";
	else if (c == '\r')
		return "\\r";
	else if (isprint(c))
	{
		std::string s;
		s.push_back(c);
		return s;
	}
	else
	{
		std::stringstream ss;
		int i = int(c);
		ss << "\\0x" << std::setfill('0') << std::setw(2) << std::hex << i;

		s = ss.str();
		return s;
	}
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


Parser<char> pchar(char charToMatch)
{
	std::string l;
	l.push_back(charToMatch);
	std::function <TResult<char>(std::string)> parseFn =
		[charToMatch, l](std::string strinput)
	{
		if (strinput.empty())
			return TResult <char>(Error(l, "no more input" , "pos"));
		else
		{
			char first = strinput.at(0);
			if (charToMatch == first)
			{
				std::string remaining = strinput.substr(1, strinput.length() - 1);
				Success<char> v;

				return TResult <char>(Success<char>{std::make_pair(first, remaining)});
			}
			else
			{
				std::stringstream s;

				s << "Unexpected " << getPrintable(first);


				std::string r = s.str();
				return TResult <char>(Error(l, r , "pos"));

			}
		}
	};
	return Parser<char>(parseFn, l);
}

Parser<char> satisfy(std::function<bool(char)> predicate, ParserLabel label)
{
	std::function <TResult<char>(std::string)> parserFn = 
		[predicate, label](std::string input)
	{
		if(input.empty())
			return TResult <char>(Error(label, "no more input" , "pos"));
		else
		{
			char first = input.at(0);
			if (predicate(first))
			{
				std::string remaining = input.substr(1, input.length() - 1);
				Success<char> v;

				return TResult <char>(Success<char>{std::make_pair(first, remaining)});
			}
			else
			{
				std::stringstream s;
				s << "Unexpected \'" << getPrintable(first) ;
				std::string r = s.str();
				return TResult <char>(Error{label, r, "pos" });
			}
		}
	};
	return Parser<char>(parserFn, label);

}




Parser<char> parseChar(char charToMatch)
{
	auto pred = [charToMatch](char ch) -> bool { return charToMatch == ch; };

	return satisfy(pred, ParserLabel{ std::string(1, charToMatch) });
}

Parser<char> parseDigit()
{
	auto pred = [](char charToMatch) -> bool { return isdigit(charToMatch); };

	return satisfy(pred, ParserLabel{ "digit" });
}

Parser<char> parseSpace()
{
	auto pred = [](char charToMatch) -> bool {return isspace(charToMatch); };

	return satisfy(pred, ParserLabel{ "space" });
}



template<typename Tt >
std::string getLabel(Parser<Tt> p)
{
	return p.label;
}


template<typename Tt >
Parser<Tt> setLabel(Parser<Tt> pa, ParserLabel newLabel)
{
	std::function<TResult<Tt>(std::string)> parseFn = [pa, newLabel](std::string input)
	{
		TResult<Tt> ret = runOnInput<Tt>(pa, input);

		TResult<Tt> _result;

		ret.match(
			[&_result](Success<Tt> r) { _result =  r; },

			[&_result, newLabel](Error e) { _result = TResult <Tt>(Error(newLabel, e.error, e.pos)); }
		);

		return _result;

	};

	
	Parser<Tt> pb{ parseFn, newLabel };
	return pb;
}

template <typename TTokena, typename TTokenb >
Parser<TTokenb>  bindM(Parser<TTokena> pa, std::function<Parser<TTokenb>(TTokena)> f)
{
	auto label = "Unknown-bindM";
	std::function <TResult<TTokenb>(std::string)> parseFn =
		[f, pa](std::string input)
	{

		TResult<TTokena> ret = pa.parseFn(input);

		TResult<TTokenb> _result;

		ret.match(
			[f, &_result](Success<TTokena> r) {
					TTokena v = r.value.first;
					std::string remaining = r.value.second;
					Parser<TTokenb> pb = f(v);

					TResult<TTokenb> result = runOnInput<TTokenb>(pb, remaining);
					_result = result;
				},


			[&_result](Error e) { _result = TResult <TTokenb>(Error(e.label, e.error, e.pos)); }
		);

		return _result;

	};

	Parser<TTokenb> pb{ parseFn, label };
	return pb;
}

template<typename TToken >
Parser<TToken> returnM(TToken v)
{
	auto label = "unknown-returnM";
	std::function <TResult<TToken>(std::string)> fn =
		[v](std::string input)
	{
		return TResult <TToken>(Success<TToken>{ std::make_pair(v, input) });
	};

	Parser<TToken> pr{ fn,label };

	return pr;
}



template <typename TTokena, typename TTokenb >
Parser<std::pair<TTokena, TTokenb>>  andThen(Parser<TTokena> pa, Parser<TTokenb> pb)
{
	std::string label = pa.label + " andThen " + pb.label;

	using TPair = std::pair<TTokena, TTokenb>;

	return setLabel(bindM<TTokena, TPair>(pa, [pb](TTokena paResult)->Parser<TPair>
			{
				return bindM<TTokenb, TPair>(pb, [paResult](TTokenb pbResult) -> Parser<TPair>
				{
					return returnM<std::pair<TTokena, TTokenb>>(std::make_pair(paResult, pbResult));
				});
			}), label);
}




template<typename TFuna, typename TFunb >
Parser<TFunb> applyM(Parser < std::function<TFunb(TFuna)>> pf, Parser<TFuna> pa)
{
	using TFUN = std::function <TFunb(TFuna)>;

	return bindM<TFUN, TFunb>(pf, [pa](TFUN f)->Parser<TFunb>
	{
		return bindM<TFuna, TFunb>(pa, [f](TFuna a)->Parser<TFunb>
		{
			return returnM<TFunb>(f(a));
		});
	});

}


template<typename Ta, typename Tb, typename Tc >
Parser<Tc> lift2(std::function<Tc(Ta, Tb)> f, Parser<Ta> xp, Parser<Tb> yp)
{
	using TCurried2 = std::function<std::function<Tc(Tb)>(Ta)>;

	TCurried2 curriedf = [f](Ta a)->std::function<Tc(Tb)>
	{
		return [f, a](Tb b)->Tc
		{
			return f(a, b);
		};

	};

	Parser<TCurried2> x = returnM<TCurried2>(curriedf);

	Parser< std::function <Tc(Tb)>> p2 = applyM<Ta, std::function <Tc(Tb)>>(x, xp);

	return applyM<Tb, Tc>(p2, yp);
}



template <typename TToken >
Parser<TToken> orElse(Parser<TToken> p1, Parser<TToken> p2)
{
	std::string label = p1.label + " orElse " + p2.label;
	std::function <TResult<TToken>(std::string)> innerFun = [p1, p2](std::string input)
	{
		TResult<TToken> result = runOnInput<TToken>(p1, input);



		TResult<TToken> _result;

		result.match(
			[&_result, result](Success<TToken> r)
		{
			_result = result;
		},


			[&_result, p2, input](Error e)
		{
			_result = runOnInput<TToken>(p2, input);
		});
		return _result;
	};

	Parser<TToken> pr{ innerFun, label };

	return pr;
}


template <typename TToken >
Parser<TToken> choice(std::list <Parser<TToken>> parserList)
{
	return std::accumulate(std::next(parserList.begin()), parserList.end(), *parserList.begin(), orElse<TToken>);
}




template <typename TToken >
Parser<TToken> anyOf(std::list<char> chlst)
{
	std::string cs = "anyOf " + charListToStr(chlst);


	using TLstParser = std::list <Parser <char>>;

	TLstParser lstParser;
	std::transform(std::begin(chlst), std::end(chlst), std::back_inserter(lstParser), [](char c)->Parser <char> { return pchar(c); });
	//lstParser = std::accumulate(chlst.begin(), chlst.end(), lstParser, [](TLstParser lstParser, char c)->TLstParser { lstParser.push_back(pchar(c)); return lstParser; });
	return setLabel( choice<char>(lstParser), cs);
}



template<typename  TTokena, typename TTokenb >
Parser<TTokenb> mapM(Parser<TTokena> pa, std::function<TTokenb(TTokena)> f)
{
	return bindM<TTokena, TTokenb>(pa, (comp(returnM<TTokenb>, f)));
}


//Parser<Tc> lift2(std::function<Tc(Ta, Tb)> f, Parser<Ta> xp, Parser<Tb> yp)

template<typename Ta, typename Tb, typename Tc >
std::function< Parser<Tc>(Parser<Ta>, Parser<Tb>)> curriedLift2(std::function<Tc(Ta, Tb)> f)            /*Parser<Ta> xp, Parser<Tb> yp*/
{
	std::function< Parser<Tc>(Parser<Ta>, Parser<Tb>)> res = [f](Parser<Ta> xp, Parser<Tb> yp)-> Parser<Tc>
	{
		return lift2<Ta, Tb, Tc>(f, xp, yp);
	};
	return res;

}


template<typename TItem >
Parser<std::list<TItem>> sequence(std::list<Parser<TItem>> lstParser)
{
	std::function<std::list<TItem>(TItem, std::list<TItem>)> cons = [](TItem a, std::list<TItem> lst)->std::list<TItem>
	{
		lst.push_front(a);
		return lst;
	};

	std::function< Parser<std::list<TItem>>(Parser<TItem>, Parser<std::list<TItem>>)>  curriedCons = curriedLift2<TItem, std::list<TItem>, std::list<TItem>>(cons);

	std::list<TItem> emptylst = {};

	Parser<std::list<TItem>> liftedEmptyLst{ returnM<std::list<TItem>>(emptylst) };
	return std::accumulate(lstParser.rbegin(), lstParser.rend(), liftedEmptyLst, Flip(curriedCons));
}





Parser<std::string> pstring(std::string str)
{
	std::list<char> cl = strToCharList(str);
	std::list<Parser<char>> lpc;
	std::transform(cl.begin(), cl.end(), std::back_inserter(lpc), [](char c)->Parser<char> {return pchar(c); });

	Parser<std::list<char>> lstSeqParser = sequence<char>(lpc);

	return mapM<std::list<char>, std::string>(lstSeqParser, charListToStr);

}
//std::function <TResult<TTokenb>(TInput)> parseFn =
template<typename TItem >
TResult<std::list<TItem>> parserZeroOrMore(Parser<TItem> parser, std::string input)
{

	std::list<TItem> lstRes;
	std::string remainingInput;
	TResult<std::list<TItem>> firstErr;


	TResult<TItem> ret = runOnInput<TItem>(parser, input);

	ret.match(
		[&lstRes, &remainingInput](Success<TItem> r) { lstRes.push_back(r.value.first); remainingInput = r.value.second; },
		[&firstErr](Error e) { firstErr = e; });



	if (lstRes.size() == 1)
	{
		for (; ; )
		{
			bool shouldBreak = false;
			TResult< TItem> ret = runOnInput<TItem>(parser, remainingInput);
			ret.match(
				[&remainingInput, &lstRes](Success<TItem> r) { remainingInput = r.value.second;  lstRes.push_back(r.value.first); },
				[&shouldBreak](Error e) {  shouldBreak = true; });
			if (shouldBreak) break;
		}

		Success< std::list<TItem>> r = { std::make_pair(lstRes, remainingInput) };
		return TResult<std::list<TItem>>(r);
	}
	else
	{
		Success< std::list<TItem>> r = { std::make_pair(lstRes, input) };
		return TResult<std::list<TItem>>(r);
	}
}



template<typename TItem >
Parser<std::list<TItem>> many(Parser<TItem> parser)
{
	std::string label = "many " + parser.label;
	std::function < TResult<std::list<TItem>>(std::string)> innerFn = [parser](std::string input)->TResult<std::list<TItem>>
	{
		return parserZeroOrMore(parser, input);
	};

	Parser<std::list<TItem>> p{ innerFn , label};;
	return p;
}

template <typename TToken >
Parser<std::list<TToken>> many1(Parser<TToken> p)
{
	std::string label = "many1 " + p.label;
	return setLabel( bindM<TToken, std::list<TToken>>(p, [p](TToken pResult)->Parser<std::list<TToken>>
	{
		return bindM<std::list<TToken>, std::list<TToken>>(many<TToken>(p), [pResult](std::list<TToken> manyLstResult)->Parser<std::list<TToken>>
		{
			manyLstResult.push_front(pResult);
			return returnM<std::list<TToken>>(manyLstResult);
		});
	}), label);
}

Parser<char> whitespaceChar()
{
	std::list<char> lc = { ' ', '\t', '\n', '\r'};
	return anyOf<char>(lc);
}

Parser<std::list<char>> whitespace()
{
	return many<char>(whitespaceChar());
}


Parser<std::list<char>> whitespace1()
{
	return many1<char>(whitespaceChar());
}

template<typename Tval >
Parser<Maybe<Tval>> opt(Parser < Tval> p)
{

	Parser<Maybe<Tval>> someParser = mapM<Tval, Maybe<Tval>>(p, Some<Tval>);

	Parser<Maybe<Tval>> nothingParser = returnM<Maybe<Tval>>(Nothing<Tval>());

	return orElse(someParser, nothingParser);


}


//Tval is the opt parser result type, also  is the Maybe<Tval> type
Parser<int> pint()
{
	using TpairMaybeLstChar = std::pair<Maybe<char>, std::list<char>>;

	std::function<int(TpairMaybeLstChar)> resultToInt = [](TpairMaybeLstChar pairOfMaybeAndLstChar) -> int
	{
		Maybe<char> sign = pairOfMaybeAndLstChar.first;
		std::list<char>  charList = pairOfMaybeAndLstChar.second;
		std::string charDigits = charListToStr(charList);
		int inted = std::stoi(charDigits);

		sign.match(
			[&inted](TSome<char> something) { inted = -inted; },
			[](TNothing<char> nothing) {}
		);

		return inted;

	};


	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	Parser<char> digit = anyOf<char>(lstDigit);

	using TDigList = std::list<char>;
	Parser<TDigList> digits = many1<char>(digit);

	Parser<Maybe<char>> maybeNegSign = opt<char>(pchar('-'));

	Parser<std::pair<Maybe<char>, TDigList>> strDig = andThen(maybeNegSign, digits);


	Parser<Maybe<char>> negsopt = opt<char>(pchar('-'));;

	using  TCharMaybeChar = std::pair<char, Maybe<char>>;
	Parser<std::pair< Maybe<char>, std::list<char>>> intParser = andThen< Maybe<char>, std::list<char>>(
		negsopt, digits);


	return mapM<std::pair< Maybe<char>, std::list<char>>, int>(intParser, resultToInt);
}


template<typename Tokena, typename Tokenb >
Parser<Tokena> throwRight(Parser<Tokena> pa, Parser<Tokenb> pb)
{
	Parser<std::pair<Tokena, Tokenb>> athenb = andThen(pa, pb);

	return mapM<std::pair<Tokena, Tokenb>, Tokena>(athenb, [](std::pair<Tokena, Tokenb> pairAB)->Tokena
	{
		return pairAB.first;
	});
}

template<typename Tokena, typename Tokenb >
Parser<Tokenb> throwLeft(Parser<Tokena> pa, Parser<Tokenb> pb)
{
	Parser<std::pair<Tokena, Tokenb>> athenb = andThen(pa, pb);

	return mapM<std::pair<Tokena, Tokenb>, Tokenb>(athenb, [](std::pair<Tokena, Tokenb> pairAB)->Tokenb
	{
		return pairAB.second;
	});
}


template<typename a ,typename b, typename c >
Parser<b> between(Parser<a>  pa, Parser<b> pb, Parser<c> pc)
{
	Parser<b> pb_ = throwLeft<a, b>(pa, pb);

	Parser<b> pb__ = throwRight< b, c>(pb_, pc);

	return pb__;
}


template<typename Tignore1, typename b, typename Tignore2 >
Parser<b> ignoreWhitespaceAround(Parser<Tignore1> pa, Parser<b> pb, Parser<Tignore2> pc)
{
	return between<Tignore1, b, Tignore2>(pa, pb, pc);
}



template<typename a, typename b >
Parser<std::list<a>> sepBy1(Parser<a> p, Parser<b> sep)
{
	Parser<a> sepThenP = throwLeft<b, a>(sep, p);

	Parser<std::list<a>> manySepThenP = many<a>(sepThenP);

	Parser<std::pair<a, std::list<a>>> wholepairList = andThen<a, std::list<a>>(p, manySepThenP);

	Parser<std::list<a>> resultArrayParser = mapM< std::pair<a, std::list<a>>, std::list<a>>(wholepairList,
		[](std::pair<a, std::list<a>> pairList)
			{
					std::list<a> l = pairList.second;
					l.push_front(pairList.first);
					return l;
			});

	return resultArrayParser;
}

template<typename a, typename b >
Parser<std::list<a>> sepBy(Parser<a> p, Parser<b> sep)
{
	Parser<std::list<a>> resultArrayParser = sepBy1<a, b>(p, sep);

	std::list<a> emptylist;
	Parser<std::list<a>> resultArrayParser2 = orElse<std::list<a>>(resultArrayParser, returnM<std::list<a>> (emptylist));
	
	return resultArrayParser2;
}







struct TIF
{

};

struct TFOR
{

};


using Keyword = mapbox::util::variant<TIF, TFOR>;


std::function <Keyword()> IF = []()->Keyword
{
	return TIF();
};


std::function<Keyword()> FOR = []()->Keyword
{
	return TFOR();
};








int main()
{

	{
		//using TPairToken = std::pair<char, char>;
		using ThreePair = std::pair< std::pair < std::pair<char, char>, char>, char>;
		auto top = andThen(andThen(andThen(parseChar('a'), parseChar('b')), parseChar('c')), parseChar('d'));

		TResult<ThreePair> r2 = runOnInput<ThreePair>(top, "abczdbddbabb");
		r2.match(
			[](Success<ThreePair> r) { std::cout << "(" /*<< r.value.first.first*/ << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
			[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
		);

	}



	{
		TResult<char> r1 = runOnInput<char>(pchar('b'), "aaaa");

		r1.match(
			[](Success<char> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
			[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });
	}

	{
			using TPairToken = std::pair<char, char>;

			TResult<TPairToken> r2 = runOnInput<TPairToken>(andThen(pchar('a'), pchar('b')), "cdbabb");
			r2.match([](Success<TPairToken> r) { std::cout << "(" << r.value.first.first << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
				[](Error e) {  std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });

	}

	{
		using TPairToken = std::pair<char, char>;
		TResult<TPairToken> r2 = runOnInput<TPairToken>(setLabel(andThen(pchar('A'), pchar('B')), "AB"), "cdbabb");
		r2.match([](Success<TPairToken> r) { std::cout << "(" << r.value.first.first << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
			[](Error e) {  std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });
	}

	{

			std::cout << "test anyof" << std::endl;
			std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
			Parser<char> parseDigit = anyOf<char>(lstDigit);
			parseDigit = setLabel(parseDigit, "digit");
			TResult<char> r6 = runOnInput<char>(parseDigit, "aazz");
			r6.match([](Success<char> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
				[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });

	}


	{
		Parser<Keyword> keyword_IF = mapM<std::pair<char, char>, Keyword>(andThen(pchar('i'), pchar('f')), [](std::pair<char, char>) -> Keyword{return IF();});

		
		Parser<Keyword> keyword_FOR = mapM<std::string, Keyword>(pstring("for"), [](std::string) -> Keyword {return FOR(); });

		std::list<Parser<Keyword>> ks = { keyword_IF, keyword_FOR };


		Parser<Keyword> pkeys = choice<Keyword>(ks);

		pkeys = setLabel<Keyword>(pkeys, "IF-FOR");

		TResult<Keyword> ret = runOnInput<Keyword>(pkeys, "fo1rzz");
		
		ret.match(
			[](Success<Keyword> r) { std::cout << "(" /*<< r.value.first*/ << ", " << r.value.second << ")" << std::endl; },
			[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
		);

	}







	//the following need to modify , do not need std::string template argument



	//{
	//	TResult<char> r1 = runOnInput<char, std::string>(pchar('b'), "baaaa");
	//	r1.match([](Success<char> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << "Error parsing " << e.label << std::endl << e.error << std::endl; });


	//	using TPairToken = std::pair<char, char>;
	//	TResult<TPairToken, std::string> r2 = runOnInput<TPairToken, std::string>(andThen(pchar('a'), pchar('b')), "ababb");
	//	r2.match([](Success<TPairToken, std::string> r) { std::cout << "(" << r.value.first.first << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });


	//	TResult<char> r3 = runOnInput<char, std::string>(orElse(pchar('a'), pchar('b')), "az");
	//	r3.match([](Success<char> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });


	//	std::list < Parser<char> >l = { pchar('a'), pchar('b'), pchar('c') , pchar('z') };

	//	Parser<char> ll = choice(l);
	//	TResult<char> r4 = runOnInput<char, std::string>(choice(l), "azz");
	//	r4.match([](Success<char> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });


	//	Parser<int, std::string> lm = mapM<char, int, std::string>(ll, std::function<int(char)>(toupper));
	//	TResult<int, std::string> r5 = runOnInput<int, std::string>(lm, "azz");
	//	r5.match([](Success<int, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });


	//	std::cout << "test anyof" << std::endl;
	//	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	//	Parser<char> parseDigit = anyOf<char, std::string>(lstDigit);
	//	TResult<char> r6 = runOnInput<char, std::string>(parseDigit, "1azz");
	//	r6.match([](Success<char> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });


	//	std::cout << "parse Three Digit" << std::endl;
	//	using ThreePair = std::pair < std::pair<char, char>, char>;

	//	Parser<ThreePair, std::string> parseThreeDigit = andThen(andThen(parseDigit, parseDigit), parseDigit);
	//	using ThreePair = std::pair < std::pair<char, char>, char>;
	//	TResult<ThreePair, std::string> r7 = runOnInput<ThreePair, std::string>(parseThreeDigit, "123azz");
	//	r7.match(
	//		[](Success<ThreePair, std::string> r) { std::cout << "(" << "(" << r.value.first.first.first << ", " << r.value.first.first.second << ")" << ", " << r.value.first.second << ")" << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
	//	);

	//	//test parseThreeDigitAsStr
	//	Parser<std::string, std::string> parseThreeDigitAsStr = mapM<ThreePair, std::string, std::string>(parseThreeDigit,
	//		[](ThreePair x) -> std::string
	//	{
	//		std::stringstream s;
	//		s << x.first.first << x.first.second << x.second;
	//		std::string r = s.str();
	//		return r;
	//	});

	//	TResult<std::string, std::string> r8 = runOnInput<std::string, std::string>(parseThreeDigitAsStr, "199876azz");
	//	r8.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });





	//	//test applyM
	//	std::function< std::string(ThreePair)> af = [](ThreePair x) -> std::string
	//	{
	//		std::stringstream s;
	//		s << x.first.first << x.first.second << x.second;
	//		std::string r = s.str();
	//		return r;
	//	};
	//	using TFUN = std::function< std::string(ThreePair)>;
	//	Parser<std::string, std::string> ap = applyM<ThreePair, std::string>(returnM<TFUN, std::string>(af), parseThreeDigit);

	//	TResult<std::string, std::string> r9 = runOnInput<std::string, std::string>(ap, "199876azz");
	//	r9.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });




	//	//test lift2
	//	std::function< std::string(char a, char b)> al = [](char a, char b) -> std::string
	//	{
	//		std::stringstream s;
	//		s << a << b;
	//		std::string r = s.str();
	//		return r;
	//	};
	//	Parser<std::string, std::string> pParse2char2string = lift2<char, char, std::string>(al, pchar('n'), pchar('b'));

	//	TResult<std::string, std::string> r10 = runOnInput<std::string, std::string>(pParse2char2string, "nbgaxbzz");
	//	r10.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });

	//}



	//{
	//	std::cout << "sequence" << std::endl;
	//	std::list<Parser<char>> parsers = std::list<Parser<char>>{ pchar('a'), pchar('b'), pchar('c') };

	//	Parser<std::list<char>, std::string> combined = sequence<char, std::string>(parsers);


	//	TResult<std::list<char>, std::string> r11 = runOnInput<std::list<char>, std::string>(combined, "abc1azz");
	//	r11.match(

	//		[](Success<std::list<char>, std::string> r) { std::cout << "(";

	//	std::cout << "(";
	//	std::transform(r.value.first.begin(), r.value.first.end(), r.value.first.begin(),
	//		[](char c) -> char {  std::cout << c << " "; return c; });
	//	std::cout << ")";

	//	std::cout << ", " << r.value.second << ")" << std::endl; },

	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });
	//}






	//{
	//	std::cout << "test pstring" << std::endl;

	//	TResult< std::string, std::string> ret = runOnInput< std::string, std::string>(pstring<std::string>("ABC"), "AB|DE");
	//	ret.match([](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });
	//}



	//{
	//	std::cout << "test many" << std::endl;

	//	auto manyA = many(pchar('A'));
	//	TResult< std::list<char>, std::string> ret = runOnInput< std::list<char>, std::string>(manyA, "AAAAD");
	//	ret.match(
	//		[](Success<std::list<char>, std::string> r) { std::cout << "(";

	//	std::ostream_iterator<char> out_it(std::cout, " ");
	//	//std:copy(r.value.first.begin(), r.value.first.end(), out_it);
	//	std::transform(r.value.first.begin(), r.value.first.end(), out_it, [](char c)->char {   return c; });

	//	std::cout << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });
	//}



	//{
	//	std::cout << "test manyAb" << std::endl;

	//	auto manyAb = many<std::string, std::string>(pstring<std::string>("AB"));
	//	TResult< std::list<std::string>, std::string> ret = runOnInput< std::list<std::string>, std::string>(manyAb, "ABABAAAD");
	//	ret.match(
	//		[](Success<std::list<std::string>, std::string> r) { std::cout << "(";

	//	std::ostream_iterator<std::string> out_it(std::cout, " ");
	//	std::transform(r.value.first.begin(), r.value.first.end(), out_it, [](std::string c)->std::string {   return c; });

	//	std::cout << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; });

	//}



	//{
	//	std::cout << "test whitespace" << std::endl;

	//	Parser<std::list<char>, std::string> wts = whitespace< std::string>();
	//	TResult< std::list<char>, std::string> ret = runOnInput< std::list<char>, std::string>(wts, " a \t \n asdfaa");
	//	ret.match(
	//		[](Success<std::list<char>, std::string> r) { std::cout << "(";

	//	std::ostream_iterator<std::string> out_it(std::cout, " ");
	//	std::transform(r.value.first.begin(), r.value.first.end(), out_it, [](char c)->std::string {   return getPrintable(c); });

	//	std::cout << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout<< "Error parsing " << e.label << std::endl << e.error << std::endl; });

	//}

	//{

	//	std::cout << "test whitespace" << std::endl;

	//	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	//	Parser<char> parseDigit = anyOf<char, std::string>(lstDigit);
	//	using TDigList = std::list<char>;
	//	Parser<TDigList, std::string> digits = many1<char, std::string>(parseDigit);

	//	TResult<TDigList, std::string> ret = runOnInput<TDigList, std::string>(digits, "azz");
	//	ret.match(
	//		[](Success<TDigList, std::string> r) { std::cout << "(";

	//	std::ostream_iterator<char> out_it(std::cout, " ");
	//	std::copy(r.value.first.begin(), r.value.first.end(), out_it);


	//	std::cout << ", " << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
	//	);

	//}


	//{
	//	std::cout << "test opt parser" << std::endl;
	//	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	//	Parser<char> digit = anyOf<char, std::string>(lstDigit);

	//	using  TCharMaybeChar = std::pair<char, Maybe<char>>;
	//	Parser<std::pair<char, Maybe<char>>, std::string> digitThenSemicolon = andThen<char, Maybe<char>>(
	//		digit, opt<char, std::string>(pchar('a'))
	//		);

	//	TResult<TCharMaybeChar, std::string> ret = runOnInput<TCharMaybeChar, std::string>(digitThenSemicolon, "1ada--zz");


	//	ret.match(
	//		[](Success<TCharMaybeChar, std::string> r)
	//	{
	//		std::cout << "("; std::cout << r.value.first.first << " ";


	//		Maybe<char> mc = r.value.first.second;
	//		r.value.first.second.match(
	//			[](TNothing<char> r) {std::cout << "Nothing"; },
	//			[](TSome<char> r) {std::cout << "Some " << r.v; }

	//		);


	//		std::cout << ", " << r.value.second << ")" << std::endl;
	//	},
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
	//	);
	//}



	//{
	//	std::cout << "test pint parser" << std::endl;

	//	TResult<int, std::string> ret = runOnInput<int, std::string>(pint<std::string>(), "-2ada--zz");

	//	ret.match(
	//		[](Success<int, std::string> r) { std::cout << r.value.first << " " << r.value.second << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
	//	);
	//}


	//{
	//	std::cout << "test throw right" << std::endl;

	//	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	//	Parser<char> parseDigit = anyOf<char, std::string>(lstDigit);
	//		
	//	Parser<char> digitThenSemicolon = throwRight<char, Maybe<char>, std::string>(parseDigit, opt<char, std::string>(pchar(';')));

	//	TResult<char> ret = runOnInput<char, std::string>(digitThenSemicolon, "2;ada--zz");
	//	ret.match(
	//		[](Success<char> r) { std::cout<<"(" << r.value.first << " ," << r.value.second <<")"<< std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
	//	);
	//}



	//{
	//	std::cout << "test throw left" << std::endl;

	//	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	//	Parser<char> parseDigit = anyOf<char, std::string>(lstDigit);

	//	Parser<char> digitThenSemicolon = throwLeft< Maybe<char>, char, std::string>( opt<char, std::string>(pchar(';')), parseDigit);

	//	TResult<char> ret = runOnInput<char, std::string>(digitThenSemicolon, ";2;ada--zz");
	//	ret.match(
	//		[](Success<char> r) { std::cout << "(" << r.value.first << " ," << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << "Error parsing " << e.label << std::endl << e.error << std::endl; }
	//	);
	//}




	//{
	//	std::cout << "test ab \\t \\ncd -> (ab,cd), \"\"" << std::endl;

	//	Parser<std::list<char>, std::string> whitesps1= whitespace1<std::string>();

	//	Parser<std::string, std::string> pab = pstring< std::string>("AB");
	//	Parser<std::string, std::string> pcd = pstring< std::string>("CD");

	//	
	//	Parser<std::string, std::string> p1 = throwRight<std::string, std::list<char>, std::string>(pab, whitesps1);

	//	using TStringPair = std::pair<std::string, std::string>;
	//	Parser< TStringPair, std::string> pab_cd = andThen<std::string, std::string>(p1, pcd);


	//	TResult< TStringPair, std::string> ret = runOnInput<TStringPair, std::string>(pab_cd, "AB \t\n CD-d");
	//	ret.match(
	//		[](Success<TStringPair, std::string> r) { std::cout << "((" << r.value.first.first <<", " << r.value.first.second <<") ," << r.value.second << ")" << std::endl; },
	//		[](Error e) { std::cout << e.error << " "; });


	//}



	//{
	//	{
	//		std::cout << "test between." << std::endl;

	//		Parser<char> pdoublequote = pchar('"');

	//		Parser<int, std::string> quotedInteger = between<char, int, char, std::string>(pdoublequote, pint<std::string>(), pdoublequote);

	//		TResult< int, std::string> ret = runOnInput<int, std::string>(quotedInteger, "\"213\"AB \t\"\n CD-d");
	//		ret.match(
	//			[](Success<int, std::string> r) { std::cout << "(" << r.value.first <<"," << r.value.second << ")" << std::endl; },
	//			[](Error e) { std::cout << e.error << " "<<std::endl; });
	//	}
	//	{

	//		std::cout << "test between2." << std::endl;
	//		Parser<std::list<char>, std::string> whitesps = whitespace<std::string>();


	//		Parser<std::string, std::string> parseABC = pstring<std::string>("ABC");
	//		Parser<std::string, std::string> parse_ABC_ = ignoreWhitespaceAround< std::list<char>, std::string, std::list<char>>(whitesps, parseABC, whitesps);

	//		TResult< std::string, std::string> ret = runOnInput<std::string, std::string>(parseABC, " ABC");
	//		ret.match(
	//			[](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << "," << r.value.second << ")" << std::endl; },
	//			[](Error e) { std::cout << e.error << " " << std::endl; });
	//	

	//	}

	//	{

	//		std::cout << "test between3." << std::endl;
	//		Parser<std::list<char>, std::string> whitesps = whitespace<std::string>();


	//		Parser<std::string, std::string> parseABC = pstring<std::string>("ABC");
	//		Parser<std::string, std::string> parse_ABC_ = ignoreWhitespaceAround< std::list<char>, std::string, std::list<char>>(whitesps, parseABC, whitesps);

	//		TResult< std::string, std::string> ret = runOnInput<std::string, std::string>(parse_ABC_, " ABC  \n\t ad");
	//		ret.match(
	//			[](Success<std::string, std::string> r) { std::cout << "(" << r.value.first << "," << r.value.second << ")" << std::endl; },
	//			[](Error e) { std::cout << e.error << " "; });


	//	}

	//}


	//{
	//	std::cout << "test sep sep1" << std::endl;

	//	Parser<char>comma = pchar(',');

	//	std::list<char> lstDigit = { '0', '1' , '2' , '3' , '4' , '5' , '6' , '7' , '8' , '9' };
	//	Parser<char> parseDigit = anyOf<char, std::string>(lstDigit);


	//	Parser<std::list<char>, std::string> zeroOrMoreDigitList = sepBy<char, char, std::string>(parseDigit, comma);

	//	Parser<std::list<char>, std::string> oneOrMoreDigitList = sepBy1<char, char, std::string>(parseDigit, comma);


	//	std::list<std::string> testStrs = { "1;", "1,2;", "1,2,3;", "Z;" };

	//	for_each(testStrs.begin(), testStrs.end(), [zeroOrMoreDigitList, oneOrMoreDigitList](std::string s)
	//	{
	//		{
	//			std::cout << "zeroOrMoreDigitList:" << std::endl;
	//			TResult< std::list<char>, std::string> ret = runOnInput<std::list<char>, std::string>(zeroOrMoreDigitList, s);
	//			ret.match(
	//				[](Success<std::list<char>, std::string> r) { std::cout << "(";
	//			std::ostream_iterator<char> out_it(std::cout, " ");
	//			std::copy(r.value.first.begin(), r.value.first.end(), out_it);
	//			std::cout << "," << r.value.second << ")" << std::endl; },
	//				[](Error e) { std::cout << e.error << " "; });
	//		}
	//		{
	//			std::cout << "oneOrMoreDigitList:" << std::endl;
	//			TResult< std::list<char>, std::string> ret = runOnInput<std::list<char>, std::string>(oneOrMoreDigitList, s);
	//			ret.match(
	//				[](Success<std::list<char>, std::string> r) { std::cout << "(";
	//			std::ostream_iterator<char> out_it(std::cout, " ");
	//			std::copy(r.value.first.begin(), r.value.first.end(), out_it);
	//			std::cout << "," << r.value.second << ")" << std::endl; },
	//				[](Error e) { std::cout << e.error << " "; });
	//		}






	//		//std::cout << s << std::endl; 
	//	});


	//	


	//}




	getchar();

	return 0;
}


