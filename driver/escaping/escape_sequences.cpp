/*
 
 https://docs.faircom.com/doc/sqlref/#33384.htm
 https://docs.microsoft.com/ru-ru/sql/odbc/reference/appendixes/time-date-and-interval-functions
 https://my.vertica.com/docs/7.2.x/HTML/index.htm#Authoring/SQLReferenceManual/Functions/Date-Time/TIMESTAMPADD.htm%3FTocPath%3DSQL%2520Reference%2520Manual%7CSQL%2520Functions%7CDate%252FTime%2520Functions%7C_____43

*/
#include "escape_sequences.h"

#include <iostream>
#include <map>
#include "lexer.h"
#include "..\log/log.h"
#include <regex>


using namespace std;

namespace {

/*const std::map<const std::string, const std::string> fn_convert_map {
    {"SQL_TINYINT", "toUInt8"},
    {"SQL_SMALLINT", "toUInt16"},
    {"SQL_INTEGER", "toInt32"},
    {"SQL_BIGINT", "toInt64"},
    {"SQL_REAL", "toFloat32"},
    {"SQL_DOUBLE", "toFloat64"},
    {"SQL_VARCHAR", "toString"},
    {"SQL_DATE", "toDate"},
    {"SQL_TYPE_DATE", "toDate"},
    {"SQL_TIMESTAMP", "toDateTime"},
    {"SQL_TYPE_TIMESTAMP", "toDateTime"},
};*/

#define DECLARE2(TOKEN, NAME) \
    { Token::TOKEN, NAME }

const std::map<const Token::Type, const std::string> function_map {
#include "function_declare.h"
};

#undef DECLARE2

const std::map<const Token::Type, const std::string> function_map_strip_params {
    {Token::CURRENT_TIMESTAMP, "now()"},
};

const std::map<const Token::Type, const std::string> literal_map {
    // {Token::SQL_TSI_FRAC_SECOND, ""},
    {Token::SQL_TSI_SECOND, "'second'"},
    {Token::SQL_TSI_MINUTE, "'minute'"},
    {Token::SQL_TSI_HOUR, "'hour'"},
    {Token::SQL_TSI_DAY, "'day'"},
    {Token::SQL_TSI_WEEK, "'week'"},
    {Token::SQL_TSI_MONTH, "'month'"},
    {Token::SQL_TSI_QUARTER, "'quarter'"},
    {Token::SQL_TSI_YEAR, "'year'"},
};

const std::map<const Token::Type, const std::string> units_map{
	{Token::SQL_TSI_SECOND, "second"},
    {Token::SQL_TSI_MINUTE, "minute"},
    {Token::SQL_TSI_HOUR, "hour"},
    {Token::SQL_TSI_DAY, "day"},
    {Token::SQL_TSI_WEEK, "week"},
    {Token::SQL_TSI_MONTH, "month"},
    {Token::SQL_TSI_QUARTER, "quarter"},
    {Token::SQL_TSI_YEAR, "year"},
};

const std::map<const std::string, const std::string> replacement_map {
    {"DISTINCT", ""},
};


string processEscapeSequencesImpl(const StringView seq, Lexer & lex);

/*string convertFunctionByType(const StringView & typeName) {
    const auto type_name_string = typeName.to_string();
    if (fn_convert_map.find(type_name_string) != fn_convert_map.end())
        return fn_convert_map.at(type_name_string);

    return string();
}*/

string processParentheses(const StringView seq, Lexer & lex) {
    string result;
    lex.SetEmitSpaces(true);
    result += lex.Consume().literal.to_string(); // (

    while (true) {
        const Token token(lex.Peek());

        // std::cerr << __FILE__ << ":" << __LINE__ << " : "<< token.literal.to_string() << " type=" << token.type << " go\n";

        if (token.type == Token::RPARENT) {
            result += token.literal.to_string();
            lex.Consume();
            break;
        } else if (token.type == Token::LPARENT) {
            result += processParentheses(seq, lex);
        } else if (token.type == Token::LCURLY) {
            lex.SetEmitSpaces(false);
            result += processEscapeSequencesImpl(seq, lex);
            lex.SetEmitSpaces(true);
        } else if (token.type == Token::EOS || token.type == Token::INVALID) {
            break;
        } else {
            result += token.literal.to_string();
            lex.Consume();
        }
    }

    return result;
}

string processIdentOrFunction(const StringView seq, Lexer & lex) {
    while (lex.Match(Token::SPACE)) {
    }
    const auto token = lex.Peek();
    string result;

    if (token.type == Token::LCURLY) {
        lex.SetEmitSpaces(false);
        result += processEscapeSequencesImpl(seq, lex);
        lex.SetEmitSpaces(true);
    } else if(function_map.find(token.type) != function_map.end()) { //if function is not covered in {}, not sure if this is not a syntax error for ODBC
        result += function_map.at(token.type);                                            // func name
		lex.Consume();
		result += processParentheses(seq, lex);
    } else if (token.type == Token::LPARENT) {
        result += processParentheses(seq, lex);
    } else if (token.type == Token::IDENT && lex.LookAhead(1).type == Token::LPARENT) { // CAST( ... )
        result += token.literal.to_string();                                            // func name
        lex.Consume();
        result += processParentheses(seq, lex);
    } else if (token.type == Token::NUMBER || token.type == Token::IDENT || token.type == Token::STRING) {
        result += token.literal.to_string();
        lex.Consume();
    } else if (function_map_strip_params.find(token.type) != function_map_strip_params.end()) {
        result += function_map_strip_params.at(token.type);
    } else {
        return "";
    }
    while (lex.Match(Token::SPACE)) {
    }

    return result;
}

string processFunction(const StringView seq, Lexer & lex) {
    const Token fn(lex.Consume());

    /*if (fn.type == Token::CONVERT) {
        string result;
        if (!lex.Match(Token::LPARENT))
            return seq.to_string();

        auto num = processIdentOrFunction(seq, lex);
        if (num.empty())
            return seq.to_string();
        result += num;

        while (lex.Match(Token::SPACE)) {
        }

        if (!lex.Match(Token::COMMA)) {
            return seq.to_string();
        }

        while (lex.Match(Token::SPACE)) {
        }

        Token type = lex.Consume();
        if (type.type != Token::IDENT) {
            return seq.to_string();
        }

        string func = convertFunctionByType(type.literal.to_string());

        if (!func.empty()) {
            while (lex.Match(Token::SPACE)) {
            }
            if (!lex.Match(Token::RPARENT)) {
                return seq.to_string();
            }
            result = func + "(" + result + ")";
        }

        return result;

    } else */
	if (fn.type == Token::TIMESTAMPADD) {
        string result;
        if (!lex.Match(Token::LPARENT))
            return seq.to_string();

        if (function_map.find(fn.type) == function_map.end()) {
			LOG("TIMESTAMPADD not found");
			return seq.to_string();
		}
        string func = function_map.at(fn.type);
        Token type = lex.Consume();
		if (units_map.find(type.type) == units_map.end()) {
			LOG("Unit " << type.literal << " not found");
			return seq.to_string();
		}
        string addUnits = units_map.at(type.type);
		LOG("Preparing " << func << " for unit type " << addUnits);
        if (!lex.Match(Token::COMMA))
            return seq.to_string();
        auto ramount = processIdentOrFunction(seq, lex);
		if (ramount.empty()) {
			LOG("Could not parse ramount");
			return seq.to_string();
		}

        while (lex.Match(Token::SPACE)) {
        }

        if (!lex.Match(Token::COMMA))
            return seq.to_string();


        auto rdate = processIdentOrFunction(seq, lex);
		if (rdate.empty()) {
			LOG("Cold not parse rdate for");
			return seq.to_string();
		}

        if (!func.empty()) {
            while (lex.Match(Token::SPACE)) {
            }
            if (!lex.Match(Token::RPARENT)) {
                return seq.to_string();
            }
            result = func + "(" + addUnits + ", " + ramount + ", " + rdate + ")";
        }
        return result;

    } else if (fn.type == Token::LOCATE) {
        string result;
        if (!lex.Match(Token::LPARENT))
            return seq.to_string();

        auto needle = processIdentOrFunction(seq, lex /*, false */);
        if (needle.empty())
            return seq.to_string();
        lex.Consume();

        auto haystack = processIdentOrFunction(seq, lex /*, false*/);
        if (haystack.empty())
            return seq.to_string();
        lex.Consume();

        auto offset = processIdentOrFunction(seq, lex /*, false */);
        lex.Consume();

        result = "position(" + haystack + "," + needle + ")";

        return result;

    } else if (fn.type == Token::LTRIM) {
        if (!lex.Match(Token::LPARENT))
            return seq.to_string();

        auto param = processIdentOrFunction(seq, lex /*, false*/);
        if (param.empty())
            return seq.to_string();
        lex.Consume();
        return "replaceRegexpOne(" + param + ", '^\\\\s+', '')";

    } else if (function_map.find(fn.type) != function_map.end()) {
        string result = function_map.at(fn.type);
        auto func = result;
        lex.SetEmitSpaces(true);
        while (true) {
            const Token tok(lex.Peek());

            if (tok.type == Token::RCURLY) {
                break;
            } else if (tok.type == Token::LCURLY) {
                lex.SetEmitSpaces(false);
                result += processEscapeSequencesImpl(seq, lex);
                lex.SetEmitSpaces(true);
            } else if (tok.type == Token::EOS || tok.type == Token::INVALID) {
                break;
            } else if (tok.type == Token::EXTRACT) {
                result += processFunction(seq, lex);
            } else {
                if (func != "EXTRACT" && literal_map.find(tok.type) != literal_map.end()) {
					LOG("Adding literal" << literal_map.at(tok.type) << " to " << result);
                    result += literal_map.at(tok.type);
                } else
                    result += tok.literal.to_string();
                lex.Consume();
            }
        }
        lex.SetEmitSpaces(false);

        return result;

    } else if (function_map_strip_params.find(fn.type) != function_map_strip_params.end()) {
        string result = function_map_strip_params.at(fn.type);

        if (lex.Peek().type == Token::LPARENT) {
            processParentheses(seq, lex); // ignore anything inside ( )
        }

        return result;
    }

    return seq.to_string();
}

string processDate(const StringView seq, Lexer & lex) {
    Token data = lex.Consume(Token::STRING);
    if (data.isInvalid()) {
        return seq.to_string();
    } else {
        return data.literal.to_string();
    }
}

string removeMilliseconds(const StringView token) {
    if (token.empty()) {
        return string();
    }

    const char * begin = token.data();
    const char * p = begin + token.size() - 1;
    const char * dot = nullptr;
    const bool quoted = (*p == '\''); //TODO check millisec quotes
    if (quoted) {
        --p;
    }
    for (; p > begin; --p) {
        if (isdigit(*p)) {
            continue;
        }
        if (*p == '.') {
            if (dot) {
                return token.to_string();
            }
            dot = p;
        } else {
            if (dot) {
                return string(begin, dot) + (quoted ? "\'" : "");
            }
            return token.to_string();
        }
    }

    return token.to_string();
}

string processDateTime(const StringView seq, Lexer & lex) {
    Token data = lex.Consume(Token::STRING);
    if (data.isInvalid()) {
        return seq.to_string();
    } else {
        return removeMilliseconds(data.literal);
    }
}

string processEscapeSequencesImpl(const StringView seq, Lexer & lex) {
    string result;

    if (!lex.Match(Token::LCURLY)) {
        return seq.to_string();
    }

    while (true) {
        while (lex.Match(Token::SPACE)) {
        }
        const Token tok(lex.Consume());

        switch (tok.type) {
            case Token::FN:
                result += processFunction(seq, lex);
                break;

            case Token::D:
                result += processDate(seq, lex);
                break;
            case Token::TS:
                result += processDateTime(seq, lex);
                break;

            // End of escape sequence
            case Token::RCURLY:
                return result;

            // Unimplemented
            case Token::T:
            default:
                return seq.to_string();
        }
    };
}

string processEscapeSequences(const StringView seq) {
    Lexer lex(seq);
    return processEscapeSequencesImpl(seq, lex);
}

string replaceForbiddenSequences(const StringView seq) {
    Lexer lex(seq);
    string result;

    while(true) {
        const Token token(lex.Consume());
        if(token.isInvalid()) {
            return seq.to_string();
        }
        if(token.type == Token::EOS) {
            break;
        }

        if(replacement_map.find(token.literal.to_string()) != replacement_map.end()) {
            result += replacement_map.at(token.literal.to_string());
            result += " ";
        } else {
            result += token.literal.to_string();
            result += " ";
        }
    }

    return result;
}

string unquoteColumns(const string query) {
	string moidified_query;
	size_t previous_pos = 0;

	for (size_t pos = query.find(" "); pos != std::string::npos; pos = query.substr(previous_pos, query.size() - previous_pos).find(" ")) {
		string literal = query.substr(previous_pos, pos); //searching for every literal that is splitted by space
		std::regex subcolumn_regex("\".+\"\\.\".+\\..+\""); // e.g. "bi_ex_net1_m"."entity.literal"

		if (std::regex_match(literal, subcolumn_regex)) {
			LOG("Matches regex");
			size_t column_delimeter = literal.find(".");
			std::regex table_regex("\".+\""); //if table name contains "."
			while (!std::regex_match(literal.substr(0, column_delimeter), table_regex)) {
				column_delimeter += literal.substr(column_delimeter + 1, literal.size() - column_delimeter).find(".") + 1;
			}
			string table_name = literal.substr(0, column_delimeter); //TODO log table_name, literal.size, column delimeter
			moidified_query += table_name; //appending table name
			moidified_query += "." + literal.substr(column_delimeter + 2, literal.size() - column_delimeter - 3); //skipping quotes
		} else {
			moidified_query += literal;
		}

		moidified_query += " ";
		previous_pos += pos + 1; //to skip current space symbol
		}

	return moidified_query;
}

} // namespace

std::string replaceEscapeSequences(const std::string & query) {
    const char * p = query.c_str();
    const char * end = p + query.size();
    const char * st = p;
    int level = 0;
    std::string ret;

    while (p != end) {
        switch (*p) {
            case '{':
                if (level == 0) {
                    if (st < p) {
                        ret += std::string(st, p);
                    }
                    st = p;
                }
                level++;
                break;

            case '}':
                if (level == 0) {
                    // TODO unexpected '}'
                    return query;
                }
                if (--level == 0) {
                    ret += processEscapeSequences(StringView(st, p + 1));
                    st = p + 1;
                }
                break;
        }

        ++p;
    }

    if (st < p) {
        ret += std::string(st, p);
    }

    const char * ret_start = ret.c_str();
    const char * ret_end = ret_start + ret.size();
    string replacedQuery = replaceForbiddenSequences(StringView(ret_start, ret_end));

    return unquoteColumns(replacedQuery);
}
